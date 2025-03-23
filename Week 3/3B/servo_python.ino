#include <Servo.h>

Servo servo;

void setup() {
  Serial.begin(9600);
  servo.attach(5);
}

void loop() {
  if (Serial.available() > 0) {  // Check if data is available from Python
    int angle = Serial.parseInt();  // Read and convert to an integer
    if (angle >= 0 && angle <= 180) {  
      servo.write(angle);  // Move servo to the new angle
      Serial.print("Servo moved to: ");
      Serial.println(angle);
    }
  }
}
