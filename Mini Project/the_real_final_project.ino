// --- Include Libraries ---
#include <SPI.h>
#include <MFRC522.h>
#include <SoftwareSerial.h>
#include <LiquidCrystal_I2C.h>
#include <Stepper.h>

// --- RFID and Bluetooth ---
#define SS_PIN 53
#define RST_PIN 5
#define BTSerial Serial1
MFRC522 rfid(SS_PIN, RST_PIN);
SoftwareSerial mySerial(19, 18);
#define STEPS 2048
Stepper myStepper(STEPS, 8, 10, 9, 11);

// --- LCD ---
LiquidCrystal_I2C lcd(0x27, 20, 4);

// --- Pins ---
const int startButton = 22;
const int forceStopButton = 23; // <-- Force stop button added
const int ledWash = 24, ledRinse = 25, ledSpin = 26, buzzer = 6;

// --- State Machine ---
enum MachineState { WAIT_START, WAIT_CARD, WAIT_COMMAND, RUNNING_CYCLE, COMPLETE };
MachineState state = WAIT_START;

// --- Global Variables ---
String users[] = {"79 E2 4D 05", "43 A8 E5 0F"};
int balances[] = {10, 5};
String currentUID = "";
String inputCommand = "";
unsigned long cycleStartTime = 0;
unsigned long currentMillis;
int cycleStep = 0;
bool buzzerPlayed = false;
int cycleDurations[3][3] = {{10, 5, 7}, {15, 7, 10}, {20, 10, 15}};
int remainingSecs = 0;

// --- Setup ---
void setup() {
  Serial.begin(9600);
  Serial1.begin(9600);
  SPI.begin();
  rfid.PCD_Init();
  lcd.init(); lcd.backlight();
  myStepper.setSpeed(10);

  pinMode(startButton, INPUT_PULLUP);
  pinMode(forceStopButton, INPUT_PULLUP);  // <-- Initialize force stop pin
  pinMode(ledWash, OUTPUT);
  pinMode(ledRinse, OUTPUT);
  pinMode(ledSpin, OUTPUT);
  pinMode(buzzer, OUTPUT);

  lcd.setCursor(0, 0);
  lcd.print("Push the Button ");
  lcd.setCursor(0, 1);
  lcd.print("to Start");
  Serial1.print("Push the Button to Start\n");
}

// --- Loop ---
void loop() {
  currentMillis = millis();

  // Check force stop button
  if (digitalRead(forceStopButton) == LOW) {
    forceStopProcess();
    return;  // Exit loop early to avoid running other states
  }

  handleBluetoothCommands();

  switch (state) {
    case WAIT_START:
      if (digitalRead(startButton) == LOW) {
        delay(100);
        if (digitalRead(startButton) == LOW) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Welcome to Self");
          lcd.setCursor(0, 1);
          lcd.print("Service Laundry");
          delay(2000);
          lcd.clear();
          lcd.print("Please tap card");
          Serial1.print("Please tap card\n");
          state = WAIT_CARD;
        }
      }
      break;

    case WAIT_CARD:
      if (rfid.PICC_IsNewCardPresent() && rfid.PICC_ReadCardSerial()) {
        currentUID = getUID(rfid.uid);
        int idx = findUserIndex(currentUID);
        if (idx != -1) {
          lcd.clear();
          lcd.setCursor(0, 0);
          lcd.print("Choose A/B/C/D");
          lcd.setCursor(0, 1);
          lcd.print("Balance: $");
          lcd.print(balances[idx]);
          Serial1.println("Card accepted. Waiting for command.");
          Serial1.println("Choose A (normal), B (warm), C (high), or D (reload)");
          state = WAIT_COMMAND;
        } else {
          lcd.clear(); lcd.print("Unknown Card");
          delay(2000);
          lcd.clear(); lcd.print("Please tap card");
        }
        rfid.PICC_HaltA(); rfid.PCD_StopCrypto1();
      }
      break;

    case WAIT_COMMAND:
      // Bluetooth command waiting handled separately
      break;

    case RUNNING_CYCLE:
      runWashingCycle();
      break;

    case COMPLETE:
      if (!buzzerPlayed) {
        playEndTune();
        buzzerPlayed = true;
      }
      lcd.setCursor(0, 1); lcd.print("Ready to Unload");
      delay(3000);
      lcd.clear();
      lcd.print("Push the Button ");
      lcd.setCursor(0, 1);
      lcd.print("to Start");
      currentUID = "";
      buzzerPlayed = false;
      state = WAIT_START;
      break;
  }
}

// --- Force Stop Logic ---
void forceStopProcess() {
  // Sound emergency alert
  for (int i = 0; i < 3; i++) {
    tone(buzzer, 1000);  // High-pitched beep
    delay(200);
    noTone(buzzer);
    delay(100);
  }

  // Turn off all processes
  digitalWrite(ledWash, LOW);
  digitalWrite(ledRinse, LOW);
  digitalWrite(ledSpin, LOW);

  state = WAIT_START;
  currentUID = "";
  inputCommand = "";
  cycleStep = 0;
  remainingSecs = 0;

  lcd.clear();
  lcd.print("Process Stopped");
  delay(1500);

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Push the Button ");
  lcd.setCursor(0, 1);
  lcd.print("to Start");

  Serial1.println("Process force stopped. Waiting to start.");
}


// --- Command & Cycle Logic ---
void parseCommand(String cmd) {
  cmd.trim(); cmd.toUpperCase();
  int userIdx = findUserIndex(currentUID);

  if (cmd == "A" || cmd == "B" || cmd == "C") {
    int option = cmd[0] - 'A';
    int cost = option == 0 ? 3 : (option == 1 ? 4 : 6);
    if (balances[userIdx] >= cost) {
      balances[userIdx] -= cost;
      lcd.setCursor(0, 1);
      lcd.print("Paid $"); lcd.print(cost);lcd.print("        ");
      lcd.setCursor(0, 2);
      lcd.print("Balance: $"); lcd.print(balances[userIdx]);lcd.print("     ");
      Serial1.print("Paid $"); Serial1.print(cost);
      Serial1.print(". Balance: $"); Serial1.println(balances[userIdx]);
      delay(2000);
      lcd.setCursor(0, 1);
      lcd.print("             ");
      lcd.setCursor(0, 2);
      lcd.print("                 ");
      //delay(1000);
      startWashingCycle(option);
    } else {
      Serial1.println("Insufficient balance.");
      lcd.setCursor(0, 1); lcd.print("Insufficient balance");
      delay(2000);
      lcd.setCursor(0, 1);
      lcd.print("                    ");
      lcd.setCursor(0, 2);
      lcd.print("                 ");
      //delay(1000);
    }
  } else if (cmd.startsWith("D ")) {
    int amt = cmd.substring(2).toInt();
    if (amt > 0) {
      balances[userIdx] += amt;
      Serial1.print("Reloaded $"); Serial1.print(amt);
      Serial1.print(". New balance: $"); Serial1.println(balances[userIdx]);
      lcd.setCursor(0, 1); lcd.print("Reloaded $"); lcd.print(amt);
      
      lcd.setCursor(0, 2); lcd.print("New balance: $"); lcd.print(balances[userIdx]); lcd.print("   ");
      delay(2000);
    }
  } else {
    Serial1.println("Invalid command.");
    lcd.setCursor(0, 1); lcd.print("Invalid command.");
  }
}

void startWashingCycle(int type) {
  lcd.clear(); lcd.print("Washing...");
  playStartTune();
  cycleStartTime = millis();
  cycleStep = 0;
  remainingSecs = cycleDurations[type][cycleStep];
  digitalWrite(ledWash, HIGH);
  state = RUNNING_CYCLE;
}

void runWashingCycle() {
  static unsigned long lastTick = 0;
  if (currentMillis - lastTick >= 1000 && remainingSecs > 0) {
    lastTick = currentMillis;
    remainingSecs--;
    myStepper.step(10);
    lcd.setCursor(0, 1); lcd.print("Time: ");
    lcd.print(remainingSecs); lcd.print("s   ");
  }

  if (remainingSecs <= 0) {
    digitalWrite(ledWash, LOW);
    digitalWrite(ledRinse, LOW);
    digitalWrite(ledSpin, LOW);
    cycleStep++;
    if (cycleStep >= 3) {
      lcd.clear(); lcd.print("Cycle Complete!");
      Serial1.print("Ready to Unload");
      state = COMPLETE;
    } else {
      int option = inputCommand == "A" ? 0 : (inputCommand == "B" ? 1 : 2);
      remainingSecs = cycleDurations[option][cycleStep];
      lcd.clear();
      switch (cycleStep) {
        case 1: lcd.print("Rinsing..."); digitalWrite(ledRinse, HIGH); break;
        case 2: lcd.print("Spinning..."); digitalWrite(ledSpin, HIGH); break;
      }
    }
  }
}

// --- Utilities ---
String getUID(MFRC522::Uid uid) {
  String uidStr = "";
  for (byte i = 0; i < uid.size; i++) {
    if (uid.uidByte[i] < 0x10) uidStr += "0";
    uidStr += String(uid.uidByte[i], HEX);
    if (i < uid.size - 1) uidStr += " ";
  }
  uidStr.toUpperCase(); uidStr.trim();
  return uidStr;
}

int findUserIndex(String uid) {
  for (int i = 0; i < sizeof(users) / sizeof(users[0]); i++) {
    if (uid == users[i]) return i;
  }
  return -1;
}

void handleBluetoothCommands() {
  while (Serial1.available()) {
    char c = Serial1.read();
    if (c == '\n' || c == '\r') {
      if (inputCommand.length() > 0) {
        parseCommand(inputCommand);
        inputCommand = "";
      }
    } else {
      inputCommand += c;
    }
  }
}

void playStartTune() {
  tone(buzzer, 523); delay(150);
  tone(buzzer, 659); delay(150);
  tone(buzzer, 784); delay(200);
  noTone(buzzer);
}

void playEndTune() {
  tone(buzzer, 784); delay(150);
  tone(buzzer, 659); delay(150);
  tone(buzzer, 523); delay(200);
  noTone(buzzer);
}
