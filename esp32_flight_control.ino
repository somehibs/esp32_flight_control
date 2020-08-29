// Here's where you configure everything 
#define ESP32
//#define CONTROLLER // RECEIVER is implicit when CONTROLLER is undefined, i.e. the else in ifdef blocks
#define TFT_ENABLE // Controller has a TFT module for displaying telemetry
#define TELEMETRY_ENABLE // Telemetry is sent back over ESP NOW occasionally
//#define TEST // TEST defines some functions that should only be called when testing this software
#define STATS // STATS enables statistic reporting along Serial and is only good for testing protocols, disable otherwise

// Include a protocol to use it
// All protocols will implement the prototype methods in protocols.h
// All protocols will also define a constant if you need to do protocol specific calls but
// avoid compiling them into your binary
//#define ESPNOW_ENCRYPTION_KEY // ESP NOW ENCRYPTION - Likely slower or more limited packet sizes
//#include "fc_esplr.h" // ESP32 LR - special p2p wifi mode that works over 1km. my first tests are unlikely to need this.
#include "fc_espnow.h" // ESP NOW - Maximum range is 160m LOS

// Controller mappings are configured in controller.h
#ifdef CONTROLLER
#include "controller.h"
void init_receiver(){}
bool controller = true;
//uint8_t peerMacAddress[8] = {0x24,0x0a,0xc4,0x62,0x33,0x88}; // TFT?
//uint8_t peerMacAddress[8] = {0xFC,0xF5,0xC4,0x2F,0x3D,0x14}; // RX address
uint8_t peerMacAddress[8] = {0x24,0x0A,0xC4,0x62,0x17,0x0C};
#else
#include "receiver.h"
void init_controller(){}
bool controller = false;
//uint8_t peerMacAddress[8] = {0x24,0x62,0xAB,0xF9,0x9D,0xA0}; // TFT MAC address
uint8_t peerMacAddress[8] = {0x24,0x0A,0xC4,0x62,0x33,0x88}; // Controller MAC address
// SBUS pins
#define TX_PIN 26
#define RX_PIN 27
#endif // CONTROLLER



void setup() {
  Serial.begin(115200);
  if (!init_radio()) {
    Serial.println("Failed to start radio");
    return;
  }
  printMac();
  set_peer_mac(peerMacAddress);
  
#ifdef CONTROLLER
    if(Serial)Serial.println("ROLE:CONTROLLER");
    init_controller();
#else
    if(Serial)Serial.println("ROLE:RECEIVER");
    init_receiver(TX_PIN, RX_PIN);
#endif //CONTROLLER
}



void loop() {
#ifdef TEST
  if (controller) {
    espNowTestPrimary();
  } else {
    espNowTestSecondary();
  }
#endif // TEST
  
#ifdef CONTROLLER
  loop_controller();
#else
  loop_receiver();
#endif // CONTROLLER

  loop_radio();
}


/*
#include <esp_wifi.h> // change the mac address
int err = esp_wifi_set_mac(ESP_IF_WIFI_STA, macAddress);
  Serial.print("mac set err: ");
  if (err == ESP_ERR_WIFI_NOT_INIT) {
    Serial.print("wifi not init ");
  } else if (err == ESP_ERR_INVALID_ARG) {
    Serial.print("invalid arg ");
  } else if (err == ESP_ERR_WIFI_MAC) {
    Serial.print("invalid mac ");
  } else if (err == ESP_ERR_WIFI_MODE) {
    Serial.print("invalid wifi mode ");
  } else if (err == ESP_ERR_WIFI_IF) {
    Serial.print("invalid wifi if ");
  }
  Serial.print(err, DEC);
  Serial.println("");*/
