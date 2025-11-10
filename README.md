# 🌡️ Smart Thermostat — PSoC 6 + LVGL + Wi-Fi

A fully integrated **smart thermostat system** built on the **Infineon PSoC 6 Pioneer Kit (CY8CKIT-062S2-43012)** with a **TFT display**.  
This project demonstrates real-time UI updates using **LVGL**, **HTTP-based weather data synchronization**, and **local sensor feedback**.

---

## 🧠 Project Overview

The Smart Thermostat fetches real-time temperature, humidity, and weather data from an online server using **HTTPS (Secure HTTP Client)** and displays it on a **graphical UI** built with **LVGL**.  
It also synchronizes **clock and date** with the server’s time header and animates various UI components dynamically.

### Key Features:
- 🌐 **Wi-Fi Connectivity:** Secure HTTPS communication with weather API.  
- ☀️ **Weather Visualization:** Displays icons and conditions (clear, cloudy, rain, etc.).  
- ⏰ **Real-time Clock Sync:** Time & date auto-synced from HTTP server headers.  
- 🌈 **Animated UI:** Smooth LVGL transitions and blinking indicators.  
- ⚙️ **Modular Firmware:** Separate layers for UI, HTTP client, and synchronization logic.  

---

## 🧩 Hardware Setup

| Component | Description |
|------------|-------------|
| **MCU Board** | PSoC 6 Pioneer Kit (CY8CKIT-062S2-43012) |
| **Display** | TFT Display Shield (CY8CKIT-028-TFT) |
| **Network** | Wi-Fi Router (Avoid mobile hotspot for best results) |

---

## 🧰 Software Setup

1. Open the project in **ModusToolbox**.  
2. Update the Wi-Fi credentials in `secure_http_client.h`:

   ```c
   #define WIFI_SSID     "Your_SSID"
   #define WIFI_PASSWORD "Your_Password"

3. Build the project.
4. Program the board.