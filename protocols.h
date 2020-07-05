// Protocols that exist
//#define FC_ESPNOW    
//#define FC_ESPNOW_ENCRYPTION_KEY 
// Protocols that haven't been implemented because I'm a lazy tosser
//#define FC_ESP32LR   // Maximum range is likely lossy at 1km LOS

// shared constants
uint16_t wifiChannel = 1;

#ifdef ESP32
  #include <WiFi.h>
  #include <esp_wifi.h>

  void configure_external_antenna() {
    wifi_ant_config_t cfg = {
        .rx_ant_mode = WIFI_ANT_MODE_AUTO, // auto/ant0/ant1
        .rx_ant_default = WIFI_ANT_ANT1, // default ant to use when auto
        .tx_ant_mode = WIFI_ANT_MODE_AUTO, // auto ant0/ant1 when rx_ant_mode configured
        .enabled_ant0 = 1,
        .enabled_ant1 = 2,
    };
    esp_wifi_set_ant(&cfg);
    wifi_ant_gpio_config_t gpioCfg;
    memset(&gpioCfg, 0, sizeof(wifi_ant_gpio_config_t));
    gpioCfg.gpio_cfg[0].gpio_select = 1;
    gpioCfg.gpio_cfg[0].gpio_num = 27; // try and pick a less valuable ADC, like the one used by WiFi already
    esp_wifi_set_ant_gpio(&gpioCfg);
  }
#else
  #include <ESP83...WiFi.h>
#endif // ESP32

void printMac() {
  if (Serial) {
    Serial.printf("MAC Address: ");
    Serial.println(WiFi.macAddress());
  }
}

// Protocol methods that must be implemented by any protocol that includes this file
bool init_radio();
void loop_radio();
void send_radio(const uint8_t* data, int data_len);

// Protocol methods that must be implemented by any controller or receiver (even if they do nothing)
void send_callback(const uint8_t* peerMac, bool sent);
void recv_callback(const uint8_t* peerMac,  const uint8_t *data, int data_len);

// Performance timers
unsigned long lastPacketSent = 0; // last time a packet made it to the peer
short packetsPerSecond = 0; // packets sent every second
short lastPacketsPerSecond = 0; // last packets sent every second, kept for display
short packetsFailedPerSecond = 0; // packets failed every second
unsigned long packetsPerSecondTimer = 0; // timer used by protocols to track a second passing
byte rssi = 0; // difficult to capture
