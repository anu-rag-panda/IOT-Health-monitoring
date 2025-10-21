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
9. [Safety Guidelines](#safety-guidelines)

---

## 🏥 Project Overview

### Purpose
The IoT Health Monitoring System is a comprehensive biomedical monitoring solution that collects and analyzes vital signs in real-time using multiple medical-grade sensors. The system provides healthcare professionals with a complete patient monitoring platform with cloud storage and web-based visualization.

### Key Features
- **Multi-parameter Monitoring**: ECG, Heart Rate, SpO₂, Temperature, and HRV
- **Real-time Web Dashboard**: Live data visualization with ECG waveform
- **Direct Database Integration**: MySQL storage without PHP dependency
- **Dual Cloud Storage**: ThingSpeak for real-time streaming + MySQL for records
- **20-second Clinical Scans**: Controlled data collection with averaging
- **Mobile-responsive Interface**: Accessible on any device
- **Medical-grade Data Processing**: HRV calculation and signal filtering

### Target Applications
- Remote patient monitoring
- Clinical healthcare facilities
- Medical research studies
- Home healthcare systems
- Telemedicine applications

---

## 🏗️ System Architecture

### System Block Diagram
```
┌─────────────────┐    ┌──────────────────┐    ┌─────────────────┐
│   BIOSENSORS    │    │   ESP8266 MCU    │    │   CLOUD STORAGE │
│                 │    │                  │    │                 │
│  AD8232 (ECG)   │◄──►│  Web Server      │◄──►│   ThingSpeak    │
│  MAX30100 (SpO₂)│    │  Data Processing │    │   MySQL Database│
│  DS18B20 (Temp) │    │  WiFi Connection │    │                 │
└─────────────────┘    └──────────────────┘    └─────────────────┘
         │                        │                        │
         │                        │                        │
         ▼                        ▼                        ▼
  Patient Interface        Healthcare Provider        Data Analytics
  Electrodes/Sensors      Web Dashboard              Long-term Storage
```

### Data Flow Architecture
1. **Physical Layer**: Sensor data acquisition from patient
2. **Processing Layer**: ESP8266 filtering and analysis
3. **Presentation Layer**: Web dashboard visualization
4. **Storage Layer**: Cloud and database persistence

### Technical Specifications
| Parameter | Specification |
|-----------|---------------|
| ECG Sampling Rate | 100 samples/20s (5 Hz) |
| SpO₂ Accuracy | ±2% |
| Temperature Range | 10°C to 85°C |
| Temperature Accuracy | ±0.5°C |
| Heart Rate Range | 30-250 BPM |
| WiFi Standard | 802.11 b/g/n |
| Web Server Port | 80 |
| Database | MySQL 5.7+ |

---

## 🔌 Hardware Setup

### Bill of Materials
| Component | Quantity | Specification | Purpose |
|-----------|----------|---------------|---------|
| ESP8266 NodeMCU | 1 | CP2102, 4MB Flash | Main Controller |
| AD8232 ECG Module | 1 | 3.3V, Analog Output | Heart Electrical Activity |
| MAX30100 Sensor | 1 | I2C, 3.3V | Pulse Oximetry |
| DS18B20 Sensor | 1 | Waterproof, 3.3V | Body Temperature |
| ECG Electrodes | 3+ | Disposable, Ag/AgCl | Patient Interface |
| Breadboard | 1 | 400 points | Circuit Assembly |
| Jumper Wires | 15+ | Male-to-Male | Connections |
| 4.7kΩ Resistor | 1 | 1/4W | DS18B20 Pull-up |
| Micro USB Cable | 1 | 5V/2A | Power Supply |

### Detailed Pin Connections

#### ESP8266 to AD8232
| ESP8266 Pin | AD8232 Pin | Wire Color | Purpose |
|-------------|------------|------------|---------|
| D1 (GPIO5) | LO+ | Yellow | Lead-off Detection Positive |
| D2 (GPIO4) | LO- | Green | Lead-off Detection Negative |
| A0 (ADC0) | OUTPUT | Blue | ECG Analog Signal |
| 3.3V | 3.3V | Red | Power Supply |
| GND | GND | Black | Ground |

#### ESP8266 to MAX30100
| ESP8266 Pin | MAX30100 Pin | Purpose |
|-------------|--------------|---------|
| D4 (GPIO2) | SDA | I2C Data Line |
| D5 (GPIO14) | SCL | I2C Clock Line |
| 3.3V | VIN | Power Supply (3.3V) |
| GND | GND | Ground |

#### ESP8266 to DS18B20
| ESP8266 Pin | DS18B20 Pin | Purpose |
|-------------|-------------|---------|
| D3 (GPIO0) | DATA | OneWire Data |
| 3.3V | VDD | Power Supply |
| GND | GND | Ground |

**Note**: Connect 4.7kΩ resistor between DATA and 3.3V for DS18B20

### Circuit Assembly Instructions

#### Step 1: Power Connections
1. Connect ESP8266 3.3V to all sensor VCC pins
2. Connect ESP8266 GND to all sensor GND pins
3. Verify stable 3.3V power supply

#### Step 2: AD8232 Wiring
```
AD8232 Pinout:
┌─────────────┐
│ 3.3V ───────┤ » ESP8266 3.3V
│ GND ────────┤ » ESP8266 GND
│ OUTPUT ─────┤ » ESP8266 A0
│ LO+ ────────┤ » ESP8266 D1
│ LO- ────────┤ » ESP8266 D2
│ SDN ────────┤ » Not Connected
└─────────────┘
```

#### Step 3: MAX30100 Wiring
```
MAX30100 Pinout:
┌─────────────┐
│ VIN ────────┤ » ESP8266 3.3V
│ GND ────────┤ » ESP8266 GND
│ SDA ────────┤ » ESP8266 D4
│ SCL ────────┤ » ESP8266 D5
│ INT ────────┤ » Not Connected
└─────────────┘
```

#### Step 4: DS18B20 Wiring
```
DS18B20 (Waterproof):
Red Wire   » ESP8266 3.3V
Black Wire » ESP8266 GND
Yellow Wire» ESP8266 D3 + 4.7kΩ to 3.3V
```

### Sensor Placement Guidelines

#### ECG Electrode Placement (3-Lead)
```
Standard Limb Lead Configuration:
RA (Right Arm) » White Wire » Upper right chest
LA (Left Arm)  » Black Wire » Upper left chest  
RL (Right Leg) » Red Wire   » Lower right abdomen (Reference)

Skin Preparation:
1. Clean skin with alcohol wipe
2. Shave excessive hair if present
3. Ensure good skin contact
4. Check lead-off detection status
```

#### Pulse Oximeter Placement
- Index finger preferred (good vascularization)
- Ensure proper alignment of LED and photodetector
- Avoid excessive pressure that may restrict blood flow
- Keep finger still during measurement
- Avoid nail polish or artificial nails

#### Temperature Sensor Placement
- Axillary (armpit) for body temperature
- Ensure good skin contact
- Allow 2-3 minutes for stabilization
- Avoid direct exposure to ambient temperature

---

## 💻 Software Installation

### Arduino IDE Setup

#### 1. Install Arduino IDE
Download from [arduino.cc](https://www.arduino.cc/en/software)
- Version 1.8.x or 2.0.x recommended

#### 2. ESP8266 Board Support
Add to **File → Preferences → Additional Board Manager URLs**:
```
http://arduino.esp8266.com/stable/package_esp8266com_index.json
```

Then install:
- **Tools → Board → Boards Manager**
- Search "esp8266"
- Install "ESP8266 by ESP8266 Community"

#### 3. Required Libraries Installation
Install via **Tools → Manage Libraries**:

| Library | Version | Purpose |
|---------|---------|---------|
| ESP8266WiFi | 2.7.4+ | WiFi Connectivity |
| ESP8266WebServer | 1.0+ | Web Server Functions |
| OneWire | 2.3.6+ | DS18B20 Communication |
| DallasTemperature | 3.9.0+ | Temperature Sensor |
| MAX30100 by OXullo Intersecans | 1.0.1+ | Pulse Oximeter |
| ArduinoJson | 6.19.4+ | JSON Processing |
| MySQL Connector/Arduino | 1.1.0+ | Direct MySQL Connection |

### MySQL Database Setup

#### 1. Database Creation
```sql
-- Connect to MySQL as root
mysql -u root -p

-- Create database and user
CREATE DATABASE health_monitor;
CREATE USER 'arduino_user'@'%' IDENTIFIED BY 'secure_password_123';
GRANT ALL PRIVILEGES ON health_monitor.* TO 'arduino_user'@'%';
FLUSH PRIVILEGES;
```

#### 2. Table Schema
```sql
USE health_monitor;

CREATE TABLE patient_data (
    id INT AUTO_INCREMENT PRIMARY KEY,
    name VARCHAR(100) NOT NULL,
    age INT NOT NULL CHECK (age BETWEEN 1 AND 120),
    sex ENUM('M', 'F', 'O') NOT NULL,
    diseases TEXT,
    ecg_value DECIMAL(5,2) CHECK (ecg_value BETWEEN 0 AND 5),
    heart_rate INT CHECK (heart_rate BETWEEN 30 AND 250),
    spo2 DECIMAL(5,2) CHECK (spo2 BETWEEN 70 AND 100),
    temperature DECIMAL(4,2) CHECK (temperature BETWEEN 25 AND 45),
    hrv INT CHECK (hrv BETWEEN 0 AND 200),
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    
    INDEX idx_created_at (created_at),
    INDEX idx_patient_name (name),
    INDEX idx_vital_signs (heart_rate, spo2, temperature)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;
```

#### 3. MySQL Server Configuration
Edit `/etc/mysql/mysql.conf.d/mysqld.cnf`:
```ini
[mysqld]
bind-address = 0.0.0.0
max_connections = 200
wait_timeout = 28800
interactive_timeout = 28800
max_allowed_packet = 64M
```

Restart MySQL: `sudo systemctl restart mysql`

### ThingSpeak Setup

#### 1. Account Creation
1. Visit [thingspeak.com](https://thingspeak.com)
2. Create free MathWorks account
3. Verify email address

#### 2. Channel Configuration
1. Click "New Channel"
2. Configure channel fields:
   - Field 1: `ECG Value`
   - Field 2: `Heart Rate`
   - Field 3: `SpO2 Level`
   - Field 4: `Temperature`
   - Field 5: `HRV`

3. Save channel and note:
   - **Channel ID**
   - **Write API Key**

---

## ⚙️ Configuration Guide

### Arduino Code Configuration

#### 1. Network Configuration
```cpp
// WiFi Settings
const char* ssid = "Your_Clinic_WiFi";
const char* password = "SecurePassword123";

// Static IP (Optional - for fixed addressing)
// WiFi.config(IPAddress(192,168,1,100), 
//             IPAddress(192,168,1,1), 
//             IPAddress(255,255,255,0));
```

#### 2. Database Configuration
```cpp
// MySQL Server Details
IPAddress server_addr(192, 168, 1, 50);  // Your MySQL server IP
char user[] = "arduino_user";
char password_db[] = "secure_password_123";
char database[] = "health_monitor";
```

#### 3. ThingSpeak Configuration
```cpp
const String thingspeakAPIKey = "YOUR_WRITE_API_KEY_HERE";
```

#### 4. System Parameters
```cpp
// Data Collection Settings
const unsigned long SCAN_DURATION = 20000;    // 20 seconds
const int ECG_SAMPLES = 100;                  // 5 Hz sampling
const int HRV_SAMPLES = 10;                   // RR intervals for HRV

// Sensor Settings
#define MAX30100_LED_CURRENT MAX30100_LED_CURR_7_6MA
#define TEMPERATURE_PRECISION 12  // 12-bit resolution
```

### Calibration Procedures

#### AD8232 ECG Calibration
1. **Electrode Contact Check**
   - Monitor LO+ and LO- pins
   - Both should read LOW for good contact
   - Serial monitor shows "Electrode contact issue" if poor contact

2. **Signal Quality Verification**
   - Patient should be relaxed and still
   - Observe ECG waveform on web dashboard
   - Check for 60Hz power line interference

3. **Baseline Drift Correction**
   - Implement in software if needed:
   ```cpp
   // High-pass filter for baseline wander
   float highPassFilter(float input) {
     static float prevInput = 0;
     static float prevOutput = 0;
     float output = 0.995 * prevOutput + input - prevInput;
     prevInput = input;
     prevOutput = output;
     return output;
   }
   ```

#### MAX30100 Calibration
```cpp
void calibratePulseOximeter() {
  // Finger detection check
  if (pox.getHeartRate() == 0) {
    Serial.println("Please place finger on sensor");
    return;
  }
  
  // Signal quality assessment
  float irValue = pox.getIR();
  if (irValue < 50000) {
    Serial.println("Weak signal - adjust finger position");
  }
  
  // Adjust LED current based on skin tone
  // Lighter skin: Lower current (4.4mA)
  // Darker skin: Higher current (11.0mA, 14.2mA, etc.)
}
```

#### DS18B20 Temperature Calibration
1. Compare with clinical thermometer
2. Calculate offset if needed:
   ```cpp
   float calibratedTemp = rawTemp + calibrationOffset;
   ```
3. Allow 2-3 minutes for stabilization

### Web Dashboard Customization

#### Color Scheme Modification
```css
/* Primary Colors */
:root {
  --primary-blue: #3498db;
  --success-green: #27ae60;
  --warning-orange: #f39c12;
  --danger-red: #e74c3c;
}

/* Status Indicators */
.status-normal { background-color: #d4edda; color: #155724; }
.status-warning { background-color: #fff3cd; color: #856404; }
.status-critical { background-color: #f8d7da; color: #721c24; }
```

#### Adding New Parameters
1. Extend PatientData struct
2. Update web interface HTML
3. Modify database schema
4. Update data processing functions

---

## 🌐 API Documentation

### Web Server Endpoints

#### GET `/`
**Purpose**: Serve main healthcare dashboard
**Response**: HTML page with patient form and real-time monitoring
**Content-Type**: `text/html`

#### POST `/start`
**Purpose**: Initiate 20-second vital signs scan
**Parameters** (form-data):
- `name` (string, required): Patient full name
- `age` (int, required): Patient age (1-120)
- `sex` (enum, required): M, F, or O
- `diseases` (string, optional): Comma-separated medical conditions

**Response**: `200 OK` with "Scan started!" or error message

#### POST `/upload`
**Purpose**: Upload collected data to cloud services
**Parameters**: None (uses internally stored data)
**Response**: `200 OK` on success, `500 Internal Server Error` on failure

#### GET `/data`
**Purpose**: Retrieve current sensor readings (JSON)
**Response**:
```json
{
  "ecg": 1.25,
  "hr": 72,
  "spo2": 98.0,
  "temp": 36.8,
  "hrv": 35,
  "scanning": false
}
```

### Data Models

#### Patient Data Structure
```cpp
struct PatientData {
  String name;        // Patient identifier
  int age;           // Years
  String sex;        // Biological sex
  String diseases;   // Medical history
  
  // Vital signs (collected during scan)
  float ecgValue;    // Millivolts (averaged)
  int heartRate;     // Beats per minute
  float spo2;        // Oxygen saturation %
  float temperature; // Celsius
  int hrv;           // Heart Rate Variability (ms)
};
```

#### Database Schema
| Field | Type | Constraints | Description |
|-------|------|-------------|-------------|
| id | INT AUTO_INCREMENT | PRIMARY KEY | Record identifier |
| name | VARCHAR(100) | NOT NULL | Patient name |
| age | INT | 1-120 | Patient age |
| sex | ENUM('M','F','O') | NOT NULL | Biological sex |
| diseases | TEXT | NULL | Medical conditions |
| ecg_value | DECIMAL(5,2) | 0-5 V | ECG signal level |
| heart_rate | INT | 30-250 BPM | Heart rate |
| spo2 | DECIMAL(5,2) | 70-100% | Oxygen saturation |
| temperature | DECIMAL(4,2) | 25-45°C | Body temperature |
| hrv | INT | 0-200 ms | Heart rate variability |
| created_at | TIMESTAMP | DEFAULT CURRENT_TIMESTAMP | Recording time |

### Cloud Integration

#### ThingSpeak API
**Endpoint**: `api.thingspeak.com/update`
**Method**: POST
**Parameters**:
- `api_key` (string): Write API key
- `field1` (float): ECG value
- `field2` (int): Heart rate
- `field3` (float): SpO2 percentage
- `field4` (float): Temperature
- `field5` (int): HRV value

**Rate Limit**: Free tier: 15-second update interval

#### Direct MySQL Connection
**Library**: MySQL_Connector_Arduino
**Protocol**: MySQL Native Protocol
**Port**: 3306
**Authentication**: Username/Password
**SSL**: Not supported (use secure network)

---

## 🔧 Troubleshooting Guide

### Common Issues and Solutions

#### 1. WiFi Connection Problems
**Symptoms**: Cannot connect, IP not obtained
**Solutions**:
```cpp
// Add detailed WiFi status monitoring
void checkWiFiStatus() {
  switch (WiFi.status()) {
    case WL_NO_SHIELD: Serial.println("WiFi shield not present"); break;
    case WL_IDLE_STATUS: Serial.println("WiFi idle"); break;
    case WL_NO_SSID_AVAIL: Serial.println("SSID not available"); break;
    case WL_SCAN_COMPLETED: Serial.println("Scan completed"); break;
    case WL_CONNECTED: Serial.println("Connected"); break;
    case WL_CONNECT_FAILED: Serial.println("Connection failed"); break;
    case WL_CONNECTION_LOST: Serial.println("Connection lost"); break;
    case WL_DISCONNECTED: Serial.println("Disconnected"); break;
  }
}
```

#### 2. Sensor Reading Issues

##### AD8232 Problems
- **No Signal**: Check electrode contact and gel
- **Noisy Signal**: Ensure patient is still, check for interference
- **Lead-off Detection**: Verify all three electrodes properly attached

##### MAX30100 Problems
```cpp
// Add sensor status checking
void checkPulseOximeter() {
  if (!pox.begin()) {
    Serial.println("MAX30100 not found. Check:");
    Serial.println("1. I2C connections (SDA/SCL)");
    Serial.println("2. Power supply (3.3V stable)");
    Serial.println("3. I2C address (0x57)");
    return;
  }
  
  // Check signal quality
  if (pox.getHeartRate() == 0) {
    Serial.println("No finger detected or weak signal");
  }
}
```

##### DS18B20 Problems
- **No Temperature Reading**: Check pull-up resistor and wiring
- **Incorrect Readings**: Verify sensor is not shorted
- **-127°C Reading**: Wiring issue or sensor failure

#### 3. Database Connection Issues

##### MySQL Connection Failed
```cpp
// Enhanced connection handling
bool connectToMySQLWithRetry() {
  int retries = 3;
  while (retries > 0) {
    if (conn.connect(server_addr, 3306, user, password_db)) {
      Serial.println("MySQL connected successfully");
      return true;
    }
    Serial.println("MySQL connection failed, retrying...");
    delay(2000);
    retries--;
  }
  return false;
}
```

##### Common MySQL Issues
1. **Access Denied**: Check username/password and host permissions
2. **Can't Connect**: Verify server IP and firewall settings
3. **Packet Too Large**: Increase `max_allowed_packet` in MySQL config

#### 4. Web Server Issues

##### Dashboard Not Loading
- Check ESP8266 IP address in Serial Monitor
- Verify client is on same network subnet
- Check for firewall blocking port 80

##### Data Not Updating
- Verify JavaScript is enabled in browser
- Check browser console for errors
- Ensure `/data` endpoint returns valid JSON

### Diagnostic Commands

#### Serial Monitor Debugging
Enable detailed debugging:
```cpp
void setup() {
  Serial.begin(115200);
  Serial.setDebugOutput(true);
}
```

Expected startup sequence:
```
Health Monitoring System Initializing...
WiFi connected! IP: 192.168.1.100
MySQL connected successfully!
MAX30100 initialized successfully!
HTTP server started on port 80
System Ready!
```

#### Network Testing
```bash
# Test MySQL connection
telnet MYSQL_SERVER_IP 3306

# Test web server
curl http://ESP8266_IP/data

# Test ThingSpeak API
curl -X POST "https://api.thingspeak.com/update?api_key=TEST&field1=1.0"
```

### Performance Optimization

#### Memory Management
```cpp
// Monitor heap memory
void checkMemory() {
  Serial.printf("Free Heap: %d bytes\n", ESP.getFreeHeap());
  Serial.printf("Heap Fragmentation: %d%%\n", ESP.getHeapFragmentation());
}

// Use String sparingly to avoid fragmentation
char buffer[128];  // Pre-allocate buffers
snprintf(buffer, sizeof(buffer), "INSERT INTO ...");
```

#### Sampling Rate Optimization
- ECG: 5 Hz (clinical monitoring standard)
- Pulse Ox: Continuous (library managed)
- Temperature: Once per scan (slow response)

---

## 🛠️ Maintenance Procedures

### Regular Maintenance Schedule

#### Daily Checks
- [ ] Verify all sensor connections
- [ ] Test WiFi connectivity
- [ ] Check web dashboard accessibility
- [ ] Verify data upload to both services

#### Weekly Maintenance
- [ ] Calibrate against reference equipment
- [ ] Check electrode supply and quality
- [ ] Review database storage capacity
- [ ] Backup MySQL database

#### Monthly Procedures
- [ ] Update Arduino libraries
- [ ] Check for firmware updates
- [ ] Performance review and optimization
- [ ] Security audit

#### Quarterly Tasks
- [ ] Replace ECG electrodes (if reusable)
- [ ] Sensor accuracy verification
- [ ] Cable and connector inspection
- [ ] Complete system testing

### Database Maintenance

#### Backup Procedures
```sql
-- Daily backup
mysqldump -u root -p health_monitor > backup_$(date +%Y%m%d).sql

-- Archive old records (keep 1 year)
CREATE TABLE patient_data_archive AS 
SELECT * FROM patient_data WHERE created_at < DATE_SUB(NOW(), INTERVAL 1 YEAR);

DELETE FROM patient_data WHERE created_at < DATE_SUB(NOW(), INTERVAL 1 YEAR);
```

#### Performance Optimization
```sql
-- Regular optimization
OPTIMIZE TABLE patient_data;

-- Index maintenance
ANALYZE TABLE patient_data;

-- Storage monitoring
SELECT 
  table_name AS `Table`,
  round(((data_length + index_length) / 1024 / 1024), 2) `Size (MB)`
FROM information_schema.TABLES 
WHERE table_schema = "health_monitor";
```

### Software Updates

#### Version Control
- Maintain code in Git repository
- Tag releases with version numbers
- Keep changelog updated
- Test updates in staging environment

#### Update Procedure
1. Backup current code and database
2. Test new version with sample data
3. Deploy during maintenance window
4. Verify all functionality
5. Update documentation

---

## ⚠️ Safety Guidelines

### Electrical Safety
- **Patient Isolation**: Ensure no direct electrical connection to mains
- **Grounding**: Proper grounding of all equipment
- **Insulation**: Check cables and connectors for damage
- **Power Supply**: Use medical-grade power adapters if available

### Clinical Safety
- **Not for Diagnosis**: This system is for monitoring only
- **Clinical Supervision**: Always have healthcare professional oversight
- **Emergency Procedures**: Have manual backup monitoring available
- **Data Interpretation**: Trained personnel should interpret results

### Data Security
- **Network Security**: Use secure WiFi with WPA2 encryption
- **Access Control**: Limit database access to authorized personnel
- **Data Encryption**: Consider HTTPS for web interface
- **Privacy Compliance**: Follow HIPAA/GDPR for patient data

### Usage Precautions
1. **Do not use** with patients connected to other electrical equipment
2. **Discontinue use** if any malfunction is suspected
3. **Regularly inspect** all components for wear or damage
4. **Keep away** from water and moisture
5. **Store properly** when not in use

## 📞 Support and Contact

### Documentation Updates
- Check GitHub repository for latest versions
- Subscribe to update notifications
- Review change logs before updates

### Technical Support
For technical issues:
1. Check this documentation first
2. Review serial monitor output
3. Verify all connections
4. Test individual components
5. Consult community forums

### Emergency Contacts
- Document Manager - Anurag Panda (+91 9883863117)

---

## 📄 License and Compliance

### Medical Device Regulations
**Important**: This system may be subject to medical device regulations in your region. Consult with regulatory authorities before clinical use.

### Software License
MIT License - See LICENSE file for details

### Compliance Standards
- IEC 60601-1: Medical electrical equipment safety
- HIPAA: Patient data privacy (US)
- GDPR: Data protection (EU)
- FDA: Medical device regulations (US)

### Disclaimer
This system is intended for educational and monitoring purposes only. It is not a certified medical device and should not be used for diagnostic purposes. Always consult healthcare professionals for medical advice and diagnosis.

---

**Document Version**: 2.0  
**Last Updated**: December 2024  
**System Version**: Health Monitor v2.0  
**Compatibility**: ESP8266 NodeMCU, MySQL 5.7+, Arduino IDE 1.8+
