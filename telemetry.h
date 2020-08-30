
#ifndef __TELEMETRY_H__
#define __TELEMETRY_H__

#include "shared.h"

class SmartPort {
public:
  void init(short rxPort, short txPort) {
    this->rxPort = rxPort;
    this->txPort = txPort;
  }

  void maintain() {
    // Make sure the RX bus is serviced
    if (Serial1) {
      while (Serial2.available()) {
        // Figure out how to consume telemetry data. For now, suck it up to make sure no one gets full buffers.
        Serial2.read();
      }
    } else {
      init(rxPort, txPort);
    }
  }
private:
  byte rxPort, txPort;
};


#endif // __TELEMETRY_H__
