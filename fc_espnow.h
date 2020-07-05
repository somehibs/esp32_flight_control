#define FC_ESPNOW
#include "protocols.h"

#ifdef FC_ESPNOW_ENCRYPTION_KEY
#define FC_ESPNOW_ENCRYPTION // Obviously you wanted this
#endif // FC_ESPNOW_ENCRYPTION_KEY

#ifndef FC_ESPNOW__H
#define FC_ESPNOW__H
#include "esp_now.h"

void send_callback_proxy(const uint8_t* peerMac, esp_now_send_status_t status) {
  bool ok = false;
  unsigned long ms = millis();
  if (ms > 1000 && packetsPerSecondTimer < ms - 1000) {
#ifdef STATS
    Serial.print(packetsPerSecond,DEC);
    Serial.print("pkt/s ");
    Serial.print(packetsFailedPerSecond);
    Serial.println("f/s");
#endif // DEBUG
    packetsPerSecondTimer = ms;
    lastPacketsPerSecond = packetsPerSecond;
    packetsPerSecond = 0;
    packetsFailedPerSecond = 0;
  }
  if (status == ESP_NOW_SEND_SUCCESS) {
    ok = true;
    packetsPerSecond += 1;
  } else {
    packetsFailedPerSecond += 1;
  }
  send_callback(peerMac, ok);
}

bool init_radio() {
#ifdef ESP32
  configure_external_antenna();
#endif // ESP32
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  if (esp_now_init() != ESP_OK) {
    Serial.println("Failed to start ESP_NOW");
    return false;
  }
  //esp_wifi_set_ps(WIFI_PS_NONE); // High performance/drain mode
  //esp_wifi_set_bandwidth(HT20); // Since we expect 
  esp_now_register_send_cb(send_callback_proxy);
  esp_now_register_recv_cb(recv_callback);
  //esp_now_set_self_role( This method used to set a protocol level byte, which they've wisely ditched in later versions as the protocol is 2way anyway
  return true;
}

void loop_radio() {
  // N/A, there's a WiFi stack that does this for us
}

void send_radio(const uint8_t* data, int data_len) {
  esp_now_send(NULL, data, data_len);
}

bool set_peer_mac(uint8_t* macAddress) {
  esp_now_peer_info peer;
  memset(&peer, 0, sizeof(esp_now_peer_info));
  memcpy(peer.peer_addr, macAddress, sizeof(uint8_t)*6);
  int result = esp_now_add_peer(&peer);
  if (result != ESP_OK) {
    return false;
  }
  Serial.println("Added peer OK");
  return true;
}














// test methods
#ifdef TEST
void espNowTestPrimary();
void espNowTestSecondary();

unsigned long sendTime = 0;
unsigned long lastTick = 0;

void testSendCallback(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (Serial) {
    if (status != ESP_NOW_SEND_SUCCESS) {
      Serial.print(F("SEND "));
      if (status == ESP_NOW_SEND_SUCCESS) {
        Serial.print(F(" * OK"));
      } else {
        Serial.print(F(" ! FAIL"));
      }
      Serial.print(F("(addr: "));
      Serial.print(*((uint32_t*)mac_addr), HEX);
      Serial.print(F(") TIME: "));
      Serial.print((millis()-sendTime), DEC);
      Serial.println("");
    }
  }
  sendTime = 0;
  perSecSendCounter += 1;
}

void testRecvCallback(const uint8_t *mac_addr, const uint8_t *data, int data_len) {
  if (Serial) {
    Serial.print(F("RECV "));
    Serial.print(F("addr: "));
    Serial.print(*((uint32_t*)mac_addr), HEX);
    Serial.print(F(") datalen: "));
    Serial.println(""+data_len);
  }
}

struct TestStruct {
  unsigned long sendTime;
  unsigned long sequence;
  unsigned long sequence2;
  unsigned long sequence3;
  unsigned long sequence4;
};

// the maximum rate seems to be about 35-40 messages per second if you wait for error callbacks
// this is pretty good, but makes me wonder how fast it gets when there's someone listening to reply
void espNowTestPrimary() {
  unsigned long lastMs = 0;
  unsigned long seq = 0;
  esp_now_register_send_cb(testSendCallback);
  TestStruct data;
  for (;;) {
    // Emit a prefix, the current millis() and a sequence number 
    sendTime = millis();
    if (sendTime-lastTick > 1000) {
      Serial.print("Send per second: " );
      //Serial.print(sendTime-lastTick, DEC);
      //Serial.print(" count: " );
      Serial.print(perSecSendCounter, DEC);
      Serial.println("");
      lastTick = sendTime;
      perSecSendCounter = 0;
    }
    data.sendTime = millis();
    data.sequence = seq;
    short err = esp_now_send(NULL, ((uint8_t*)&data), sizeof(TestStruct));
    if (err != 0) {
      Serial.println("FAILED TO SEND ESPNOW");
    }
    seq += 1;
    // wait for send to complete, maximum lag of 1ms
    while (sendTime != 0) {
      delay(500);
    }
  }
}
void espNowTestSecondary() {
  esp_now_register_recv_cb(testRecvCallback);
  for (;;) {
    // Look for a request
    // Compute the time between this and the last request, send both back
    // Report that it happened over serial
    delay(500);
  }
}

#endif //DEBUG


#endif // FC_ESPNOW__H
