#include <OneWire.h>
#include <DallasTemperature.h>

#define ONE_WIRE_BUS 2

OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);

void setup() {
    Serial.begin(115200);
    Serial.println("DS18B20 Temperature Sensor");
    
    sensors.begin();
    
    int deviceCount = sensors.getDeviceCount();
    Serial.print("Found ");
    Serial.print(deviceCount);
    Serial.println(" temperature sensors");
}

void loop() {
    sensors.requestTemperatures();
    
    for(int i = 0; i < sensors.getDeviceCount(); i++) {
        float tempC = sensors.getTempCByIndex(i);
        
        Serial.print("Sensor ");
        Serial.print(i);
        Serial.print(": ");
        Serial.print(tempC);
        Serial.print("°C | ");
        Serial.print(DallasTemperature::toFahrenheit(tempC));
        Serial.println("°F");
    }
    
    Serial.println("--------------------");
    delay(2000);
}