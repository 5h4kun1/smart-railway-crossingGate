# 🚂 Smart Railway Crossing Control System

A comprehensive IoT-based railway crossing automation system that uses dual IR sensors for train detection and provides both automatic and manual control modes with web-based monitoring.

## 📋 Table of Contents
- [Features](#features)
- [System Architecture](#system-architecture)
- [Hardware Requirements](#hardware-requirements)
- [Pin Configuration](#pin-configuration)
- [Installation](#installation)
- [Usage](#usage)
- [Web Interface](#web-interface)
- [Serial Communication Protocol](#serial-communication-protocol)
- [Safety Features](#safety-features)
- [Troubleshooting](#troubleshooting)
- [Contributing](#contributing)
- [License](#license)

## ✨ Features

### Core Functionality
- **Dual IR Sensor Detection**: Uses two IR sensors placed on both sides of the track for reliable train detection
- **Automatic Gate Control**: Servo-controlled gate that automatically opens/closes based on train presence
- **Bi-directional Train Detection**: Detects trains approaching from either direction
- **Manual Override**: Physical buttons and web interface for manual control
- **Real-time Web Monitoring**: WiFi-enabled web dashboard for remote monitoring and control

### Smart Features
- **Train Direction Detection**: Identifies whether train is approaching or exiting
- **Delayed Opening**: 3-second safety delay before opening gate after train passes
- **Multiple Control Modes**: Automatic and manual operation modes
- **Visual & Audio Alerts**: LED indicators and buzzer for status notifications
- **LCD Display**: Real-time status display with custom characters

### Safety & Reliability
- **Fail-safe Operation**: System defaults to closed position for safety
- **Redundant Sensors**: Dual sensor setup for improved reliability
- **Status Monitoring**: Continuous system health monitoring
- **Error Handling**: Comprehensive error detection and reporting

## 🏗️ System Architecture

```
┌─────────────────┐    WiFi     ┌──────────────────┐
│   Web Browser   │◄───────────►│    ESP32-CAM     │
│   Dashboard     │             │  (WiFi AP Mode)  │
└─────────────────┘             └──────────────────┘
                                         │
                                   Serial (9600 baud)
                                         │
                                         ▼
┌─────────────────────────────────────────────────────────┐
│                Arduino Nano                             │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────┐  │
│  │ IR Sensors  │  │ Servo Motor  │  │ LCD Display   │  │
│  │   (x2)      │  │   Control    │  │   (16x2 I2C)  │  │
│  └─────────────┘  └──────────────┘  └───────────────┘  │
│  ┌─────────────┐  ┌──────────────┐  ┌───────────────┐  │
│  │ Push Buttons│  │   Buzzer     │  │   LED Status  │  │
│  │   (x3)      │  │   Alerts     │  │  Indicators   │  │
│  └─────────────┘  └──────────────┘  └───────────────┘  │
└─────────────────────────────────────────────────────────┘
```

## 🔧 Hardware Requirements

### Main Components
- **ESP32-CAM Module** (used as ESP32 for WiFi connectivity)
- **Arduino Nano** (main control unit)
- **Servo Motor** (gate mechanism)
- **2x IR Obstacle Sensors** (train detection)
- **16x2 I2C LCD Display** (status display)
- **Buzzer** (audio alerts)
- **Push Buttons** (3x for manual control)
- **LEDs** (status indicators: Red, Green, Built-in)

### Supporting Components
- Jumper wires
- Breadboard or PCB
- Power supply (5V for Arduino, 3.3V for ESP32)
- Resistors (for LED current limiting)
- Pull-up resistors (if not using internal pull-ups)

## 📌 Pin Configuration

### ESP32-CAM Connections
```
GPIO 1  (TX) ──► Arduino Nano RX (Pin 3)
GPIO 3  (RX) ◄── Arduino Nano TX (Pin 1)
GPIO 2       ──► Status LED
```

### Arduino Nano Connections
```
Digital Pins:
Pin 2  ──► IR Sensor 1 (Approach Detection)
Pin 3  ──► IR Sensor 2 (Exit Detection)
Pin 4  ──► Manual Open Button
Pin 5  ──► Manual Close Button
Pin 6  ──► Auto Mode Button
Pin 8  ──► Buzzer
Pin 9  ──► Servo Motor Signal
Pin 10 ──► Red LED (Gate Closed)
Pin 11 ──► Green LED (Gate Open)
Pin 13 ──► Status LED (Built-in)

I2C Pins:
A4 (SDA) ──► LCD SDA
A5 (SCL) ──► LCD SCL

Power:
5V   ──► Servo Motor, LCD VCC
GND  ──► Common Ground
3.3V ──► IR Sensors VCC
```

## 🚀 Installation

### 1. Hardware Setup
1. Wire all components according to the pin configuration
2. Ensure proper power supply connections
3. Test individual components before final assembly
4. Mount IR sensors on both sides of the track

### 2. Software Installation

#### Arduino IDE Setup
```bash
# Install required libraries:
# - Servo (built-in)
# - Wire (built-in)
# - LiquidCrystal_I2C
# - WiFi (ESP32)
# - WebServer (ESP32)
```

#### ESP32 Board Setup
1. Add ESP32 board URL to Arduino IDE preferences:
   ```
   https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
   ```
2. Install ESP32 board from Board Manager
3. Select "AI Thinker ESP32-CAM" as board

### 3. Code Upload
1. Upload `nano.ino` to Arduino Nano
2. Upload `esp32softap.ino` to ESP32-CAM module
3. Ensure serial connections are correct

### 4. Network Configuration
- **SSID**: `Railway_Crossing_AP`
- **Password**: `railway123`
- **ESP32 IP**: `192.168.4.1` (default AP mode)

## 📱 Usage

### Automatic Mode (Default)
1. System starts in automatic mode
2. IR sensors continuously monitor for trains
3. Gate closes when train is detected
4. Gate opens 3 seconds after train passes
5. Status updates shown on LCD and web interface

### Manual Mode
#### Physical Buttons:
- **Open Button**: Manually open gate
- **Close Button**: Manually close gate  
- **Auto Button**: Return to automatic mode

#### Web Interface:
1. Connect to WiFi AP: `Railway_Crossing_AP`
2. Open browser and navigate to: `192.168.4.1`
3. Use web controls for manual operation

### Status Monitoring
- **LCD Display**: Real-time status with custom train/gate icons
- **LED Indicators**: 
  - Green: Gate Open
  - Red: Gate Closed
  - Status LED: Blinking patterns indicate system state
- **Buzzer**: Audio alerts during gate operation and train detection

## 🌐 Web Interface

The web dashboard provides:
- **Real-time Status**: Gate position, train detection, control mode
- **Manual Controls**: Open/Close gate, Switch to auto mode
- **Visual Indicators**: Animated train detection alerts
- **Mobile Responsive**: Works on smartphones and tablets
- **Modern UI**: Glass-morphism design with animations

### API Endpoints
```
GET  /          # Main dashboard
GET  /status    # JSON status data
GET  /control   # Manual control commands
POST /api/control # API control endpoint
```

## 📡 Serial Communication Protocol

### ESP32 to Arduino Commands
```
STATUS          # Request status update
MANUAL_OPEN     # Manual gate open
MANUAL_CLOSE    # Manual gate close
AUTO_MODE       # Switch to automatic mode
```

### Arduino to ESP32 Responses
```
STATE:X,TRAIN:Y,MANUAL:Z    # Status update
GATE_OPENED                 # Gate operation complete
GATE_CLOSED                 # Gate operation complete
AUTO_ACTIVE                 # Mode change confirmation
MANUAL_ACTIVE               # Mode change confirmation
ERROR:message               # Error reporting
```

### State Codes
- **Gate States**: 0=Open, 1=Closed, 2=Opening, 3=Closing
- **Train Detection**: 0=No train, 1=Train detected
- **Control Mode**: 0=Automatic, 1=Manual

## 🛡️ Safety Features

### Hardware Safety
- **Fail-safe Design**: Gate defaults to closed position
- **Dual Sensor Redundancy**: Two IR sensors for reliable detection
- **Manual Override**: Always available for emergency situations
- **Visual/Audio Alerts**: Multiple notification methods

### Software Safety
- **Input Validation**: All commands validated before execution
- **Error Handling**: Comprehensive error detection and recovery
- **Watchdog Timers**: System health monitoring
- **Status Verification**: Continuous system state verification

## 🐛 Troubleshooting

### Common Issues

#### No WiFi Connection
- Check ESP32 power supply
- Verify antenna connection
- Reset ESP32 and retry connection

#### Gate Not Moving
- Check servo power supply (5V required)
- Verify servo signal connection
- Test servo independently

#### Sensors Not Detecting
- Check IR sensor power (3.3V)
- Verify sensor alignment
- Test sensors with obstacles

#### LCD Not Displaying
- Check I2C connections (SDA/SCL)
- Verify LCD I2C address (try 0x27, 0x3F)
- Check power supply to LCD

#### Serial Communication Issues
- Verify TX/RX connections (crossed)
- Check baud rate (9600)
- Ensure common ground connection

### Debug Mode
Enable debug output by monitoring serial console:
```
Arduino Nano: 9600 baud
ESP32: 115200 baud
```

## 🤝 Contributing

1. Fork the repository
2. Create feature branch: `git checkout -b feature-name`
3. Commit changes: `git commit -am 'Add feature'`
4. Push to branch: `git push origin feature-name`
5. Submit pull request

### Development Guidelines
- Follow Arduino coding standards
- Add comments for complex logic
- Test thoroughly before submitting
- Update documentation as needed

## 📝 License

open for all 

## 🙏 Acknowledgments

- Arduino community for libraries and support
- ESP32 development team
- Contributors and testers

## 📞 Support

For support and questions:
- Create an issue on GitHub
- Check troubleshooting section
- Review code comments for implementation details

---

**⚠️ Safety Notice**: This system is designed for educational and prototyping purposes. For actual railway applications, please consult with railway safety authorities and follow appropriate safety standards and regulations.
