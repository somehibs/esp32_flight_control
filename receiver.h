#ifndef RECEIVER_H
#define RECEIVER_H

// this is an sbus implementation

#include "shared.h"

ControlPacket currentPacket;
bool mutatedCurrentPacket = false;
SBusMessage currentMessage;
bool outstandingPacket = false;

#define DEAD_PKT_THRESHOLD 5
#define LOST_PKT_THRESHOLD 2
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
  short chanIndex = 0;
  if (!mutatedCurrentPacket) {
    for (int i = 0; i < MAX_ANALOG_CHANNELS; ++i) {
      currentPacket.channels[i] = map(currentPacket.channels[i], 1000, 2000, 173, 1811);//192, 1792);
    }
    mutatedCurrentPacket = true;
  }
  currentMessage.channels[1] = (uint8_t) ((currentPacket.channels[0] & 0x07FF));
  currentMessage.channels[2] = (uint8_t) ((currentPacket.channels[0] & 0x07FF)>>8 | (currentPacket.channels[1] & 0x07FF)<<3);
  currentMessage.channels[3] = (uint8_t) ((currentPacket.channels[1] & 0x07FF)>>5 | (currentPacket.channels[2] & 0x07FF)<<6);
  currentMessage.channels[4] = (uint8_t) ((currentPacket.channels[2] & 0x07FF)>>2);
  currentMessage.channels[5] = (uint8_t) ((currentPacket.channels[2] & 0x07FF)>>10 | (currentPacket.channels[3] & 0x07FF)<<1);
  currentMessage.channels[6] = (uint8_t) ((currentPacket.channels[3] & 0x07FF)>>7; //| (currentPacket.channels[4] & 0x07FF)<<4);
  /*currentMessage.channels[7] = (uint8_t) ((currentPacket.channels[4] & 0x07FF)>>4); | (currentPacket.channels[5] & 0x07FF)<<7);
  currentMessage.channels[8] = (uint8_t) ((currentPacket.channels[5] & 0x07FF)>>1);
  currentMessage.channels[9] = (uint8_t) ((currentPacket.channels[5] & 0x07FF)>>9 | (currentPacket.channels[6] & 0x07FF)<<2);
  currentMessage.channels[10] = (uint8_t) ((currentPacket.channels[6] & 0x07FF)>>6 | (currentPacket.channels[7] & 0x07FF)<<5);
  currentMessage.channels[11] = (uint8_t) ((currentPacket.channels[7] & 0x07FF)>>3);
  currentMessage.channels[12] = (uint8_t) ((currentPacket.channels[8] & 0x07FF));
  currentMessage.channels[13] = (uint8_t) ((currentPacket.channels[8] & 0x07FF)>>8 | (currentPacket.channels[9] & 0x07FF)<<3);
  currentMessage.channels[14] = (uint8_t) ((currentPacket.channels[9] & 0x07FF)>>5 | (currentPacket.channels[10] & 0x07FF)<<6);
  currentMessage.channels[15] = (uint8_t) ((currentPacket.channels[10] & 0x07FF)>>2);
  currentMessage.channels[16] = (uint8_t) ((currentPacket.channels[10] & 0x07FF)>>10 | (currentPacket.channels[11] & 0x07FF)<<1);
  currentMessage.channels[17] = (uint8_t) ((currentPacket.channels[11] & 0x07FF)>>7 | (currentPacket.channels[12] & 0x07FF)<<4);
  currentMessage.channels[18] = (uint8_t) ((currentPacket.channels[12] & 0x07FF)>>4 | (currentPacket.channels[13] & 0x07FF)<<7);
  currentMessage.channels[19] = (uint8_t) ((currentPacket.channels[13] & 0x07FF)>>1);
  currentMessage.channels[20] = (uint8_t) ((currentPacket.channels[13] & 0x07FF)>>9 | (currentPacket.channels[14] & 0x07FF)<<2);
  currentMessage.channels[21] = (uint8_t) ((currentPacket.channels[14] & 0x07FF)>>6 | (currentPacket.channels[15] & 0x07FF)<<5);
  currentMessage.channels[22] = (uint8_t) ((currentPacket.channels[15] & 0x07FF)>>3);*/
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
  //reversi();
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
    long written = Serial2.write((uint8_t*)&currentMessage, sizeof(SBusMessage));
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
    dead = 0;
  } else {
    dead += 1;
    if (dead > DEAD_PKT_THRESHOLD) {
      currentMessage.postfix |= 1<<4;
    }
    if (dead > LOST_PKT_THRESHOLD) {
      currentMessage.postfix |= 1<<3;
    }
    //writeSBusMessage();
    //Serial.println("Missed a message");
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
    mutatedCurrentPacket = false;
    outstandingPacket = true;
  }
}

#endif // RECEIVER_H
