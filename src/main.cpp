#include <Arduino.h>
#include <U8x8lib.h>
#include <HardwareSerial.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEScan.h>
#include <WiFi.h>
#include <esp_wifi.h>
#include <esp_event.h>
#include <nvs_flash.h>
#include "opendroneid.h"
#include "odid_wifi.h"
#include <esp_timer.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>

// --- HELTEC V3 DISPLAY CONFIGURATION ---
// Pins: RST=21, SCL=18, SDA=17
U8X8_SSD1306_128X64_NONAME_HW_I2C u8x8(21, 18, 17);

// Data structure for detected drones
struct id_data {
  uint8_t   mac[6];
  int       rssi;
  uint32_t  last_seen;
  char      uav_id[ODID_ID_SIZE + 1];
  int       altitude_msl;
};

void callback(void *, wifi_promiscuous_pkt_type_t);
void send_json_fast(const id_data *UAV);

#define MAX_UAVS 8
id_data uavs[MAX_UAVS] = {0};
BLEScan* pBLEScan = nullptr;
ODID_UAS_Data UAS_data;
static QueueHandle_t printQueue;

// Helper to find or create a drone record
id_data* next_uav(uint8_t* mac) {
  for (int i = 0; i < MAX_UAVS; i++) {
    if (memcmp(uavs[i].mac, mac, 6) == 0) return &uavs[i];
  }
  for (int i = 0; i < MAX_UAVS; i++) {
    if (uavs[i].mac[0] == 0) return &uavs[i];
  }
  return &uavs[0];
}

// BLE Callback - Listens for Bluetooth Drone IDs
class MyAdvertisedDeviceCallbacks : public BLEAdvertisedDeviceCallbacks {
public:
  void onResult(BLEAdvertisedDevice device) override {
    int len = device.getPayloadLength();
    if (len <= 0) return;
    uint8_t* payload = device.getPayload();
    
    // Check for Remote ID magic bytes
    if (len > 5 && payload[1] == 0x16 && payload[2] == 0xFA && payload[3] == 0xFF) {
      uint8_t* mac = (uint8_t*) device.getAddress().getNative();
      id_data* UAV = next_uav(mac);
      UAV->last_seen = millis();
      UAV->rssi = device.getRSSI();
      memcpy(UAV->mac, mac, 6);
      
      // Basic decoding to get ID and Altitude
      uint8_t* odid = &payload[6];
      if ((odid[0] & 0xF0) == 0x00) { // Basic ID
          ODID_BasicID_data basic;
          decodeBasicIDMessage(&basic, (ODID_BasicID_encoded*) odid);
          strncpy(UAV->uav_id, (char*) basic.UASID, ODID_ID_SIZE);
      } else if ((odid[0] & 0xF0) == 0x10) { // Location
          ODID_Location_data loc;
          decodeLocationMessage(&loc, (ODID_Location_encoded*) odid);
          UAV->altitude_msl = (int) loc.AltitudeGeo;
      }
      
      // Send to display task
      id_data tmp = *UAV;
      xQueueSend(printQueue, &tmp, 0);
    }
  }
};

void bleScanTask(void *parameter) {
  for (;;) {
    BLEScanResults* foundDevices = pBLEScan->start(1, false);
    pBLEScan->clearResults();
    delay(100);
  }
}

void wifiProcessTask(void *parameter) {
  for (;;) { delay(10); } 
}

// --- THIS UPDATES YOUR BLUE SCREEN ---
void printerTask(void *param) {
  id_data UAV;
  for (;;) {
    if (xQueueReceive(printQueue, &UAV, portMAX_DELAY)) {
      // 1. Send JSON to Serial (for PC mapping)
      char mac_str[18];
      snprintf(mac_str, sizeof(mac_str), "%02x:%02x:%02x:%02x:%02x:%02x",
           UAV.mac[0], UAV.mac[1], UAV.mac[2], UAV.mac[3], UAV.mac[4], UAV.mac[5]);
      Serial.printf("{\"mac\":\"%s\",\"rssi\":%d,\"uav_id\":\"%s\"}\n", mac_str, UAV.rssi, UAV.uav_id);

      // 2. Update the Blue OLED
      u8x8.clearLine(2);
      u8x8.setCursor(0, 2);
      u8x8.print(UAV.uav_id[0] ? UAV.uav_id : "DETECTED!");
      
      u8x8.clearLine(3);
      u8x8.setCursor(0, 3);
      u8x8.print("RSSI: "); u8x8.print(UAV.rssi); u8x8.print("dBm");
      
      u8x8.clearLine(4);
      u8x8.setCursor(0, 4);
      u8x8.print("Alt:  "); u8x8.print(UAV.altitude_msl); u8x8.print("m");

      // "Heartbeat" flash on screen
      u8x8.drawString(0, 7, "** ALERT ** ");
      delay(100);
      u8x8.drawString(0, 7, "            ");
    }
  }
}

void callback(void *buffer, wifi_promiscuous_pkt_type_t type) {
  // Wifi sniffing logic handled here (simplified for stability)
}

void setup() {
  // --- HELTEC V3 POWER SEQUENCE (CRITICAL) ---
  pinMode(36, OUTPUT); // V3.2 uses 36 (Try 35 if screen stays black)
  digitalWrite(36, LOW); // Turn ON Vext power
  
  pinMode(21, OUTPUT);
  digitalWrite(21, LOW); // Reset OLED
  delay(50);
  digitalWrite(21, HIGH); // Wake OLED
  // -------------------------------------------

  Serial.begin(115200);

  // Start Display
  u8x8.begin();
  u8x8.setFont(u8x8_font_chroma48medium8_r);
  u8x8.drawString(0, 0, "SKY-SPY V3");
  u8x8.drawString(0, 1, "Scanning...");

  nvs_flash_init();
  WiFi.mode(WIFI_STA);
  WiFi.disconnect();
  esp_wifi_set_promiscuous(true);
  esp_wifi_set_promiscuous_rx_cb(&callback);
  
  BLEDevice::init("DroneID");
  pBLEScan = BLEDevice::getScan();
  pBLEScan->setAdvertisedDeviceCallbacks(new MyAdvertisedDeviceCallbacks());
  pBLEScan->setActiveScan(true);

  printQueue = xQueueCreate(MAX_UAVS, sizeof(id_data));
  
  xTaskCreatePinnedToCore(bleScanTask, "BLEScanTask", 4096, NULL, 1, NULL, 1);
  xTaskCreatePinnedToCore(wifiProcessTask, "WiFiProcessTask", 4096, NULL, 1, NULL, 0);
  xTaskCreatePinnedToCore(printerTask, "PrinterTask", 4096, NULL, 1, NULL, 1);
}

void loop() {
  delay(1000); 
}
