# OpenCirclz - ObserverTool

A smart, pocket-sized timing and utility device for Roundnet observers, built on the ESP32 platform. It features timers for every situation with haptic feedback, a coin toss generator, seamless Scoreboard synchronization, and built-in OTA (Over-The-Air) update capabilities.

## ✨ Features
* **Smart Timers:** Pre-configured programs (Serve, Timeout, Break, Medicals) with visual countdowns.
* **Scoreboard Integration:** Wirelessly broadcasts active timers to the OpenCirclz Scoreboard Pro via ESP-NOW for live stream or crowd overlays.
* **Haptic Feedback:** Custom vibration profiles for warnings, alarms, and interactions for a look-free interaction.
* **Power Management:** Auto-dimming and sleep modes for maximum battery life.
* **Coin Toss:** Built-in random heads/tails generator.
* **OTA Updates:** Secret service mode to flash new firmware via Wi-Fi without a cable.
* **Easter Egg:** Hidden "Flappy Dot" mini-game.

## 🛠️ Hardware Requirements
* **Microcontroller:** ESP32-C3 Super Mini [HERE](https://de.aliexpress.com/item/1005008796274949.html)
* **Display:** 0.96" OLED I2C (128x64, SSD1306) [HERE](https://de.aliexpress.com/item/1005007084725556.html)
* **Inputs:** 2x Push Buttons (Start & Mode) (12*11mm opening size) [HERE](https://de.aliexpress.com/item/1005009402689922.html)
* **Vibration:** QYF-740 Vibration Motor Module (Built-in driver, operates at 3.0-5.3VDC) [HERE](https://aliexpress.com/item/1005009127931477.html)
* **Battery:** Small 502030 3.7V LiPo Battery [HERE](https://de.aliexpress.com/item/1005006584143607.html)
* **Power Switch**: Simple ON/OFF Power Switch (19*13mm opening size) [HERE](https://aliexpress.com/item/1005008276122557.html)
* **Charge Controller:** Mini Type-C LiPo Charge/Discharge Module *(Specs: 18x12mm, 5V to 4.2V, set to 0.1A charge current, integrated over-discharge protection)* [HERE](https://de.aliexpress.com/item/1005010413122149.html)

### Pin Configuration
| Component         | ESP32 Pin |
|-------------------|-----------|
| I2C SCL (OLED)    | GPIO 6    |
| I2C SDA (OLED)    | GPIO 7    |
| START Button      | GPIO 3    |
| MODE Button       | GPIO 4    |
| Vibration Motor   | GPIO 10   |

*(Note: Buttons are configured as `INPUT_PULLUP`, so connect them between the GPIO and GND).*

## 🧊 3D Printed Enclosure
To complete your ObserverTool, a custom-designed 3D printable case is available. You can find all the necessary `.stl` files directly in the [enclosure](enclosure) folder. 

**Assembly & Design:**
* **Highly Maintenance-Friendly:** The entire enclosure is designed without the need for any glue. 
* **Hardware:** Everything is assembled using **1.7 x 5 mm screws**.
* **Smart Mounting:** The OLED display, ESP32, and battery are securely held in place using custom 3D-printed pressure plates. This makes future repairs or battery replacements incredibly easy.

**Recommended Print Settings:**
* **Material:** PLA, PETG
* **Infill:** 15-20% is usually sufficient.

## 💻 Software Dependencies
To compile this project in the Arduino IDE or PlatformIO, you need the following libraries:
* `Adafruit GFX Library`
* `Adafruit SSD1306`

The ESP32 core libraries (`WiFi`, `esp_now`, `esp_wifi`, `WebServer`, `Update`, `DNSServer`, `Preferences`) are included by default in the ESP32 board package.

## 📥 Installation & Flashing Guide (Arduino IDE)

Follow these steps to compile and flash the firmware to your ESP32 for the first time:

### 1. Setup Arduino IDE & ESP32 Core
1. Download and install the latest [Arduino IDE](https://www.arduino.cc/en/software).
2. Open the IDE, go to **File > Preferences**, and paste the following URL into the *Additional Boards Manager URLs* field:
   `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
3. Go to **Tools > Board > Boards Manager**, search for **esp32**, and install the package by *Espressif Systems*.

### 2. Install Required Libraries
Go to **Sketch > Include Library > Manage Libraries** and search for the following libraries to install them:
* `Adafruit GFX Library` (by Adafruit)
* `Adafruit SSD1306` (by Adafruit)

### 3. Configure Board Settings (⚠️ CRITICAL)
Connect your ESP32 to your computer via USB. Go to the **Tools** menu and configure the following settings:
* **Board:** Select your specific ESP32 board (e.g., `ESP32 Dev Module`, `Adafruit ESP32 Feather`, etc.).
* **Partition Scheme:** Select **Minimal SPIFFS (1.9MB APP with OTA/128KB SPIFFS)**. 
  *(Note: This step is absolutely mandatory! If you skip this, the OTA-Update feature will not have enough memory to function and the board will crash).*
* **Port:** Select the COM port your ESP32 is connected to (e.g., `COM3` on Windows or `/dev/cu.usbserial-...` on Mac).

### 4. Flash the Firmware
1. Open the `ObserverTool.ino` file in the Arduino IDE.
2. Click the **Upload** button (the right-pointing arrow at the top left).
3. *Troubleshooting tip: If the IDE gets stuck at "Connecting...", press and hold the physical `BOOT` button on your ESP32 board until the flashing process starts.*

## 📦 Quick Update (Precompiled Binary)
If you already have a version of the ObserverTool running and just want to update to the latest firmware without compiling the code yourself, you can use the precompiled `.bin` file.

1. Go to the [Releases](../../releases) page of this repository.
2. Download the latest `ObserverTool_vX.X.bin` file from the "Assets" section.
3. Power on your ObserverTool while holding both the **Mode + Start** buttons to enter OTA Service Mode.
4. Connect to the Wi-Fi network `ObserverTool`.
5. Open your browser and navigate to `http://192.168.4.1` (or wait for the captive portal).
6. Upload the downloaded `.bin` file and hit "Flash Update". The device will restart automatically.

## 🚀 Usage & Controls
* **Normal Operation:**
  * Press **Mode** to cycle through the timer programs.
  * Press **Start** to start or stop the selected timer.
  * *Coin Toss:* Select "Coin Toss" and press Start to generate a random result.
* **Wake Up:** Press any button to wake the device from sleep or dim mode.

### Secret Boot Modes
Hold specific buttons **while powering on** the device to access special modes:
* **Settings Menu:** Hold `START`
  * Toggle Scoreboard ESP-NOW Sync `ON` or `OFF`. Press `MODE` to save and exit.
* **OTA Service Mode:** Hold `MODE` + `START`
  * Connect to the Wi-Fi AP `ObserverTool`.
  * A captive portal will appear (or navigate to `http://192.168.4.1`).
  * Upload your new `.bin` firmware file.
* **Flappy Dot Game:** Hold `MODE`
  * Press `START` to jump. Press `MODE` to exit back to the main tool.

## 📄 License
This project is licensed under the Creative Commons Attribution-NC-SA 4.0 - see the [LICENSE](LICENSE) file for details.
