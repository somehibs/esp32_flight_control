#ifndef RECEIVER_H
#define RECEIVER_H

// this is an sbus implementation

#include "shared.h"

ControlPacket currentPacket;
bool outstandingPacket;

SBusMessage newSBusMessage() {
  SBusMessage msg;
  msg.header = 0x0F;
  //msg.channels;
  //msg.postfix;
  msg.footer = 0x00;
  return msg;
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

void connectSBus() {
  Serial2.begin(100000, SERIAL_8E2, rxPin, txPin);
  if (Serial2) {
    Serial.println("SBUS opened OK");
  } else {
    Serial.println("SBUS fail");
    emitTelemetryStatus(RECVR_SBUS_NOT_READY);
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
    if (Serial2.available()) {
      // Consume any serial data
      Serial2.read();
    }
  } else {
    // emit an error
    emitTelemetryStatus(RECVR_SBUS_NOT_READY);
    // restart serial
    connectSBus();
  }
  /*if (okForTelemetry()) {
    emitTelemetry();
  } else {
    consumeTelemetry();
  }*/
}

void maintain_sbus() {
  if (outstandingPacket) {
    if (!Serial2) {
      emitTelemetryStatus(RECVR_SBUS_NOT_READY);
      return;
    }
    
    // Convert the packet into an SBUS message
    SBusMessage msg = newSBusMessage();
    long written = Serial2.write((uint8_t*)&msg, sizeof(SBusMessage));
    Serial.print(written, DEC);
    if (written == sizeof(SBusMessage)) {
      Serial.println(" bytes written to SBus OK.");
    } else {
      Serial.println(" bytes written to SBus does not match expected size!");
    }
    outstandingPacket = false;
  }
}

void loop_receiver() {
  maintain_sbus();
  maintain_telemetry();
  delay(2000);
}

void send_callback(const uint8_t* peerMac, bool sent) {
  
}

void recv_callback(const uint8_t* peerMac,  const uint8_t *data, int data_len) {
  // Probably got a control packet
  if (data_len == sizeof(ControlPacket)) {
    ControlPacket packet = *(ControlPacket*)data;
  }
}

#endif // RECEIVER_H
