# ultrasonic-esp32-range-finder
a cool project for using a 1.8in tft display, an esp32, and a ultrasonic sensor to make them into a system for finding range of objects.
feel free to edit or use it, it's open for all.


# ESP32 Ultrasonic Radar Pinout Map

## 📟 TFT Display (ST7735) → ESP32
```
TFT Pin    ESP32 Pin
━━━━━━━    ━━━━━━━━━
GND    →   GND
VCC    →   3.3V
SCL    →   GPIO 18
SDA    →   GPIO 23
RES    →   GPIO 4
DC     →   GPIO 2
CS     →   GPIO 5
BL     →   3.3V
```

## 📡 HC-SR04 Sensor → ESP32
```
Sensor Pin    ESP32 Pin
━━━━━━━━━━    ━━━━━━━━━
VCC       →   3.3V
GND       →   GND
Trig      →   GPIO 15
Echo      →   GPIO 19
```

## 🔌 Power Connections
```
Component    Power Source
━━━━━━━━━    ━━━━━━━━━━━━
ESP32     →  USB 5V
Display   →  3.3V (from ESP32)
Sensor    →  3.3V (from ESP32)
All GND   →  Common Ground
```

## 📋 Quick Reference
- **Display**: Uses SPI communication (pins 18, 23, 2, 4, 5)
- **Sensor**: Uses digital pins (15, 19) 
- **Power**: Everything runs on 3.3V except USB input
- **Upload**: Hold BOOT button while uploading code
