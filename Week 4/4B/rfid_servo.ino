#include <SPI.h>
#include <MFRC522.h>
#include <Servo.h>

#define SS_PIN 10
#define RST_PIN 9
#define GREEN_LED 7
#define RED_LED 6
#define SERVO_PIN 5

MFRC522 rfid(SS_PIN, RST_PIN);
Servo servo;

String authorizedUIDs[] = {"123456789", "987654321"};  // Replace with actual RFID UIDs
bool isAuthorized = false;

void setup() {
    Serial.begin(9600);
    SPI.begin();
    rfid.PCD_Init();
    pinMode(GREEN_LED, OUTPUT);
    pinMode(RED_LED, OUTPUT);
    servo.attach(SERVO_PIN);
    servo.write(90);  // Default position
}

void loop() {
    if (!rfid.PICC_IsNewCardPresent() || !rfid.PICC_ReadCardSerial()) {
        return;
    }

    String uid = "";
    for (byte i = 0; i < rfid.uid.size; i++) {
        uid += String(rfid.uid.uidByte[i], DEC);
    }

    Serial.println(uid);  // Send UID to Python
    isAuthorized = false;

    for (String authUID : authorizedUIDs) {
        if (uid == authUID) {
            isAuthorized = true;
            break;
        }
    }

    if (isAuthorized) {
        digitalWrite(GREEN_LED, HIGH);
        digitalWrite(RED_LED, LOW);
    } else {
        digitalWrite(RED_LED, HIGH);
        digitalWrite(GREEN_LED, LOW);
    }

    delay(2000);
    digitalWrite(GREEN_LED, LOW);
    digitalWrite(RED_LED, LOW);

    rfid.PICC_HaltA();
    rfid.PCD_StopCrypto1();
}
