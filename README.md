# Sky-Spy V3: Heltec WiFi LoRa 32 (V3) Port

This is a specialized fork of [Sky-Spy](https://github.com/colonelpanichacks/Sky-Spy) adapted to run natively on the **Heltec WiFi LoRa 32 (V3)**.

While the original Sky-Spy was built for the Seeed XIAO S3, this version includes the specific power management and display drivers needed to make the Heltec's onboard **Blue OLED** work as a standalone drone radar.

## ⚡ What's Different?
* **OLED HUD:** Integrated **U8g2** support to display Drone ID, RSSI, and Altitude directly on the Heltec's 0.96" screen.
* **Power Management:** Handles the Heltec V3's unique `Vext` power gating (GPIO 36/35) and OLED Reset (GPIO 21) to ensure the screen wakes up correctly.
* **Hardware Config:** Pre-configured `platformio.ini` for the ESP32-S3 chip used in the V3.

## 🛠 Hardware Required
* **Board:** [Heltec WiFi LoRa 32 V3](https://heltec.org/project/wifi-lora-32-v3/) (V3.1 or V3.2)
* **Antenna:** You **MUST** have the antenna attached to protect the RF chip, even for passive scanning.
* **Battery (Optional):** Supports standard 1.25mm JST LiPo batteries for handheld use.

## 🚀 Installation
1.  **Clone this repository:**
    ```bash
    git clone [https://github.com/michaelgilbert1208/Sky-Spy-Heltec.git](https://github.com/michaelgilbert1208/Sky-Spy-Heltec.git)
    cd Sky-Spy-Heltec
    ```
2.  **Open in VS Code** with the **PlatformIO** extension installed.
3.  **Build & Upload:**
    * Connect your Heltec V3 via USB-C.
    * Click the **PlatformIO Icon** (Alien face) on the left sidebar.
    * Select **heltec_wifi_lora_32_V3** -> **Upload**.
    * *Note: The first build will automatically download the necessary display libraries.*

## 📟 How to Use
Once flashed, the board acts as a passive "Drone Radar."

* **Scanning:** The screen will display "Scanning..." while listening for WiFi Beacon and Bluetooth 5.0 (Long Range) Remote ID packets.
* **Detection:** When a drone is found, the screen updates with:
    * **ID:** The Drone's unique serial number (e.g., `1581F...`).
    * **RSSI:** Signal strength in dBm. (Closer to -40 is nearby; -90 is far away).
    * **Alt:** The drone's altitude in meters (if broadcast).
* **Direction Finding:** Monitor the RSSI number. Turn your body 360°. When the signal drops significantly (the number gets lower, e.g., -95), the drone is likely **behind** you (blocked by your body).

## 🧩 Pinout Reference
If you are modifying the code further, these are the critical pins for the Heltec V3:

| Component | GPIO Pin | Notes |
| :--- | :--- | :--- |
| **OLED SDA** | `17` | I2C Data |
| **OLED SCL** | `18` | I2C Clock |
| **OLED RST** | `21` | **High** to wake, **Low** to reset |
| **Vext Power** | `36` | Set **Low** to power the screen (Use `35` for older V3.0 boards) |
| **User Button**| `0` | "PRG" Button |
| **LED** | `35` | Onboard White LED |

## ⚖️ License & Credits
* Based on **[Sky-Spy](https://github.com/colonelpanichacks/Sky-Spy)** by Colonel Panic.
* Core logic from **[ArduRemoteID](https://github.com/ArduPilot/ArduRemoteID)**.
* Licensed under **GNU GPL v3.0**.
* *Disclaimer: This tool is for educational purposes and passive monitoring only.*
