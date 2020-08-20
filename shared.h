// Some shared constants between controller.h and receiver.h
#ifndef __SHARED_H__
#define __SHARED_H__

// 4 flags to a byte to allow for on/off states, allowing the other end to encode them with more detail over PPM where necessary
// SBUS might be better, especially if there's a hardware serial connection we can use
#define TOTAL_FLAGS 4
#define FLAGS_SIZE 1
#define MAX_ANALOG_CHANNELS 4
#define MAX_DIGITAL_BYTES 2
#define MAX_SBUS_CHANNELS 18 // 16 analog, 2 digital
// internally, this is shaking out to 4 analog, 8 digital

struct ControlPacket {
  short channels[MAX_ANALOG_CHANNELS];
  byte digitalChannels[MAX_DIGITAL_BYTES]; // MAX_ANALOG_CHANNELS + 4 digital channels on ch15 ch16 ch17 ch18
};

enum ReceiverStatus {
  RECVR_OK = 00000000, // This packet has valid telemetry
  RECVR_SBUS_NOT_READY = 00000001, // This packet indicates Serial2.begin failed for some reason (hardware error?)
  RECVR_SBUS_DECODE_ERROR = 00000010, // This packet indicates Serial2.read calls have resulted in a malformed SBUS message?
  RECVR_SBUS_FRAGMENTED = 00000100, // not implemented
};

struct TelemetryPacket {
  byte status;
};

struct SBusMessage {
  byte header;
  byte channels[22]; // 16 11 bit channels
  byte postfix; // 0000 FAILSAFE FRAME_LOST CH18 CH17
  byte footer;
};

#endif // __SHARED_H__
