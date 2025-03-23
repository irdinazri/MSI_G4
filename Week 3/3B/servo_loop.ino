#include <Servo.h>

Servo servo;

int angle = 90;

void setup() 
{
  Serial.begin (9600);
  servo.attach(5);
}

void loop() 
{
  servo.write(angle);
  delay(1000);  
  
  if (angle == 90) {
    angle = 180;
  } else {
    angle = 90;
  }
  Serial.println (angle);
}