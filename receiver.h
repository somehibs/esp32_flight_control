#ifndef RECEIVER_H
#define RECEIVER_H

// this is an sbus implementation

#include "shared.h"

ControlPacket currentPacket;
SBusMessage currentMessage;
bool outstandingPacket = false;

#define DEAD_PKT_THRESHOLD 5
#define LOST_PKT_THRESHOLD 2
short dead = 0;

/*
struct ControlPacket {
  short channels[MAX_ANALOG_CHANNELS];
  byte digitalChannels[2]; // MAX_ANALOG_CHANNELS + 4 digital channels on ch15 ch16 ch17 ch18
};
*/
#define SBUS_CHANNEL_SIZE 11
void refreshSbusMessage() {
  memset(&currentMessage, 0, sizeof(SBusMessage));
  currentMessage.header = 0x0F;
  //currentPacket.channels[22]; // 16 11bit channels
  short chanIndex = 0;
  for (int i = 0; i < MAX_ANALOG_CHANNELS; ++i) {
    memcpy(currentMessage.channels+chanIndex, currentPacket.channels+i, SBUS_CHANNEL_SIZE);
    chanIndex += 1;
  }
  for (int i = 0; i < MAX_DIGITAL_BYTES; ++i) {
    for (int j = 0; j < 4; ++j) {
      // If it's on, it's on. If it's off, it's off.
      chanIndex += 1;
    }
  }
  if (dead > DEAD_PKT_THRESHOLD) {
    currentMessage.postfix |= 1<<4;
  }
  if (dead > LOST_PKT_THRESHOLD) {
    currentMessage.postfix |= 1<<3;
  }
  currentMessage.footer = 0x00;
}
 
byte txPin, rxPin;

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
  Serial2.begin(100000, SERIAL_8E2, rxPin, txPin);
  if (Serial2) {
    Serial.println("SBUS opened OK");
    return true;
  } else {
    Serial.println("SBUS fail");
    emitTelemetryStatus(RECVR_SBUS_NOT_READY);
    return false;
  }
}

void init_receiver(byte sbus_tx, byte sbus_rx) {
  // clear packet
  memset(&currentPacket, 0, sizeof(ControlPacket));
  outstandingPacket = false;

  // configure sbus
  txPin = sbus_tx;
  rxPin = sbus_rx;
  // start sbus
  connectSBus();
}

void maintain_telemetry() {
  // Make sure the RX bus is serviced
  if (Serial2) {
    while (Serial2.available()) {
      // Figure out how to consume telemetry data. For now, suck it up to make sure no one gets full buffers.
      Serial2.read();
    }
  }
}

void writeSBusMessage() {
    long written = 25;//Serial2.write((uint8_t*)&msg, sizeof(SBusMessage));
    Serial.print(written, DEC);
    if (written == sizeof(SBusMessage)) {
      Serial.println(" bytes written to SBus OK.");
    } else {
      Serial.println(" bytes written to SBus does not match expected size!");
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
    Serial.printf("R: %d P: %d Y: %d T: %d digi: %d dig2: %d\n", packet->channels[0], packet->channels[1], packet->channels[2], packet->channels[3], packet->digitalChannels[0], packet->digitalChannels[1]);
    refreshSbusMessage();
    outstandingPacket = false;
    writeSBusMessage();
  } else {
    dead += 1;
    if (dead > DEAD_PKT_THRESHOLD) {
      currentMessage.postfix |= 1<<4;
    }
    if (dead > LOST_PKT_THRESHOLD) {
      currentMessage.postfix |= 1<<3;
    }
    writeSBusMessage();
    Serial.println("Missed a message");
  }
}

void loop_receiver() {
  maintain_telemetry();
  maintain_sbus();
  delay(1);
}

void send_callback(const uint8_t* peerMac, bool sent) {
  
}

void recv_callback(const uint8_t* peerMac,  const uint8_t *data, int data_len) {
  // Probably got a control packet
  if (data_len == sizeof(ControlPacket)) {
    ControlPacket packet = *(ControlPacket*)data;
    memcpy(&currentPacket, &packet, sizeof(ControlPacket));
    outstandingPacket = true;
  }
}

#endif // RECEIVER_H
