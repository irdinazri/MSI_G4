#include <SoftwareSerial.h>

const int lm35Pin = A0; // LM35 connected to A0
SoftwareSerial bluetooth(3, 2); // RX, TX for HC-05

void setup() {
  Serial.begin(9600);
  // bluetooth.begin(9600);
}

void loop() {
  int sensorValue = analogRead(lm35Pin);
  float voltage = sensorValue * (5.0 / 1023.0); // Convert to voltage
  float temperatureC = voltage * 100; // LM35 outputs 10mV/°C

  // Send temperature to Bluetooth
  // bluetooth.print("Temperature: ");
  // bluetooth.print(temperatureC);
  // bluetooth.println(" °C");
  Serial.print("Temperature: ");
  Serial.print(temperatureC);
  Serial.println(" °C");

  delay(1000); // Send every 1 second
}
