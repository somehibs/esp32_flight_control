// This entire class is custom to my setup. While you might be able to follow my tutorial, it's likely easiest to start from scratch with this file.

#include "tft.h"
#include "shared.h"

class Controller {
public:
  void init() {
    updateTft = true;
    pktComplete = true;
    tft.init();
    //
  }

  void loop() {
    // Send data to the controller, if ok
    ControlPacket packet;
    // Check for new data in any buffers, but don't take too long to render!
    if (pktComplete) {
      pktComplete = false;
      send_radio((const uint8_t*)&packet, sizeof(ControlPacket));
      tft.updatePackets();
    }
    if (updateTft) {
      //tft.updatePackets();
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
  ctl.init();
}

void loop_controller() {
  ctl.loop();
  delay(1);
}

void send_callback(const uint8_t* peerMac, bool sent) {
  ctl.sent(peerMac, sent);
}
void recv_callback(const uint8_t* peerMac,  const uint8_t *data, int data_len) {
  ctl.recv(peerMac, data, data_len);
}
