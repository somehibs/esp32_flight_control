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
    if (suppress) return 0x0000;
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
    if (state >= diff) {
      return map(state, getMid(), ANALOG_MAX, FloatingAnalogMidOutput, FloatingAnalogMaxOutput);
    } else {
      // state < mid
      return map(state, 0, getMid(), FloatingAnalogMinOutput, FloatingAnalogMidOutput);
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
/*rightAnalogH.getChannel(); // roll
rightAnalogV.getChannel(); // pitch
leftAnalogH.getChannel(); // yaw
throttlePot.getChannel(); // throttle*/
FloatingAnalog leftAnalogH = FloatingAnalog(37);
FloatingAnalog leftAnalogV = FloatingAnalog(32);
FloatingAnalog rightAnalogH = FloatingAnalog(33);
FloatingAnalog rightAnalogV = FloatingAnalog(36);
FloatingAnalog throttlePot = FloatingAnalog(38);
FloatingAnalog sensitivityPot = FloatingAnalog(39);

DigitalSwitch leftMost = DigitalSwitch(13);
DigitalSwitch leftMid = DigitalSwitch(12);
DigitalSwitch leftLeast = DigitalSwitch(0);
DigitalSwitch rightMost = DigitalSwitch(0);
DigitalSwitch rightMid = DigitalSwitch(0);
DigitalSwitch rightLeast = DigitalSwitch(0);

DigitalSwitch* digitalChannelOne[4] = {0, 0, &leftLeast, &rightMost};
DigitalSwitch* digitalChannelTwo[4] = {&rightMid, &rightLeast, &leftMid, &leftMost};

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
    analogSetCycles(20);
    analogSetSamples(1);
    for (int i = 0; i < 256; ++i) {
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
    sensitivityPot.low = 0;
    sensitivityPot.diff = 0;
    sensitivityPot.high = ANALOG_MAX;
    //sensitivityPot.invert = true;
  }

  void readPinsIntoPacket(ControlPacket* packet) {
    short sensitivity = sensitivityPot.getChannel();
    FloatingAnalogMaxOutput = map(sensitivity, 1000, 2000, 1750, 2000); // output strength mapped between 1750 and 2000
    FloatingAnalogMinOutput = map(sensitivity, 1000, 2000, 1350, 1000); // yay inversion!
    //Serial.printf("Sensitivity: %d max %d min %d\n", sensitivity, FloatingAnalogMaxOutput, FloatingAnalogMinOutput);
    packet->channels[0] = rightAnalogH.getChannel(); // roll
    packet->channels[1] = rightAnalogV.getChannel(); // pitch
    packet->channels[3] = leftAnalogH.getChannel(); // yaw
    packet->channels[2] = throttlePot.getChannel(); // throttle
    packet->digitalChannels[0] = getDigitalChannel(0);
    packet->digitalChannels[1] = getDigitalChannel(1);
    //Serial.print(packet->digitalChannels[1], BIN);
    //Serial.printf(" R: %d P: %d Y: %d T: %d digi: %d dig2: %d\n", packet->channels[0], packet->channels[1], packet->channels[3], packet->channels[2], packet->digitalChannels[0], packet->digitalChannels[1]);
  }

  byte getDigitalChannel(short channel) {
    byte ret = 0;
    DigitalSwitch** switches = digitalChannelOne;
    if (channel == 1) {
      switches = digitalChannelTwo;
    }
    
    for (int i = 0; i < 4; ++i) {
      if (switches[i] != 0) {
        ret |= switches[i]->getChannel(3-i);
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
      tft.renderControlState(packet);
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
  ctl.init(true);
}

void loop_controller() {
  ctl.loop();
  delay(5);
}

void send_callback(const uint8_t* peerMac, bool sent) {
  ctl.sent(peerMac, sent);
}
void recv_callback(const uint8_t* peerMac,  const uint8_t *data, int data_len) {
  ctl.recv(peerMac, data, data_len);
}
