# 🐟 Smart Aquarium Guardian System (ESP32)

A complete IoT-based **Smart Aquarium Monitoring & Feeding System** built using ESP32.
This system monitors **temperature**, **water level**, and provides **automatic + manual feeding**, along with a **web dashboard UI**.

---

## 🚀 Features

* 🌡️ Real-time **Water Temperature Monitoring** (DS18B20)
* 💧 **Water Level Detection** using Ultrasonic Sensor
* 🍽️ **Automatic Fish Feeding** (RTC-based scheduling)
* 🎮 **Manual Feeding** (Button + Web UI)
* 📊 Beautiful **Web Dashboard (ESP32 Hosted)**
* 🔐 **Login System** (Username & Password)
* ⚠️ Smart **Alert System**

  * Temperature HIGH / LOW
  * Water Level HIGH / LOW
* 🔔 **Buzzer + LED Alerts**
* 🧠 **EEPROM (Preferences) Storage**
* 📝 **Feed Logs with Timestamp**
* ⏱️ Real-Time Clock (RTC DS3231)

---

## 🧰 Hardware Used

* ESP32
* DS18B20 Temperature Sensor
* Ultrasonic Sensor (HC-SR04)
* Servo Motor (Fish Feeder)
* RTC Module (DS3231)
* Push Button
* Buzzer
* LED

---

## 🔌 Pin Configuration

| Component       | ESP32 Pin |
| --------------- | --------- |
| DS18B20         | GPIO 4    |
| Ultrasonic TRIG | GPIO 12   |
| Ultrasonic ECHO | GPIO 13   |
| Servo Motor     | GPIO 18   |
| Button          | GPIO 27   |
| Buzzer          | GPIO 19   |
| LED             | GPIO 25   |

---

## 🌐 WiFi Configuration

```cpp
const char* ssid = "SmartAquarium";
const char* password = "12345678";
```

ESP32 creates a **WiFi Access Point**

👉 Connect to:

* SSID: `SmartAquarium`
* Password: `12345678`

Then open browser:

```
http://192.168.4.1
```

---

## 🔐 Login Credentials

* Username: `admin`
* Password: `1234`

---

## 📊 Dashboard Features

* Live Temperature Display
* Water Level Percentage
* Real-Time Clock
* Feed Count
* Feeding Button
* Schedule Setup (2 timers)
* Alert Threshold Settings
* Activity Logs

---

## ⚙️ Feeding System

### 🔹 Manual Feeding

* Button press
* Web dashboard "FEED NOW"

### 🔹 Automatic Feeding

* Uses RTC time
* Two schedules:

  * Schedule 1 (h1:m1:s1)
  * Schedule 2 (h2:m2:s2)

### 🔹 Cooldown Protection

* 20 seconds delay between feeds

---

## ⚠️ Alert System

Triggers when:

* Temperature < minTemp
* Temperature > maxTemp
* Water Level < minLevel
* Water Level > maxLevel

### Alert Actions:

* 🔴 LED ON
* 🔊 Buzzer Beeping (non-blocking)
* 🚨 Dashboard Alert Banner

---

## 💾 Data Storage

Using ESP32 Preferences:

* Feed Count
* Feeding Logs
* Schedule Times
* Alert Thresholds

---

## 🧠 Key Functions

* `readSensors()` → Reads temp & water level
* `checkAlerts()` → Checks thresholds & triggers alert
* `feedFish()` → Controls servo feeding
* `saveLog()` → Stores feed history
* `handleSensor()` → API for live sensor data
* `handleSave()` → Save user settings

---

## 📁 APIs

| Endpoint  | Description   |
| --------- | ------------- |
| `/`       | Dashboard     |
| `/login`  | Login Page    |
| `/sensor` | Sensor Data   |
| `/time`   | RTC Time      |
| `/data`   | Logs & Count  |
| `/feed`   | Trigger Feed  |
| `/save`   | Save Settings |

---

## 🛠️ How to Upload

1. Open Arduino IDE
2. Select Board: **ESP32**
3. Install Libraries:

   * WiFi
   * WebServer
   * Preferences
   * RTClib
   * ESP32Servo
   * OneWire
   * DallasTemperature
4. Upload Code

---

## 🧪 Future Improvements

* 📱 Mobile App Integration
* ☁️ Cloud Data Logging (Firebase / MQTT)
* 📊 Graph Visualization
* 🤖 AI-based Fish Feeding Prediction
* 🔋 Battery Backup System

---

## 👨‍💻 Author

Developed as a **Smart IoT Aquarium Project** using ESP32.

---

## ⭐ If you like this project

Give it a ⭐ on GitHub and share it!

---
