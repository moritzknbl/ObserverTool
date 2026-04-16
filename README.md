# OpenCirclz - ObserverTool

A smart, pocket-sized timing and utility device for Roundnet observers, built on the ESP32 platform. It features timers for every situation with haptic feedback, a coin toss generator, and built-in OTA (Over-The-Air) update capabilities.

## ✨ Features
* **Smart Timers:** Pre-configured programs (Serve, Timeout, Break, Medicals) with visual countdowns.
* **Haptic Feedback:** Custom vibration profiles for warnings, alarms, and interactions for a look free interaction
* **Power Management:** Auto-dimming and deep sleep mode for maximum battery life.
* **Coin Toss:** Built-in random heads/tails generator.
* **OTA Updates:** Secret service mode to flash new firmware via Wi-Fi without a cable.
* **Easter Egg:** Hidden "Flappy Bird" mini-game.

## 🛠️ Hardware Requirements
* **Microcontroller:** ESP32-C3 Super Mini
* **Display:** 0.96" OLED I2C (128x64, SSD1306)
* **Inputs:** 2x Push Buttons (Action & Mode)
* **Output:** 1x Small Vibration Motor (connected via transistor/MOSFET recommended)

### Pin Configuration
| Component         | ESP32 Pin |
|-------------------|-----------|
| I2C SCL (OLED)    | GPIO 6    |
| I2C SDA (OLED)    | GPIO 7    |
| START Button     | GPIO 3    |
| MODE Button       | GPIO 4    |
| Vibration Motor Module   | GPIO 10   |

*(Note: Buttons are configured as `INPUT_PULLUP`, so connect them between the GPIO and GND).*

## 💻 Software Dependencies
To compile this project in the Arduino IDE or PlatformIO, you need the following libraries:
* `Adafruit GFX Library`
* `Adafruit SSD1306`

The ESP32 core libraries (`WiFi`, `WebServer`, `Update`, `DNSServer`, `Preferences`) are included by default in the ESP32 board package.

## 🚀 Usage & Controls
* **Normal Operation:**
  * Press **Mode** to cycle through the timer programs.
  * Press **Action** to start or stop the selected timer.
  * *Coin Toss:* Select "Coin Toss" and press Action to generate a random result.
* **Wake Up:** Press any button to wake the device from sleep or dim mode.

### Secret Boot Modes
Hold specific buttons **while powering on** the device to access special modes:
* **OTA Service Mode:** Hold `MODE` + `START`
  * Connect to the Wi-Fi AP `ObserverTool`.
  * A captive portal will appear (or navigate to `http://192.168.4.1`).
  * Upload your new `.bin` firmware file.
* **Flappy Dot Game:** Hold `MODE`
  * Press `START` to jump. Press `MODE` to exit back to the main tool.

## 📄 License
This project is licensed under the MIT License - see the [LICENSE](LICENSE) file for details.
