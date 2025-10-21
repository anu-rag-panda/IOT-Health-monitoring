# IoT Health Monitoring System - Complete Documentation

## 📖 Table of Contents
1. [Project Overview](#project-overview)
2. [System Architecture](#system-architecture)
3. [Hardware Setup](#hardware-setup)
4. [Software Installation](#software-installation)
5. [Configuration Guide](#configuration-guide)
6. [API Documentation](#api-documentation)
7. [Troubleshooting](#troubleshooting)
8. [Maintenance](#maintenance)

---

## 🏥 Project Overview

### Purpose
The IoT Health Monitoring System is a comprehensive solution for real-time patient vital sign monitoring using multiple biomedical sensors connected to an ESP8266 microcontroller. The system collects ECG, heart rate, SpO2, and temperature data, then transmits it to both cloud platforms (ThingSpeak) and local databases for analysis and storage.

### Key Features
- **Multi-sensor Integration**: ECG, Pulse Oximetry, Temperature monitoring
- **Real-time Web Interface**: Live data visualization and patient management
- **Cloud Storage**: Dual storage to ThingSpeak and MySQL database
- **20-second Scan Sessions**: Controlled data collection periods
- **Responsive Design**: Mobile-friendly web dashboard

### Target Users
- Healthcare professionals
- Remote patient monitoring systems
- Medical research institutions
- Home healthcare applications

---

## 🏗️ System Architecture

### Block Diagram
```
[ Sensors ] → [ ESP8266 ] → [ Web Server ] → [ Cloud Storage ]
   │            │               │              │
 ECG        WiFi Module    Patient Dashboard  ThingSpeak
 Pulse Ox                  Data Processing    MySQL Database
 Temperature
```

### Data Flow
1. **Sensor Data Acquisition**
   - AD8232: ECG waveform and heart rate
   - MAX30100: Heart rate variability and SpO2
   - DS18B20: Body temperature

2. **Local Processing**
   - ESP8266 processes and filters data
   - 20-second averaging window
   - Web server hosting for real-time display

3. **Cloud Integration**
   - ThingSpeak: Real-time data streaming and visualization
   - MySQL: Permanent patient record storage

---

## 🔌 Hardware Setup

### Components List
| Component | Quantity | Purpose |
|-----------|----------|---------|
| ESP8266 NodeMCU | 1 | Main microcontroller & WiFi |
| AD8232 ECG Sensor | 1 | Heart electrical activity |
| MAX30100 Pulse Oximeter | 1 | Heart rate & oxygen saturation |
| DS18B20 Temperature Sensor | 1 | Body temperature |
| Breadboard | 1 | Circuit assembly |
| Jumper Wires | Multiple | Connections |
| 4.7kΩ Resistor | 1 | DS18B20 pull-up |
| ECG Electrodes | 3 | Patient connection |

### Pin Connections Table
| ESP8266 Pin | Sensor | Connection | Notes |
|-------------|---------|------------|-------|
| D1 | AD8232 | LO+ | Lead-off detection |
| D2 | AD8232 | LO- | Lead-off detection |
| A0 | AD8232 | OUTPUT | ECG analog output |
| D4 | MAX30100 | SDA | I2C data |
| D5 | MAX30100 | SCL | I2C clock |
| D3 | DS18B20 | DATA | OneWire data |
| 3.3V | All | VCC | Power supply |
| GND | All | GND | Ground |

### Circuit Diagram
```
ESP8266 NodeMCU
┌─────────────┐
│          3V3│───┬───[All Sensors VCC]
│          GND│───┬───[All Sensors GND]
│           D1│───┼───[AD8232 LO+]
│           D2│───┼───[AD8232 LO-]
│           A0│───┼───[AD8232 OUTPUT]
│           D4│───┼───[MAX30100 SDA]
│           D5│───┼───[MAX30100 SCL]
│           D3│───┼───[DS18B20 DATA]───4.7kΩ───3V3
└─────────────┘
```

### Sensor Placement Guidelines

#### AD8232 ECG Electrodes
```
Right Arm (RA) ─── White wire
Left Arm (LA) ─── Black wire  
Right Leg (RL) ─── Red wire (ground reference)
```

#### MAX30100 Placement
- Finger clip sensor on index finger
- Ensure good contact without excessive pressure
- Avoid ambient light interference

#### DS18B20 Placement
- Axillary (armpit) for body temperature
- Ensure good skin contact
- Can be insulated for accurate reading

---

## 💻 Software Installation

### Arduino IDE Setup

#### 1. Install Required Boards
```arduino
File → Preferences → Additional Board Manager URLs:
http://arduino.esp8266.com/stable/package_esp8266com_index.json
```

#### 2. Install Libraries
```arduino
Tools → Manage Libraries → Search and Install:
- ESP8266WiFi
- ESP8266WebServer
- OneWire
- DallasTemperature
- MAX30100 by OXullo Intersecans
- ArduinoJson by Benoit Blanchon
```

#### 3. Board Configuration
```arduino
Tools → Board → NodeMCU 1.0 (ESP-12E Module)
Tools → Flash Size → 4M (3M SPIFFS)
Tools → CPU Frequency → 80 MHz
Tools → Upload Speed → 115200
Tools → Port → [Your COM Port]
```

### Server Setup

#### PHP/MySQL Environment
1. **Install XAMPP/WAMP** or use cloud hosting
2. **Create MySQL Database**:
```sql
CREATE DATABASE health_monitor;
USE health_monitor;

CREATE TABLE patient_data (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    age INT NOT NULL,
    sex ENUM('M', 'F', 'O') NOT NULL,
    diseases TEXT,
    ecg_value DECIMAL(5,2),
    heart_rate INT,
    spo2 DECIMAL(5,2),
    temperature DECIMAL(4,2),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);
```

3. **Upload PHP Script** to your web server in the root directory as `health_upload.php`

### ThingSpeak Setup

1. **Create Account** at [thingspeak.com](https://thingspeak.com)
2. **Create New Channel**:
   - Channel Name: "Health Monitor"
   - Field 1: ECG_Value
   - Field 2: Heart_Rate
   - Field 3: SpO2
   - Field 4: Temperature

3. **Note API Key** from "API Keys" tab

---

## ⚙️ Configuration Guide

### Arduino Code Configuration

#### WiFi Settings
```cpp
const char* ssid = "Your_WiFi_SSID";
const char* password = "Your_WiFi_Password";
```

#### Cloud Services
```cpp
// ThingSpeak
const String thingspeakAPIKey = "YOUR_THINGSPEAK_WRITE_API_KEY";

// MySQL Server  
const char* mysqlServer = "yourserver.com";
const String mysqlEndpoint = "/health_upload.php";
```

#### Sensor Parameters
```cpp
const unsigned long SCAN_DURATION = 20000; // 20 seconds
const int ECG_SAMPLES = 100; // Samples for averaging
```

### Calibration Procedures

#### AD8232 ECG Calibration
1. Ensure patient is relaxed and still
2. Verify all electrodes have good contact
3. Check for lead-off detection (LED indicators)
4. Monitor serial output for "Electrode contact issue" messages

#### MAX30100 Calibration
```cpp
// In initializeSensors() function
pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA); // Adjust for skin tone
// Available currents: 0=0.0mA, 1=4.4mA, 2=7.6mA, 3=11.0mA, etc.
```

#### DS18B20 Temperature Calibration
- Compare readings with clinical thermometer
- Allow 2-3 minutes for stabilization
- Consider adding offset in code if needed

---

## 🌐 API Documentation

### Web Server Endpoints

#### GET /
**Purpose**: Serve main web dashboard
**Response**: HTML page with patient form and data display

#### POST /start
**Purpose**: Begin 20-second data collection
**Parameters**:
- `name` (string): Patient name
- `age` (int): Patient age  
- `sex` (string): Patient gender (M/F/O)
- `diseases` (string): Known medical conditions

**Response**: "Scan started!"

#### POST /upload
**Purpose**: Send collected data to cloud services
**Response**: "Data uploaded successfully!" or "Upload failed!"

### Data Formats

#### MySQL JSON Payload
```json
{
  "name": "John Doe",
  "age": 28,
  "sex": "M",
  "diseases": "Hypertension",
  "ecg": 1.25,
  "hr": 72,
  "spo2": 98.0,
  "temp": 36.8
}
```

#### ThingSpeak API Call
```
POST /update
api_key=YOUR_API_KEY&field1=1.25&field2=72&field3=98.0&field4=36.8
```

### Database Schema Details

#### patient_data Table
| Field | Type | Description |
|-------|------|-------------|
| id | INT AUTO_INCREMENT | Unique record identifier |
| name | VARCHAR(100) | Patient full name |
| age | INT | Patient age in years |
| sex | ENUM('M','F','O') | Patient gender |
| diseases | TEXT | Comma-separated medical conditions |
| ecg_value | DECIMAL(5,2) | Average ECG voltage |
| heart_rate | INT | Beats per minute |
| spo2 | DECIMAL(5,2) | Oxygen saturation percentage |
| temperature | DECIMAL(4,2) | Body temperature in °C |
| created_at | TIMESTAMP | Data collection timestamp |

---

## 🔧 Troubleshooting Guide

### Common Issues and Solutions

#### 1. WiFi Connection Issues
**Symptoms**: Cannot connect to WiFi, IP address not obtained
**Solutions**:
- Verify SSID and password
- Check WiFi signal strength
- Ensure router supports 2.4GHz band
- Add WiFi status monitoring in code

#### 2. Sensor Reading Problems

##### AD8232 Issues
```cpp
// Check lead-off detection
if (digitalRead(ECG_LO_PLUS) == HIGH || digitalRead(ECG_LO_MINUS) == HIGH) {
    Serial.println("Check electrode contact!");
}
```

##### MAX30100 Issues
- Ensure finger is properly positioned
- Check I2C connection (SDA/SCL)
- Verify sensor is getting adequate power
- Monitor serial output for initialization status

##### DS18B20 Issues
- Check pull-up resistor (4.7kΩ between DATA and 3.3V)
- Verify OneWire bus connection
- Ensure sensor is not shorted

#### 3. Web Server Not Accessible
- Check ESP8266 IP address in Serial Monitor
- Verify no firewall blocking port 80
- Ensure client is on same network

#### 4. Cloud Upload Failures

##### ThingSpeak Issues
- Verify API key is correct
- Check field numbers match channel configuration
- Monitor ThingSpeak channel for data reception

##### MySQL/PHP Issues
- Check server URL and endpoint path
- Verify PHP script is accessible via browser
- Check MySQL connection parameters
- Review server error logs

### Serial Debug Messages
The system provides comprehensive serial output:
```
Health Monitoring System Ready!
WiFi connected!
IP Address: 192.168.1.100
HTTP server started
Scan started for patient: John Doe
MAX30100 initialized successfully!
Averages calculated:
ECG: 1.25
HR: 72
SpO2: 98.0
Temp: 36.8
Data sent to ThingSpeak
Data sent to MySQL server
```

### Performance Optimization

#### Memory Management
- Use `String` sparingly to avoid heap fragmentation
- Pre-allocate buffers where possible
- Monitor stack usage with `ESP.getFreeHeap()`

#### Sampling Rates
- ECG: ~5 samples per second (for 20-second average)
- Pulse Ox: Continuous monitoring via library
- Temperature: Single reading per scan session

---

## 🛠️ Maintenance

### Regular Checks
1. **Weekly**: Verify all sensor connections
2. **Monthly**: Calibrate against medical equipment
3. **Quarterly**: Update software libraries
4. **Annually**: Replace electrodes and sensors as needed

### Data Management
- Implement database archiving for old records
- Set up automated backups for MySQL data
- Monitor ThingSpeak channel limits (free tier: 3 million messages/year)

### Safety Considerations
- This system is for monitoring purposes only, not for diagnosis
- Always have clinical supervision for medical applications
- Ensure electrical isolation for patient safety
- Follow local medical device regulations

### Expansion Possibilities

#### Hardware Additions
- Add blood pressure monitor
- Include respiratory rate sensor
- Integrate GPS for mobile applications
- Add LCD display for local readouts

#### Software Enhancements
- Implement user authentication
- Add data analytics and trend analysis
- Create mobile app companion
- Add alert system for abnormal readings
- Implement data encryption for privacy

---

## 📞 Support

### Documentation Updates
- Check GitHub repository for latest versions
- Subscribe to update notifications
- Review change logs before updates

### Community Resources
- Arduino Forum: ESP8266 sections
- GitHub: Sensor library repositories
- Stack Overflow: Arduino and IoT tags

### Technical Support
For technical issues:
1. Check this documentation first
2. Review serial monitor output
3. Verify all connections
4. Test individual components
5. Consult community forums

---

## 📄 License and Disclaimer

### Medical Disclaimer
This system is intended for educational and monitoring purposes only. It is not a medical device and should not be used for diagnostic purposes. Always consult healthcare professionals for medical advice and diagnosis.

### Safety Warning
- Ensure proper electrical isolation when connected to humans
- Do not use with patients connected to mains-powered equipment
- Follow all local safety regulations and guidelines
- Regular maintenance and inspection required

### License
This project is open-source under MIT License. Use responsibly and in accordance with local regulations.

---

**Last Updated**: 21-10-2025  
**Version**: 1.0  
**Documentation Maintainer**: Anurag Panda