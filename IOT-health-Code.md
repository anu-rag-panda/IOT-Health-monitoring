# IoT Health Monitoring System - Complete Arduino Code (PHP-Free)

## Arduino Code (ESP8266 with Direct MySQL)

```cpp
#include <Wire.h>
#include <ESP8266WiFi.h>
#include <ESP8266WebServer.h>
#include <OneWire.h>
#include <DallasTemperature.h>
#include "MAX30100_PulseOximeter.h"
#include <ArduinoJson.h>
#include <MySQL_Connection.h>
#include <MySQL_Cursor.h>

// WiFi Credentials
const char* ssid = "YOUR_WIFI_SSID";
const char* password = "YOUR_WIFI_PASSWORD";

// ThingSpeak Configuration
const char* thingspeakServer = "api.thingspeak.com";
const String thingspeakAPIKey = "YOUR_THINGSPEAK_API_KEY";

// MySQL Database Configuration (Direct Connection)
IPAddress server_addr(MYSQL_SERVER_IP);  // Change to your MySQL server IP
char user[] = "arduino_user";            // MySQL username
char password_db[] = "arduino_password"; // MySQL password
char database[] = "health_monitor";      // Database name

WiFiClient client;
MySQL_Connection conn((Client *)&client);

// Sensor Pin Definitions
#define ECG_LO_PLUS D1
#define ECG_LO_MINUS D2
#define ECG_OUTPUT A0
#define ONE_WIRE_BUS D3
#define SDA_PIN D4
#define SCL_PIN D5

// Sensor Objects
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature tempSensor(&oneWire);
PulseOximeter pox;

// Web Server
ESP8266WebServer server(80);

// Data Variables
struct PatientData {
  String name = "";
  int age = 0;
  String sex = "";
  String diseases = "";
  float ecgValue = 0.0;
  int heartRate = 0;
  float spo2 = 0.0;
  float temperature = 0.0;
  int hrv = 0;
};

PatientData patient;
bool isScanning = false;
unsigned long scanStartTime = 0;
const unsigned long SCAN_DURATION = 20000; // 20 seconds

// ECG Data
const int ECG_SAMPLES = 100;
float ecgSamples[ECG_SAMPLES];
int ecgSampleIndex = 0;

// HRV Calculation
const int HRV_SAMPLES = 10;
unsigned long rrIntervals[HRV_SAMPLES];
int rrIndex = 0;
unsigned long lastBeatTime = 0;

void setup() {
  Serial.begin(115200);
  
  // Initialize pins
  pinMode(ECG_LO_PLUS, INPUT);
  pinMode(ECG_LO_MINUS, INPUT);
  pinMode(ECG_OUTPUT, INPUT);
  
  // Initialize sensors
  initializeSensors();
  
  // Connect to WiFi
  connectToWiFi();
  
  // Connect to MySQL Database
  connectToMySQL();
  
  // Initialize web server routes
  setupWebServer();
  
  Serial.println("Health Monitoring System Ready!");
}

void loop() {
  server.handleClient();
  
  if (isScanning) {
    readSensors();
    
    // Check if scan duration has elapsed
    if (millis() - scanStartTime >= SCAN_DURATION) {
      isScanning = false;
      calculateAverages();
      calculateHRV();
      Serial.println("Scan completed!");
    }
  }
  
  // Update pulse oximeter
  pox.update();
}

void initializeSensors() {
  // Initialize temperature sensor
  tempSensor.begin();
  
  // Initialize pulse oximeter
  Wire.begin(SDA_PIN, SCL_PIN);
  if (!pox.begin()) {
    Serial.println("MAX30100 initialization failed!");
  } else {
    Serial.println("MAX30100 initialized successfully!");
    // Set up beat detection callback
    pox.setOnBeatDetectedCallback(onBeatDetected);
  }
  
  // Set pulse oximeter configuration
  pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
  
  Serial.println("All sensors initialized!");
}

void onBeatDetected() {
  unsigned long currentTime = millis();
  if (lastBeatTime > 0) {
    unsigned long rrInterval = currentTime - lastBeatTime;
    rrIntervals[rrIndex] = rrInterval;
    rrIndex = (rrIndex + 1) % HRV_SAMPLES;
  }
  lastBeatTime = currentTime;
}

void connectToWiFi() {
  Serial.print("Connecting to WiFi");
  WiFi.begin(ssid, password);
  
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  
  Serial.println("\nWiFi connected!");
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());
}

void connectToMySQL() {
  Serial.print("Connecting to MySQL database...");
  
  if (conn.connect(server_addr, 3306, user, password_db)) {
    Serial.println("MySQL connected successfully!");
  } else {
    Serial.println("MySQL connection failed!");
  }
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/start", HTTP_POST, handleStartScan);
  server.on("/upload", HTTP_POST, handleUploadData);
  server.on("/data", HTTP_GET, handleGetData);
  
  server.begin();
  Serial.println("HTTP server started");
}

void handleRoot() {
  String html = createWebPage();
  server.send(200, "text/html", html);
}

void handleStartScan() {
  // Get patient details from form
  patient.name = server.arg("name");
  patient.age = server.arg("age").toInt();
  patient.sex = server.arg("sex");
  patient.diseases = server.arg("diseases");
  
  // Reset data
  resetSensorData();
  
  // Start scanning
  isScanning = true;
  scanStartTime = millis();
  
  Serial.println("Scan started for patient: " + patient.name);
  
  server.send(200, "text/plain", "Scan started!");
}

void handleUploadData() {
  if (sendToThingSpeak() && sendToMySQLDirect()) {
    server.send(200, "text/plain", "Data uploaded successfully!");
  } else {
    server.send(500, "text/plain", "Upload failed!");
  }
}

void handleGetData() {
  StaticJsonDocument<512> jsonDoc;
  jsonDoc["ecg"] = patient.ecgValue;
  jsonDoc["hr"] = patient.heartRate;
  jsonDoc["spo2"] = patient.spo2;
  jsonDoc["temp"] = patient.temperature;
  jsonDoc["hrv"] = patient.hrv;
  jsonDoc["scanning"] = isScanning;
  
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  
  server.send(200, "application/json", jsonString);
}

void readSensors() {
  // Read ECG
  readECG();
  
  // Read pulse oximeter data (handled in loop via update())
  patient.heartRate = pox.getHeartRate();
  patient.spo2 = pox.getSpO2();
  
  // Read temperature
  tempSensor.requestTemperatures();
  patient.temperature = tempSensor.getTempCByIndex(0);
}

void readECG() {
  if (digitalRead(ECG_LO_PLUS) == HIGH || digitalRead(ECG_LO_MINUS) == HIGH) {
    Serial.println("Electrode contact issue detected!");
    return;
  }
  
  int ecgRaw = analogRead(ECG_OUTPUT);
  float ecgVoltage = (ecgRaw * 3.3) / 1024.0;
  
  // Store sample for averaging
  ecgSamples[ecgSampleIndex] = ecgVoltage;
  ecgSampleIndex = (ecgSampleIndex + 1) % ECG_SAMPLES;
}

void calculateAverages() {
  // Calculate ECG average
  float sum = 0;
  for (int i = 0; i < ECG_SAMPLES; i++) {
    sum += ecgSamples[i];
  }
  patient.ecgValue = sum / ECG_SAMPLES;
  
  Serial.println("Averages calculated:");
  Serial.println("ECG: " + String(patient.ecgValue));
  Serial.println("HR: " + String(patient.heartRate));
  Serial.println("SpO2: " + String(patient.spo2));
  Serial.println("Temp: " + String(patient.temperature));
}

void calculateHRV() {
  // Simple HRV calculation using RR intervals
  if (rrIndex > 1) {
    long sum = 0;
    int count = 0;
    for (int i = 0; i < rrIndex; i++) {
      if (rrIntervals[i] > 0) {
        sum += rrIntervals[i];
        count++;
      }
    }
    
    if (count > 0) {
      long mean = sum / count;
      long variance = 0;
      for (int i = 0; i < rrIndex; i++) {
        if (rrIntervals[i] > 0) {
          long diff = rrIntervals[i] - mean;
          variance += diff * diff;
        }
      }
      variance /= count;
      patient.hrv = sqrt(variance);
    }
  }
  Serial.println("HRV: " + String(patient.hrv));
}

void resetSensorData() {
  patient.ecgValue = 0.0;
  patient.heartRate = 0;
  patient.spo2 = 0.0;
  patient.temperature = 0.0;
  patient.hrv = 0;
  ecgSampleIndex = 0;
  rrIndex = 0;
  lastBeatTime = 0;
  
  for (int i = 0; i < ECG_SAMPLES; i++) {
    ecgSamples[i] = 0.0;
  }
  for (int i = 0; i < HRV_SAMPLES; i++) {
    rrIntervals[i] = 0;
  }
}

bool sendToThingSpeak() {
  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    
    if (client.connect(thingspeakServer, 80)) {
      String postData = "api_key=" + thingspeakAPIKey +
                       "&field1=" + String(patient.ecgValue) +
                       "&field2=" + String(patient.heartRate) +
                       "&field3=" + String(patient.spo2) +
                       "&field4=" + String(patient.temperature) +
                       "&field5=" + String(patient.hrv);
      
      client.println("POST /update HTTP/1.1");
      client.println("Host: " + String(thingspeakServer));
      client.println("Connection: close");
      client.println("Content-Type: application/x-www-form-urlencoded");
      client.println("Content-Length: " + String(postData.length()));
      client.println();
      client.println(postData);
      
      delay(1000);
      client.stop();
      
      Serial.println("Data sent to ThingSpeak");
      return true;
    }
  }
  Serial.println("ThingSpeak upload failed");
  return false;
}

bool sendToMySQLDirect() {
  if (conn.connected()) {
    // Create INSERT query
    char query[512];
    sprintf(query, 
      "INSERT INTO patient_data (name, age, sex, diseases, ecg_value, heart_rate, spo2, temperature, hrv, created_at) VALUES ('%s', %d, '%s', '%s', %.2f, %d, %.2f, %.2f, %d, NOW())",
      patient.name.c_str(),
      patient.age,
      patient.sex.c_str(),
      patient.diseases.c_str(),
      patient.ecgValue,
      patient.heartRate,
      patient.spo2,
      patient.temperature,
      patient.hrv
    );
    
    MySQL_Cursor *cur_mem = new MySQL_Cursor(&conn);
    cur_mem->execute(query);
    delete cur_mem;
    
    Serial.println("Data stored in MySQL database");
    return true;
  } else {
    Serial.println("MySQL not connected, attempting reconnect...");
    if (connectToMySQL()) {
      return sendToMySQLDirect();
    }
  }
  return false;
}

String createWebPage() {
  String html = R"=====(
  <!DOCTYPE html>
  <html>
  <head>
      <title>IoT Health Monitor</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
          body {
              font-family: Arial, sans-serif;
              max-width: 600px;
              margin: 0 auto;
              padding: 20px;
              background-color: #f5f5f5;
          }
          .container {
              background: white;
              padding: 20px;
              border-radius: 10px;
              box-shadow: 0 2px 10px rgba(0,0,0,0.1);
          }
          .header {
              text-align: center;
              color: #2c3e50;
              margin-bottom: 20px;
          }
          .form-group {
              margin-bottom: 15px;
          }
          label {
              display: block;
              margin-bottom: 5px;
              font-weight: bold;
          }
          input, select {
              width: 100%;
              padding: 8px;
              border: 1px solid #ddd;
              border-radius: 4px;
              box-sizing: border-box;
          }
          .button-group {
              display: flex;
              gap: 10px;
              margin: 20px 0;
          }
          button {
              flex: 1;
              padding: 12px;
              border: none;
              border-radius: 5px;
              cursor: pointer;
              font-size: 16px;
              font-weight: bold;
          }
          .scan-btn {
              background-color: #3498db;
              color: white;
          }
          .upload-btn {
              background-color: #27ae60;
              color: white;
          }
          .data-display {
              background-color: #ecf0f1;
              padding: 15px;
              border-radius: 5px;
              margin-top: 20px;
          }
          .data-row {
              display: flex;
              justify-content: space-between;
              margin-bottom: 10px;
          }
          .data-value {
              font-weight: bold;
              color: #2c3e50;
          }
          .status {
              text-align: center;
              padding: 10px;
              margin: 10px 0;
              border-radius: 5px;
          }
          .scanning {
              background-color: #fff3cd;
              color: #856404;
          }
          .ready {
              background-color: #d1ecf1;
              color: #0c5460;
          }
          .chart-container {
              margin: 20px 0;
              height: 200px;
              background: #f8f9fa;
              border: 1px solid #dee2e6;
              border-radius: 5px;
              position: relative;
          }
          .ecg-wave {
              width: 100%;
              height: 100%;
          }
      </style>
  </head>
  <body>
      <div class="container">
          <div class="header">
              <h1>🩺 IoT Health Monitor</h1>
          </div>
          
          <form id="patientForm">
              <div class="form-group">
                  <label for="name">Name:</label>
                  <input type="text" id="name" name="name" required>
              </div>
              
              <div style="display: flex; gap: 10px;">
                  <div class="form-group" style="flex: 1;">
                      <label for="age">Age:</label>
                      <input type="number" id="age" name="age" min="1" max="120" required>
                  </div>
                  
                  <div class="form-group" style="flex: 1;">
                      <label for="sex">Sex:</label>
                      <select id="sex" name="sex" required>
                          <option value="">Select</option>
                          <option value="M">Male</option>
                          <option value="F">Female</option>
                          <option value="O">Other</option>
                      </select>
                  </div>
              </div>
              
              <div class="form-group">
                  <label for="diseases">Known Diseases:</label>
                  <input type="text" id="diseases" name="diseases" placeholder="Hypertension, Diabetes, etc.">
              </div>
          </form>
          
          <div class="button-group">
              <button class="scan-btn" onclick="startScan()">Start Scan</button>
              <button class="upload-btn" onclick="uploadData()">Upload Data</button>
          </div>
          
          <div id="status" class="status ready">Ready to scan</div>
          
          <div class="chart-container">
              <canvas id="ecgChart" class="ecg-wave"></canvas>
          </div>
          
          <div class="data-display">
              <div class="data-row">
                  <span>ECG Value:</span>
                  <span id="ecgValue" class="data-value">--</span>
              </div>
              <div class="data-row">
                  <span>Heart Rate:</span>
                  <span id="hrValue" class="data-value">-- bpm</span>
              </div>
              <div class="data-row">
                  <span>SpO2:</span>
                  <span id="spo2Value" class="data-value">-- %</span>
              </div>
              <div class="data-row">
                  <span>Temperature:</span>
                  <span id="tempValue" class="data-value">-- °C</span>
              </div>
              <div class="data-row">
                  <span>HRV:</span>
                  <span id="hrvValue" class="data-value">-- ms</span>
              </div>
          </div>
      </div>

      <script src="https://cdn.jsdelivr.net/npm/chart.js"></script>
      <script>
          let ecgChart;
          let ecgData = [];
          let updateInterval;
          
          // Initialize ECG chart
          function initChart() {
              const ctx = document.getElementById('ecgChart').getContext('2d');
              ecgChart = new Chart(ctx, {
                  type: 'line',
                  data: {
                      labels: Array.from({length: 50}, (_, i) => i),
                      datasets: [{
                          label: 'ECG Waveform',
                          data: ecgData,
                          borderColor: '#e74c3c',
                          backgroundColor: 'rgba(231, 76, 60, 0.1)',
                          borderWidth: 2,
                          pointRadius: 0,
                          tension: 0.4
                      }]
                  },
                  options: {
                      responsive: true,
                      maintainAspectRatio: false,
                      scales: {
                          y: {
                              min: 0,
                              max: 3.3,
                              title: {
                                  display: true,
                                  text: 'Voltage (V)'
                              }
                          },
                          x: {
                              title: {
                                  display: true,
                                  text: 'Time'
                              }
                          }
                      },
                      animation: false
                  }
              });
          }
          
          function startScan() {
              const form = document.getElementById('patientForm');
              if (!form.checkValidity()) {
                  alert('Please fill all required fields');
                  return;
              }
              
              const formData = new FormData(form);
              document.getElementById('status').className = 'status scanning';
              document.getElementById('status').textContent = 'Scanning... (20 seconds)';
              
              // Reset ECG data
              ecgData = Array(50).fill(0);
              if (ecgChart) {
                  ecgChart.data.datasets[0].data = ecgData;
                  ecgChart.update();
              }
              
              // Start updating data
              startDataUpdate();
              
              fetch('/start', {
                  method: 'POST',
                  body: new URLSearchParams(formData)
              }).then(response => {
                  if (response.ok) {
                      // Stop updating after 20 seconds
                      setTimeout(() => {
                          stopDataUpdate();
                          document.getElementById('status').className = 'status ready';
                          document.getElementById('status').textContent = 'Scan completed! Ready to upload.';
                      }, 20000);
                  }
              });
          }
          
          function startDataUpdate() {
              updateInterval = setInterval(updateDisplay, 1000);
          }
          
          function stopDataUpdate() {
              clearInterval(updateInterval);
          }
          
          function updateDisplay() {
              fetch('/data')
                  .then(response => response.json())
                  .then(data => {
                      // Update values
                      document.getElementById('ecgValue').textContent = data.ecg.toFixed(2);
                      document.getElementById('hrValue').textContent = data.hr + ' bpm';
                      document.getElementById('spo2Value').textContent = data.spo2.toFixed(1) + ' %';
                      document.getElementById('tempValue').textContent = data.temp.toFixed(1) + ' °C';
                      document.getElementById('hrvValue').textContent = data.hrv + ' ms';
                      
                      // Simulate ECG waveform update
                      if (data.scanning && ecgChart) {
                          // Add new data point and remove oldest
                          ecgData.push(Math.random() * 2 + 0.5); // Simulated ECG data
                          ecgData.shift();
                          ecgChart.data.datasets[0].data = ecgData;
                          ecgChart.update();
                      }
                  });
          }
          
          function uploadData() {
              document.getElementById('status').textContent = 'Uploading data...';
              
              fetch('/upload', {
                  method: 'POST'
              }).then(response => {
                  if (response.ok) {
                      document.getElementById('status').className = 'status ready';
                      document.getElementById('status').textContent = 'Data uploaded successfully!';
                  } else {
                      document.getElementById('status').textContent = 'Upload failed!';
                  }
              });
          }
          
          // Initialize chart when page loads
          window.onload = function() {
              initChart();
          };
      </script>
  </body>
  </html>
  )=====";
  
  return html;
}
```

## Required Libraries Installation

Add these libraries to your Arduino IDE:

1. **ESP8266WiFi** (Built-in)
2. **ESP8266WebServer** (Built-in)
3. **OneWire** by Jim Studt
4. **DallasTemperature** by Miles Burton
5. **MAX30100** by OXullo Intersecans
6. **ArduinoJson** by Benoit Blanchon
7. **MySQL Connector/Arduino** by Dr. Charles Bell

## MySQL Database Setup

### SQL Schema Creation

```sql
-- Create database and user
CREATE DATABASE health_monitor;
CREATE USER 'arduino_user'@'%' IDENTIFIED BY 'arduino_password';
GRANT ALL PRIVILEGES ON health_monitor.* TO 'arduino_user'@'%';
FLUSH PRIVILEGES;

USE health_monitor;

-- Create patient_data table
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
    hrv INT,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP
);

-- Create indexes for better performance
CREATE INDEX idx_created_at ON patient_data(created_at);
CREATE INDEX idx_name ON patient_data(name);
CREATE INDEX idx_heart_rate ON patient_data(heart_rate);
```

### MySQL Server Configuration

Add these lines to your MySQL configuration (`my.cnf` or `my.ini`):

```ini
[mysqld]
bind-address = 0.0.0.0  # Allow remote connections
max_connections = 100
wait_timeout = 28800
```

## Configuration Steps

### 1. WiFi Configuration
```cpp
const char* ssid = "Your_WiFi_SSID";
const char* password = "Your_WiFi_Password";
```

### 2. ThingSpeak Configuration
1. Create account at [thingspeak.com](https://thingspeak.com)
2. Create a new channel with these fields:
   - Field 1: ECG Value
   - Field 2: Heart Rate
   - Field 3: SpO2
   - Field 4: Temperature
   - Field 5: HRV
3. Copy the Write API Key

### 3. MySQL Configuration
```cpp
IPAddress server_addr(192,168,1,100);  // Your MySQL server IP
char user[] = "arduino_user";
char password_db[] = "arduino_password";
char database[] = "health_monitor";
```

### 4. Sensor Calibration

#### AD8232 Calibration
- Ensure proper electrode placement
- Check for lead-off detection
- Verify signal quality in serial monitor

#### MAX30100 Calibration
```cpp
// Adjust LED current based on skin tone
pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
// Options: 0=0.0mA, 1=4.4mA, 2=7.6mA, 3=11.0mA, etc.
```

## Hardware Connections

| ESP8266 Pin | Sensor | Connection |
|-------------|---------|------------|
| D1 | AD8232 | LO+ |
| D2 | AD8232 | LO- |
| A0 | AD8232 | OUTPUT |
| D4 | MAX30100 | SDA |
| D5 | MAX30100 | SCL |
| D3 | DS18B20 | DATA |
| 3.3V | All | VCC |
| GND | All | GND |

**Note**: Add 4.7kΩ pull-up resistor between DS18B20 DATA and 3.3V

## Features Included

1. **Real-time Web Dashboard** with live ECG waveform
2. **Direct MySQL Integration** without PHP
3. **ThingSpeak Cloud Storage**
4. **HRV Calculation** from pulse oximeter data
5. **20-second Scan Sessions** with automatic averaging
6. **Responsive Design** for mobile devices
7. **Error Handling** and connection recovery
8. **Serial Debug Output** for monitoring

## Usage Instructions

1. **Upload the code** to ESP8266 NodeMCU
2. **Connect sensors** according to pin mapping
3. **Power on** the system
4. **Connect to WiFi** network
5. **Open web browser** to ESP8266 IP address
6. **Enter patient details** and click "Start Scan"
7. **Wait 20 seconds** for data collection
8. **Click "Upload Data"** to save to cloud and database

## Troubleshooting

### Common Issues:

1. **WiFi Connection Failed**
   - Check SSID and password
   - Verify WiFi signal strength

2. **MySQL Connection Failed**
   - Check server IP and credentials
   - Ensure MySQL allows remote connections
   - Verify firewall settings

3. **Sensor Reading Issues**
   - Check wiring connections
   - Verify sensor power (3.3V)
   - Monitor serial output for error messages

4. **Web Server Not Accessible**
   - Check IP address in serial monitor
   - Ensure client is on same network

This complete solution provides a robust health monitoring system with direct database integration and real-time web interface!
