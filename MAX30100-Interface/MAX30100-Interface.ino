#include <Wire.h>
#include "MAX30100_PulseOximeter.h"

#define REPORTING_PERIOD_MS     1000

PulseOximeter pox;
uint32_t tsLastReport = 0;

void onBeatDetected() {
    Serial.println("♥ Beat Detected!");
}

void setup() {
    Serial.begin(115200);
    Serial.println("Initializing MAX30100...");

    if (!pox.begin()) {
        Serial.println("MAX30100 initialization failed!");
        while(1);
    }
    
    pox.setIRLedCurrent(MAX30100_LED_CURR_7_6MA);
    pox.setOnBeatDetectedCallback(onBeatDetected);
    
    Serial.println("MAX30100 initialized successfully!");
}

void loop() {
    pox.update();
    
    if (millis() - tsLastReport > REPORTING_PERIOD_MS) {
        Serial.print("Heart rate: ");
        Serial.println(pox.getHeartRate());
        Serial.print(" bpm / SpO2: ");
        Serial.println(pox.getSpO2());
        Serial.println("%");
        Serial.println(pox.getHeartRate());
        Serial.println(pox.getSpO2());
        tsLastReport = millis();
    }

    
}
