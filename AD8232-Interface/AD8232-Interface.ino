const int LO_PLUS = 10;    // LO+
const int LO_MINUS = 11;   // LO-
const int OUTPUT_PIN = A0; // ECG output

void setup() {
    Serial.begin(115200);
    
    // Configure LO+ and LO- as inputs for electrode detection
    pinMode(LO_PLUS, INPUT);
    pinMode(LO_MINUS, INPUT);
    pinMode(OUTPUT_PIN, INPUT);
    
    Serial.println("AD8232 ECG Sensor Initialized");
    Serial.println("Reading ECG signal...");
}

void loop() {
    // Check if electrodes are properly connected
    if ((digitalRead(LO_PLUS) == 1) || (digitalRead(LO_MINUS) == 1)) {
        Serial.println("Electrodes disconnected! Check connection.");
    } else {
        // Read ECG value
        int ecgValue = analogRead(OUTPUT_PIN);
        
        // For serial plotter visualization
        Serial.println(ecgValue);
        
        // Optional: Print raw value for debugging
        // Serial.print("ECG Raw: ");
        // Serial.println(ecgValue);
    }
    
    delay(10); // Small delay for stable reading
}
