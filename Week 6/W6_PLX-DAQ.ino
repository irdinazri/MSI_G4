void setup() {
  Serial.begin(9600);  // Start serial communication
  Serial.println("CLEARDATA");  // Clear previous data
  Serial.println("LABEL,Time,LDR Value,Temperature (°C)");  // Excel headers
}

void loop() {
  // Read LDR value from A0
  int ldrValue = analogRead(A0);

  // Read LM35 value from A1
  int lm35Value = analogRead(A1);
  float voltage = lm35Value * (5.0 / 1023.0);      // Convert to voltage
  float temperatureC = voltage * 100;              // Convert to °C

  // Send to PLX-DAQ
  Serial.print("DATA,TIME,");
  Serial.print(ldrValue);
  Serial.print(",");
  Serial.println(temperatureC);

  delay(1000);  // 1-second delay
}
