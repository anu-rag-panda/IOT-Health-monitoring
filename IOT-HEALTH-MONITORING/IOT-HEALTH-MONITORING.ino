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
const char* ssid = "MY_SSID";
const char* password = "anuragini1234";

// ThingSpeak Configuration
const char* thingspeakServer = "api.thingspeak.com";
const String thingspeakAPIKey = "YOUR_THINGSPEAK_API_KEY";

// MySQL Database Configuration (Direct Connection)
IPAddress server_addr(127,0,0,1);  // Change to your MySQL server IP
char user[] = "root";            // MySQL username
char password_db[] = "anurag@panda"; // MySQL password
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
  // Personal Information
  String name = "";
  int age = 0;
  String sex = "";
  String bloodGroup = "";
  int height = 0;      // in cm
  float weight = 0.0;  // in kg
  String diseases = "";
  
  // Vital Signs
  float ecgValue = 0.0;
  int heartRate = 0;
  float spo2 = 0.0;
  float temperature = 0.0;
  int hrv = 0;
  
  // Calculated Metrics
  float bmi = 0.0;
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
  
  Serial.println("Enhanced Health Monitoring System Ready!");
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
      calculateBMI();
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

bool connectToMySQL() {
  Serial.print("Connecting to MySQL database...");
  
  if (conn.connect(server_addr, 3306, user, password_db)) {
    Serial.println("MySQL connected successfully!");
    return true;
  } else {
    Serial.println("MySQL connection failed!");
    return false;
  }
}

void setupWebServer() {
  server.on("/", HTTP_GET, handleRoot);
  server.on("/start", HTTP_POST, handleStartScan);
  server.on("/upload", HTTP_POST, handleUploadData);
  server.on("/data", HTTP_GET, handleGetData);
  server.on("/patient", HTTP_GET, handleGetPatientData);
  
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
  patient.bloodGroup = server.arg("bloodGroup");
  patient.height = server.arg("height").toInt();
  patient.weight = server.arg("weight").toFloat();
  patient.diseases = server.arg("diseases");
  
  // Reset sensor data
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
  jsonDoc["bmi"] = patient.bmi;
  jsonDoc["scanning"] = isScanning;
  
  String jsonString;
  serializeJson(jsonDoc, jsonString);
  
  server.send(200, "application/json", jsonString);
}

void handleGetPatientData() {
  StaticJsonDocument<1024> jsonDoc;
  jsonDoc["name"] = patient.name;
  jsonDoc["age"] = patient.age;
  jsonDoc["sex"] = patient.sex;
  jsonDoc["bloodGroup"] = patient.bloodGroup;
  jsonDoc["height"] = patient.height;
  jsonDoc["weight"] = patient.weight;
  jsonDoc["diseases"] = patient.diseases;
  jsonDoc["bmi"] = patient.bmi;
  
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

void calculateBMI() {
  if (patient.height > 0 && patient.weight > 0) {
    float heightInMeters = patient.height / 100.0;
    patient.bmi = patient.weight / (heightInMeters * heightInMeters);
    Serial.println("BMI calculated: " + String(patient.bmi));
  }
}

String getBMICategory(float bmi) {
  if (bmi < 18.5) return "Underweight";
  else if (bmi < 25) return "Normal";
  else if (bmi < 30) return "Overweight";
  else return "Obese";
}

void resetSensorData() {
  patient.ecgValue = 0.0;
  patient.heartRate = 0;
  patient.spo2 = 0.0;
  patient.temperature = 0.0;
  patient.hrv = 0;
  patient.bmi = 0.0;
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
                       "&field5=" + String(patient.hrv) +
                       "&field6=" + String(patient.bmi);
      
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
      "INSERT INTO patient_data (name, age, sex, blood_group, height, weight, diseases, ecg_value, heart_rate, spo2, temperature, hrv, bmi, created_at) VALUES ('%s', %d, '%s', '%s', %d, %.2f, '%s', %.2f, %d, %.2f, %.2f, %d, %.2f, NOW())",
      patient.name.c_str(),
      patient.age,
      patient.sex.c_str(),
      patient.bloodGroup.c_str(),
      patient.height,
      patient.weight,
      patient.diseases.c_str(),
      patient.ecgValue,
      patient.heartRate,
      patient.spo2,
      patient.temperature,
      patient.hrv,
      patient.bmi
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
      <title>Enhanced IoT Health Monitor</title>
      <meta name="viewport" content="width=device-width, initial-scale=1">
      <style>
          body {
              font-family: 'Segoe UI', Tahoma, Geneva, Verdana, sans-serif;
              max-width: 800px;
              margin: 0 auto;
              padding: 20px;
              background: linear-gradient(135deg, #667eea 0%, #764ba2 100%);
              min-height: 100vh;
          }
          .container {
              background: white;
              padding: 30px;
              border-radius: 15px;
              box-shadow: 0 10px 30px rgba(0,0,0,0.2);
          }
          .header {
              text-align: center;
              color: #2c3e50;
              margin-bottom: 30px;
              border-bottom: 2px solid #3498db;
              padding-bottom: 15px;
          }
          .form-group {
              margin-bottom: 20px;
          }
          label {
              display: block;
              margin-bottom: 8px;
              font-weight: 600;
              color: #34495e;
          }
          input, select {
              width: 100%;
              padding: 12px;
              border: 2px solid #bdc3c7;
              border-radius: 8px;
              box-sizing: border-box;
              font-size: 16px;
              transition: border-color 0.3s;
          }
          input:focus, select:focus {
              border-color: #3498db;
              outline: none;
          }
          .form-row {
              display: flex;
              gap: 15px;
              margin-bottom: 15px;
          }
          .form-col {
              flex: 1;
          }
          .button-group {
              display: flex;
              gap: 15px;
              margin: 30px 0;
          }
          button {
              flex: 1;
              padding: 15px;
              border: none;
              border-radius: 8px;
              cursor: pointer;
              font-size: 16px;
              font-weight: bold;
              transition: all 0.3s;
          }
          .scan-btn {
              background: linear-gradient(135deg, #3498db, #2980b9);
              color: white;
          }
          .scan-btn:hover {
              background: linear-gradient(135deg, #2980b9, #21618c);
              transform: translateY(-2px);
          }
          .upload-btn {
              background: linear-gradient(135deg, #27ae60, #219a52);
              color: white;
          }
          .upload-btn:hover {
              background: linear-gradient(135deg, #219a52, #1e8449);
              transform: translateY(-2px);
          }
          .data-display {
              background: linear-gradient(135deg, #ecf0f1, #bdc3c7);
              padding: 25px;
              border-radius: 10px;
              margin-top: 25px;
          }
          .data-row {
              display: flex;
              justify-content: space-between;
              margin-bottom: 12px;
              padding: 10px;
              background: white;
              border-radius: 6px;
              box-shadow: 0 2px 5px rgba(0,0,0,0.1);
          }
          .data-label {
              font-weight: 600;
              color: #2c3e50;
          }
          .data-value {
              font-weight: bold;
              color: #e74c3c;
          }
          .data-normal { color: #27ae60; }
          .data-warning { color: #f39c12; }
          .data-critical { color: #e74c3c; }
          .status {
              text-align: center;
              padding: 15px;
              margin: 20px 0;
              border-radius: 8px;
              font-weight: bold;
          }
          .scanning {
              background: linear-gradient(135deg, #fff3cd, #ffeaa7);
              color: #856404;
              border: 2px solid #ffeaa7;
          }
          .ready {
              background: linear-gradient(135deg, #d1ecf1, #a2d9ce);
              color: #0c5460;
              border: 2px solid #a2d9ce;
          }
          .chart-container {
              margin: 25px 0;
              height: 200px;
              background: #f8f9fa;
              border: 2px solid #dee2e6;
              border-radius: 10px;
              position: relative;
          }
          .section-title {
              color: #2c3e50;
              border-left: 4px solid #3498db;
              padding-left: 15px;
              margin: 25px 0 15px 0;
          }
          .patient-info {
              background: #f8f9fa;
              padding: 20px;
              border-radius: 10px;
              margin-bottom: 20px;
          }
          .bmi-indicator {
              display: inline-block;
              padding: 4px 12px;
              border-radius: 20px;
              font-size: 12px;
              font-weight: bold;
              margin-left: 10px;
          }
          .bmi-underweight { background: #3498db; color: white; }
          .bmi-normal { background: #27ae60; color: white; }
          .bmi-overweight { background: #f39c12; color: white; }
          .bmi-obese { background: #e74c3c; color: white; }
      </style>
  </head>
  <body>
      <div class="container">
          <div class="header">
              <h1>🏥 Enhanced IoT Health Monitor</h1>
              <p>Complete Patient Health Assessment System</p>
          </div>
          
          <div class="patient-info">
              <h3 class="section-title">Patient Demographics</h3>
              <form id="patientForm">
                  <div class="form-row">
                      <div class="form-col">
                          <label for="name">Full Name:</label>
                          <input type="text" id="name" name="name" placeholder="Enter patient name" required>
                      </div>
                      <div class="form-col">
                          <label for="age">Age:</label>
                          <input type="number" id="age" name="age" min="1" max="120" placeholder="Years" required>
                      </div>
                  </div>
                  
                  <div class="form-row">
                      <div class="form-col">
                          <label for="sex">Sex:</label>
                          <select id="sex" name="sex" required>
                              <option value="">Select Gender</option>
                              <option value="M">Male</option>
                              <option value="F">Female</option>
                              <option value="O">Other</option>
                          </select>
                      </div>
                      <div class="form-col">
                          <label for="bloodGroup">Blood Group:</label>
                          <select id="bloodGroup" name="bloodGroup" required>
                              <option value="">Select Blood Group</option>
                              <option value="A+">A+</option>
                              <option value="A-">A-</option>
                              <option value="B+">B+</option>
                              <option value="B-">B-</option>
                              <option value="AB+">AB+</option>
                              <option value="AB-">AB-</option>
                              <option value="O+">O+</option>
                              <option value="O-">O-</option>
                          </select>
                      </div>
                  </div>
                  
                  <div class="form-row">
                      <div class="form-col">
                          <label for="height">Height (cm):</label>
                          <input type="number" id="height" name="height" min="50" max="250" placeholder="Centimeters" required>
                      </div>
                      <div class="form-col">
                          <label for="weight">Weight (kg):</label>
                          <input type="number" id="weight" name="weight" min="2" max="300" step="0.1" placeholder="Kilograms" required>
                      </div>
                  </div>
                  
                  <div class="form-group">
                      <label for="diseases">Known Diseases/Conditions:</label>
                      <input type="text" id="diseases" name="diseases" placeholder="Hypertension, Diabetes, Asthma, etc.">
                  </div>
              </form>
          </div>
          
          <div class="button-group">
              <button class="scan-btn" onclick="startScan()">🩺 Start Health Scan</button>
              <button class="upload-btn" onclick="uploadData()">📊 Upload Patient Data</button>
          </div>
          
          <div id="status" class="status ready">System Ready - Enter patient details to begin</div>
          
          <div class="chart-container">
              <canvas id="ecgChart" class="ecg-wave"></canvas>
          </div>
          
          <h3 class="section-title">Vital Signs & Health Metrics</h3>
          <div class="data-display">
              <div class="data-row">
                  <span class="data-label">ECG Value:</span>
                  <span id="ecgValue" class="data-value">-- mV</span>
              </div>
              <div class="data-row">
                  <span class="data-label">Heart Rate:</span>
                  <span id="hrValue" class="data-value">-- bpm</span>
              </div>
              <div class="data-row">
                  <span class="data-label">SpO2 Level:</span>
                  <span id="spo2Value" class="data-value">-- %</span>
              </div>
              <div class="data-row">
                  <span class="data-label">Body Temperature:</span>
                  <span id="tempValue" class="data-value">-- °C</span>
              </div>
              <div class="data-row">
                  <span class="data-label">Heart Rate Variability:</span>
                  <span id="hrvValue" class="data-value">-- ms</span>
              </div>
              <div class="data-row">
                  <span class="data-label">BMI (Body Mass Index):</span>
                  <span id="bmiValue" class="data-value">-- 
                      <span id="bmiCategory" class="bmi-indicator"></span>
                  </span>
              </div>
          </div>
          
          <div id="patientSummary" style="margin-top: 20px; display: none;">
              <h3 class="section-title">Patient Summary</h3>
              <div class="data-display">
                  <div class="data-row">
                      <span class="data-label">Name:</span>
                      <span id="summaryName" class="data-value">--</span>
                  </div>
                  <div class="data-row">
                      <span class="data-label">Age/Sex:</span>
                      <span id="summaryAgeSex" class="data-value">--</span>
                  </div>
                  <div class="data-row">
                      <span class="data-label">Blood Group:</span>
                      <span id="summaryBloodGroup" class="data-value">--</span>
                  </div>
                  <div class="data-row">
                      <span class="data-label">Height/Weight:</span>
                      <span id="summaryHeightWeight" class="data-value">--</span>
                  </div>
                  <div class="data-row">
                      <span class="data-label">Medical Conditions:</span>
                      <span id="summaryDiseases" class="data-value">--</span>
                  </div>
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
                      labels: Array.from({length: 50}, (_, i) => ''),
                      datasets: [{
                          label: 'ECG Waveform',
                          data: ecgData,
                          borderColor: '#e74c3c',
                          backgroundColor: 'rgba(231, 76, 60, 0.1)',
                          borderWidth: 2,
                          pointRadius: 0,
                          tension: 0.4,
                          fill: true
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
                              },
                              grid: {
                                  color: 'rgba(0,0,0,0.1)'
                              }
                          },
                          x: {
                              title: {
                                  display: true,
                                  text: 'Time'
                              },
                              grid: {
                                  color: 'rgba(0,0,0,0.1)'
                              }
                          }
                      },
                      animation: false,
                      plugins: {
                          legend: {
                              display: true,
                              position: 'top'
                          }
                      }
                  }
              });
          }
          
          function getBMICategory(bmi) {
              if (bmi < 18.5) return ['Underweight', 'bmi-underweight'];
              else if (bmi < 25) return ['Normal', 'bmi-normal'];
              else if (bmi < 30) return ['Overweight', 'bmi-overweight'];
              else return ['Obese', 'bmi-obese'];
          }
          
          function startScan() {
              const form = document.getElementById('patientForm');
              if (!form.checkValidity()) {
                  alert('Please fill all required patient details');
                  return;
              }
              
              const formData = new FormData(form);
              document.getElementById('status').className = 'status scanning';
              document.getElementById('status').textContent = 'Scanning vital signs... (20 seconds)';
              
              // Show patient summary
              updatePatientSummary();
              document.getElementById('patientSummary').style.display = 'block';
              
              // Reset ECG data
              ecgData = Array(50).fill(1.0);
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
                          document.getElementById('status').textContent = 'Scan completed! Ready to upload data.';
                      }, 20000);
                  }
              });
          }
          
          function updatePatientSummary() {
              const form = document.getElementById('patientForm');
              const formData = new FormData(form);
              
              document.getElementById('summaryName').textContent = formData.get('name');
              document.getElementById('summaryAgeSex').textContent = 
                  formData.get('age') + ' years, ' + 
                  (formData.get('sex') === 'M' ? 'Male' : formData.get('sex') === 'F' ? 'Female' : 'Other');
              document.getElementById('summaryBloodGroup').textContent = formData.get('bloodGroup');
              document.getElementById('summaryHeightWeight').textContent = 
                  formData.get('height') + ' cm, ' + formData.get('weight') + ' kg';
              document.getElementById('summaryDiseases').textContent = 
                  formData.get('diseases') || 'None reported';
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
                      // Update vital signs
                      document.getElementById('ecgValue').textContent = data.ecg.toFixed(2) + ' mV';
                      document.getElementById('hrValue').textContent = data.hr + ' bpm';
                      document.getElementById('spo2Value').textContent = data.spo2.toFixed(1) + ' %';
                      document.getElementById('tempValue').textContent = data.temp.toFixed(1) + ' °C';
                      document.getElementById('hrvValue').textContent = data.hrv + ' ms';
                      
                      // Update BMI with category
                      if (data.bmi > 0) {
                          const [category, cssClass] = getBMICategory(data.bmi);
                          document.getElementById('bmiValue').textContent = data.bmi.toFixed(1);
                          const bmiCategory = document.getElementById('bmiCategory');
                          bmiCategory.textContent = category;
                          bmiCategory.className = 'bmi-indicator ' + cssClass;
                      }
                      
                      // Simulate ECG waveform update during scanning
                      if (data.scanning && ecgChart) {
                          // Simulate realistic ECG waveform
                          for (let i = 0; i < ecgData.length; i++) {
                              ecgData[i] = 1.0 + 0.5 * Math.sin(i * 0.5) + 0.2 * Math.random();
                          }
                          ecgChart.data.datasets[0].data = ecgData;
                          ecgChart.update();
                      }
                  })
                  .catch(error => {
                      console.error('Error fetching data:', error);
                  });
          }
          
          function uploadData() {
              document.getElementById('status').textContent = 'Uploading patient data to cloud...';
              
              fetch('/upload', {
                  method: 'POST'
              }).then(response => {
                  if (response.ok) {
                      document.getElementById('status').className = 'status ready';
                      document.getElementById('status').textContent = 'Patient data uploaded successfully!';
                  } else {
                      document.getElementById('status').textContent = 'Upload failed! Please try again.';
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