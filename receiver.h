#ifndef RECEIVER_H
#define RECEIVER_H

// this is an sbus receiver implementation
#include "shared.h"

// this is sport, but you could comment it out and implement another telemetry mechanism. this seems fairly flexible, so it's probably your first preference.
// you can add on to the smartport system with your own vendor specific codes - we don't have to be compatible with frsky nor do our own checksumming thanks to protocol layer checksums and retransmission (chinese espnow radio stack)
// note the custom behaviour in the telemetry system to handle dead packets and transmit time bound historical averages to the controller
#include "telemetry.h"

ControlPacket currentPacket;
bool mutatedCurrentPacket = false;
SBusMessage currentMessage;
bool outstandingPacket = false;

#define FAILSAFE_THRESHOLD 250
#define LOST_PKT_THRESHOLD 50
#define SBUS_FAILSAFE_OFFSET 3
#define SBUS_LOST_PKT_OFFSET 2
short dead = 0;

void reversi() {
  currentMessage.footer = 0x0f;
  currentMessage.header = 0x00;
}
#define SBUS_CHANNEL_SIZE 3
void refreshSbusMessage() {
  memset(&currentMessage, 0, sizeof(SBusMessage));
  currentMessage.header = 0x0F;
  //currentPacket.channels[22]; // 16 11bit channels
  if (!mutatedCurrentPacket) {
    for (int i = 0; i < MAX_ANALOG_CHANNELS; ++i) {
      currentPacket.channels[i] = map(currentPacket.channels[i], 1000, 2000, 173, 1811);//192, 1792);
    }
    mutatedCurrentPacket = true;
  }

  short offset = 0;
  short byteindex = 1;
  // Bind wire analog channels to SBus channels (currently RPTY to reduce packet size)
  for (unsigned i = 0; (i < MAX_ANALOG_CHANNELS) && (i < MAX_SBUS_CHANNELS); ++i) {
    /*protect from out of bounds values and limit to 11 bits*/
    if (currentPacket.channels[i] > 0x07ff) {
      currentPacket.channels[i] = 0x07ff;
    }
    while (offset >= 8) {
      ++byteindex;
      offset -= 8;
    }
    ((byte*)&currentMessage)[byteindex] |= (currentPacket.channels[i] << (offset)) & 0xff;
    ((byte*)&currentMessage)[byteindex + 1] |= (currentPacket.channels[i] >> (8 - offset)) & 0xff;
    ((byte*)&currentMessage)[byteindex + 2] |= (currentPacket.channels[i] >> (16 - offset)) & 0xff;
    offset += 11;
  }

  // Doesn't currently bind wire digital channels to SBus AUX channels
  short chanIndex = 0;
  for (int i = 0; i < MAX_DIGITAL_BYTES; ++i) {
    for (int j = 0; j < 4; ++j) {
      if (i == MAX_DIGITAL_BYTES-1 && j >= 2) {
        // protect last two channels from assignment, leaving them for postfix 17 and 18
        break;
      }
      // If it's on, it's on. If it's off, it's off.
      chanIndex += 1;
    }
  }
  if ( (currentPacket.digitalChannels[MAX_DIGITAL_BYTES-1]) & 0x1) {
    currentMessage.postfix |= 1;
  }
  if ( (currentPacket.digitalChannels[MAX_DIGITAL_BYTES-1] >> 1) & 0x1) {
    currentMessage.postfix |= 1<<1;
  }
  if (dead > LOST_PKT_THRESHOLD) {
    currentMessage.postfix |= 1<<SBUS_LOST_PKT_OFFSET;
  }
  if (dead > FAILSAFE_THRESHOLD) {
    currentMessage.postfix |= 1<<SBUS_FAILSAFE_OFFSET;
  }
  currentMessage.footer = 0x00;
  //reversi();
}
unsigned long ok = 0;
void writeSBusMessage() {
    long written = Serial2.write((uint8_t*)&currentMessage, sizeof(SBusMessage));
    if (written == sizeof(SBusMessage)) {
      ok += 1;
      if (ok % 100 == 0) {
        Serial.print(ok, DEC);
        Serial.println(" messages written to SBus OK.");
        Serial.print(currentPacket.digitalChannels[MAX_DIGITAL_BYTES-1], BIN);
        Serial.print(" digi, ");
        Serial.print(currentMessage.postfix, BIN);
        Serial.println(" postfix.");
      }
    } else {
      Serial.println(" bytes written to SBus does not match expected size!");
    }
}


byte txPin;

void emitTelemetryStatus(byte status) {
  Serial.print("Telemetry status send: ");
  Serial.print(status, DEC);
  Serial.println();
  // check last emission time, don't say it more than every 100ms
  if (lastPacketSent < millis()-1000) {
    TelemetryPacket packet;
    memset(&packet, 0, sizeof(TelemetryPacket));
    packet.status = status;
  }
}

void emitTelemetry() {
  emitTelemetryStatus(RECVR_OK);
}

bool connectSBus() {
  Serial2.begin(100000, SERIAL_8E2, 25, txPin);
  if (Serial2) {
    Serial.println("SBUS opened OK");
    return true;
  } else {
    Serial.println("SBUS fail");
    emitTelemetryStatus(RECVR_SBUS_NOT_READY);
    return false;
  }
}

// use another class with similar function names if you want to swap out telemetry. ensure everything is delegated to the telemetry class
SmartPort telemetry;
void init_receiver(byte sbus_tx, byte telemetry_rx, byte telemetry_tx) {
  // clear packet
  memset(&currentPacket, 0, sizeof(ControlPacket));
  outstandingPacket = false;

  // configure sbus
  txPin = sbus_tx;
  // start sbus
  connectSBus();
  //telemetry.init(telemetry_rx, telemetry_tx);
}

void maintain_sbus();
void loop_receiver() {
  maintain_sbus();
  //telemetry.maintain();
  if (dead > 0) {
    delay(1); // be lenient, wait 2ms per dead increment
  }
}

void maintain_sbus() {
  if (!Serial2) {
    connectSBus();
    return;
  }
  if (outstandingPacket) {
    // Convert the packet into an SBUS message
    ControlPacket* packet = &currentPacket;
    //Serial.printf("R: %d P: %d Y: %d T: %d digi: %d dig2: %d\n", packet->channels[0], packet->channels[1], packet->channels[2], packet->channels[3], packet->digitalChannels[0], packet->digitalChannels[1]);
    refreshSbusMessage();
    outstandingPacket = false;
    dead = 0;
    writeSBusMessage();
  } else {
    dead += 1;
    if (dead > LOST_PKT_THRESHOLD) {
      currentMessage.postfix |= 1<<SBUS_LOST_PKT_OFFSET;
    }
    if (dead > FAILSAFE_THRESHOLD) {
      currentMessage.postfix |= 1<<SBUS_FAILSAFE_OFFSET;
    }
    writeSBusMessage();
  }
}

void send_callback(const uint8_t* peerMac, bool sent) {
  
}

void recv_callback(const uint8_t* peerMac,  const uint8_t *data, int data_len) {
  // Probably got a control packet
  if (data_len == sizeof(ControlPacket)) {
    ControlPacket packet = *(ControlPacket*)data;
    memcpy(&currentPacket, &packet, sizeof(ControlPacket));
    mutatedCurrentPacket = false;
    outstandingPacket = true;
  }
}

#endif // RECEIVER_H
