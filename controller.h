// This class is kinda custom to my setup (tft-esp32)
// readPinsIntoPacket will read from the constants below
// calibration will block execution for a while during a constant read of the sticks to get the 'middle' value and the floating amount to avoid 'updating' floating points

#define ANALOG_MAX 4095

// digital switch from a button
class DigitalSwitch {
public:
  DigitalSwitch(short pin) {
    // make sure the pin is low.
    this->pin = pin;
    if (pin == 0) {
      suppress = true;
    } else {
      suppress = false;
    }
    lastRead = false;
    state = false;
  }

  byte getChannel(short chan) {
    if (chan == 2) return 1 << chan;
    if (suppress) return false;
    byte newState = digitalRead(pin);
    if (newState != lastRead) {
      if (newState) {
        // Flip the state if the button is pressed. Button release does nothing
        state = !state;
      }
      lastRead = newState;
    }
    // This is a chance to read the real pin and update any stored value. Before returning the stored value anyway.
    return state << chan;
  }

  bool suppress;
  bool state;
  bool lastRead;
  short pin;
};

short FloatingAnalogMaxOutput = 2000;
short FloatingAnalogMinOutput = 1000;
short FloatingAnalogMidOutput = 1500;
class FloatingAnalog {
public:
  FloatingAnalog(short pin, const char* debugName) {
    init(pin);
    this->debugName = debugName;
  }
  FloatingAnalog(short pin) {
    init(pin);
  }
  
  void init(short pin) {
    this->debugName = 0;
    this->pin = pin;
    low = 999999;
    high = -999999;
    invert = false;
  }

  void calibrate_step() {
    short r = analogRead(pin);
    if (r < low) {
      low = r;
    }
    if (r > high) {
      high = r;
    }
    diff = (high-low)/2;
    if (this->debugName != 0) {
      Serial.printf("Calibrate %s: %d h:%d l:%d m:%d\n", this->debugName, r, high, low, getMid());
    }
  }

  short getMid() {
    return high-(diff/2);
  }

  short getChannel() {
    short state = analogRead(pin);
    if (high == ANALOG_MAX) {
      if (invert) {
        state = ANALOG_MAX - state;
      }
      return map(state, 0, ANALOG_MAX, 1000, 2000);
    }
    // 0 - mid is 1500-2000
    // mid should be 2000 perfectly
    // mid - high is 2000-2500
    short offset = 1500;
    if (state >= diff) {
      offset += 500;
      return map(state, getMid(), ANALOG_MAX, 1500, 2000);
    } else {
      // state < mid
      return map(state, 0, getMid(), 1000, 1500);
    }
  }
  
  // measurements from multiple calibrate() calls
  short low;
  short high;
  short diff;

  // hacked together control for inverting a variable resistor 
  bool invert;

  // pin to use for analog reads
  short pin; 
  const char* debugName;
};

FloatingAnalog leftAnalogH = FloatingAnalog(33);
FloatingAnalog leftAnalogV = FloatingAnalog(32);
FloatingAnalog rightAnalogH = FloatingAnalog(35);
FloatingAnalog rightAnalogV = FloatingAnalog(34);
FloatingAnalog throttlePot = FloatingAnalog(36);
FloatingAnalog sensitivityPot = FloatingAnalog(-1);

DigitalSwitch leftMost = DigitalSwitch(0);
DigitalSwitch leftMid = DigitalSwitch(0);
DigitalSwitch leftLeast = DigitalSwitch(0);
DigitalSwitch rightMost = DigitalSwitch(0);
DigitalSwitch rightMid = DigitalSwitch(0);
DigitalSwitch rightLeast = DigitalSwitch(0);

DigitalSwitch* digitalChannelOne[4] = {&leftMost, &leftMid, &leftLeast, &rightMost};
DigitalSwitch* digitalChannelTwo[4] = {&rightMid, &rightLeast, 0, 0};

#include "tft.h"
#include "shared.h"

class Controller {
public:
  void init(bool enableTft) {
    if (enableTft) {
      updateTft = true;
      tft.init();
    }
    // Packets are only sent once the last packet finishes sending (success or fail), so true for the first loop
    pktComplete = true;
    calibrate();
  }

  void calibrate() {
    // increase the sample count to improve reading accuracy
    // don't increase too much, or you'll end up sampling slightly low
    analogSetCycles(18);
    analogSetSamples(1);
    leftMost.state = true;
    for (int i = 0; i < 5000; ++i) {
      leftAnalogH.calibrate_step();
      leftAnalogV.calibrate_step();
      rightAnalogH.calibrate_step();
      rightAnalogV.calibrate_step();
    }
    // low / high -
    throttlePot.low = 0;
    throttlePot.diff = 0;
    throttlePot.high = ANALOG_MAX;
    throttlePot.invert = true;
  }

  void readPinsIntoPacket(ControlPacket* packet) {
    short sensitivity = sensitivityPot.getChannel();
    FloatingAnalogMaxOutput = map(sensitivity, 1000, 2000, 1750, 2000); // output strength mapped between 1750 and 2000
    FloatingAnalogMinOutput = map(sensitivity, 1000, 2000, 1350, 1000); // yay inversion!
    packet->channels[0] = rightAnalogH.getChannel(); // roll
    packet->channels[1] = rightAnalogV.getChannel(); // pitch
    packet->channels[3] = leftAnalogH.getChannel(); // yaw
    packet->channels[2] = throttlePot.getChannel(); // throttle
    packet->digitalChannels[0] = getDigitalChannel(0);
    packet->digitalChannels[1] = getDigitalChannel(1);
    //Serial.printf("R: %d P: %d Y: %d T: %d digi: %d dig2: %d\n", packet->channels[0], packet->channels[1], packet->channels[2], packet->channels[3], packet->digitalChannels[0], packet->digitalChannels[1]);
  }

  byte getDigitalChannel(short channel) {
    byte ret = 0;
    DigitalSwitch** switches = digitalChannelOne;
    if (channel == 1) {
      switches = digitalChannelTwo;
    }
    
    for (int i = 1; i < 5; ++i) {
      if (switches[i-1] != 0) {
        ret |= switches[i-1]->getChannel(i);
      }
    }
    return ret;
  }

  void loop() {
    // Update the local control packet, send local state to FC
    ControlPacket packet;
    // Check for new data in any buffers, but don't take too long to render!
    if (pktComplete) {
      pktComplete = false;
      readPinsIntoPacket(&packet);
      send_radio((const uint8_t*)&packet, sizeof(ControlPacket));
      tft.updatePackets();
    }
    if (updateTft) {
      tft.updatePackets();
      updateTft = false;
    }
  }

  void sent(const uint8_t* peerMac, bool sent) {
    pktComplete = true;
  }
  void recv(const uint8_t* peerMac,  const uint8_t *data, int data_len) {
    updateTft = true;
  }
  
private:
  TFT tft;
  bool updateTft;
  bool pktComplete;
};

Controller ctl;

void init_controller() {
  ctl.init(false);
}

void loop_controller() {
  ctl.loop();
  delay(3);
}

void send_callback(const uint8_t* peerMac, bool sent) {
  ctl.sent(peerMac, sent);
}
void recv_callback(const uint8_t* peerMac,  const uint8_t *data, int data_len) {
  ctl.recv(peerMac, data, data_len);
}
