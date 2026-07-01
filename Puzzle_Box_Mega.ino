// Manage Libraries

#include <LiquidCrystal_I2C.h>
#include <Adafruit_Keypad.h>
#include <Wire.h>

// Constants for the row and column sizes
const byte ROWS = 4;
const byte COLS = 4;

// LED Constants
const int REDLEDPIN = 47;
const int GREENLEDPIN = 45;

// Pushbutton Constant
const int PUSHBUTTONPIN = 49;

// Piezo Constant
const int BUZZERPIN = 5;

// Rotary Encoder Constants
const int SWITCHPIN = 17; // Encoder push button
const int DTPIN = 18;
const int CLOCKPIN = 19;

// Array to represent keys on keypad
char hexaKeys[ROWS][COLS] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

int switchState = 0;
const bool PUSHBUTTON_ACTIVE_LOW = false;
int rotaryEncoderLastState = 0;

// Connections to Arduino
byte rowPins[ROWS] = {13, 12, 11, 10};
byte colPins[COLS] = {9, 8, 7, 6};

// Create keypad object and LiquidCrystal object
Adafruit_Keypad keypad = Adafruit_Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
LiquidCrystal_I2C lcd(0x27, 16, 2);

void setup() {
  Serial.begin(9600);
  lcd.init();
  lcd.backlight();

  lcd.setCursor(0, 0);
  lcd.print("Hello, World!");

  lcd.setCursor(0, 1);
  lcd.print("Arduino I2C LCD");

  pinMode(REDLEDPIN, OUTPUT);
  pinMode(GREENLEDPIN, OUTPUT);
  digitalWrite(REDLEDPIN, LOW);
  digitalWrite(GREENLEDPIN, LOW);

  pinMode(PUSHBUTTONPIN, INPUT);
  pinMode(BUZZERPIN, OUTPUT);
  pinMode(DTPIN, INPUT_PULLUP);
  pinMode(CLOCKPIN, INPUT_PULLUP);
  pinMode(SWITCHPIN, INPUT_PULLUP);

  rotaryEncoderLastState = digitalRead(CLOCKPIN);
  keypad.begin();
}

bool joystickMode = false;
bool pushSequenceActive = false;
unsigned long pushSequenceStart = 0;
unsigned long lastLightToggle = 0;
unsigned long lastMessageChange = 0;
bool lightRedPhase = true;
int pushMessageStep = 0;
int lastPushButtonReading = LOW;
int rotaryEncoderCount = 0;

// Note frequencies used in the song
#define NOTE_C4  262
#define NOTE_CS4 277
#define NOTE_D4  294
#define NOTE_DS4 311
#define NOTE_E4  330
#define NOTE_F4  349
#define NOTE_FS4 370
#define NOTE_G4  392
#define NOTE_GS4 415
#define NOTE_A4  440
#define NOTE_AS4 466
#define NOTE_B4  494
#define NOTE_C5  523
#define NOTE_CS5 554
#define NOTE_D5  587
#define NOTE_DS5 622
#define NOTE_E5  659
#define NOTE_F5  698
#define NOTE_FS5 740
#define NOTE_G5  784
#define NOTE_GS5 831
#define NOTE_A5  880
#define NOTE_AS5 932
#define NOTE_B5  988
#define REST     0

const int buzzerPin = BUZZERPIN;

int melody[] = {
  NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, REST,
  NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, REST,
  NOTE_C5, NOTE_D5, NOTE_B4, NOTE_B4, REST,
  NOTE_A4, NOTE_G4, NOTE_A4, REST,

  NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, REST,
  NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, REST,
  NOTE_C5, NOTE_D5, NOTE_B4, NOTE_B4, REST,
  NOTE_A4, NOTE_G4, NOTE_A4, REST,

  NOTE_E4, NOTE_G4, NOTE_A4, NOTE_A4, REST,
  NOTE_A4, NOTE_C5, NOTE_D5, NOTE_D5, REST,
  NOTE_D5, NOTE_E5, NOTE_F5, NOTE_F5, REST,
  NOTE_E5, NOTE_D5, NOTE_E5, NOTE_A4, REST,

  NOTE_A4, NOTE_B4, NOTE_C5, NOTE_C5, REST,
  NOTE_D5, NOTE_E5, NOTE_A4, REST,
  NOTE_A4, NOTE_C5, NOTE_B4, NOTE_B4, REST,
  NOTE_C5, NOTE_A4, NOTE_B4, REST,

  NOTE_A4, NOTE_A4
};

int noteDurations[] = {
  8, 8, 5, 5, 8,
  8, 8, 5, 5, 8,
  8, 8, 5, 5, 8,
  8, 8, 5, 8,

  8, 8, 5, 5, 8,
  8, 8, 5, 5, 8,
  8, 8, 5, 5, 8,
  8, 8, 5, 8,

  8, 8, 5, 5, 8,
  8, 8, 5, 5, 8,
  8, 8, 5, 5, 8,
  8, 8, 5, 5, 8,

  8, 8, 5, 5, 8,
  8, 8, 5,
  8, 8, 5, 5,
  8, 8, 5,

  5, 5
};

bool melodyPlaying = false;
int melodyIndex = 0;
unsigned long melodyStepStart = 0;
unsigned long melodyNoteOffTime = 0;
unsigned long melodyAdvanceTime = 0;
bool melodyToneActive = false;

void printWrapped(const String& text) {
  lcd.clear();

  for (int i = 0; i < text.length() && i < 32; i++) {
    if (i == 16) {
      lcd.setCursor(0, 1);
    }
    lcd.print(text[i]);
  }
}

void showJoystickEntry() {
  printWrapped("Joystick Mode Activating in 3");
  delay(1000);
  printWrapped("Joystick Mode Activating in 2");
  delay(1000);
  printWrapped("Joystick Mode Activating in 1");
  delay(1000);
  lcd.clear();
}

void showJoystickExit() {
  printWrapped("Exiting debug mode");
  delay(1000);
  lcd.clear();
}

void startPushSequence() {
  pushSequenceActive = true;
  pushSequenceStart = millis();
  lastLightToggle = millis();
  lastMessageChange = millis();
  lightRedPhase = true;
  pushMessageStep = 0;
  printWrapped("My Mayor Muslim");
}

void stopPushSequence() {
  pushSequenceActive = false;
  digitalWrite(REDLEDPIN, LOW);
  digitalWrite(GREENLEDPIN, LOW);
  lcd.clear();
}

void startCurrentMelodyNote() {
  int totalNotes = sizeof(melody) / sizeof(melody[0]);
  if (melodyIndex >= totalNotes) {
    melodyPlaying = false;
    melodyToneActive = false;
    noTone(buzzerPin);
    return;
  }

  int noteDuration = 1000 / noteDurations[melodyIndex];
  melodyStepStart = millis();
  melodyAdvanceTime = melodyStepStart + (unsigned long)(noteDuration * 1.30);

  if (melody[melodyIndex] == REST) {
    melodyToneActive = false;
    melodyNoteOffTime = 0;
    melodyAdvanceTime = melodyStepStart + (unsigned long)(noteDuration * 0.35);
    noTone(buzzerPin);
    return;
  }

  tone(buzzerPin, melody[melodyIndex], noteDuration);
  melodyToneActive = true;
  melodyNoteOffTime = melodyStepStart + noteDuration;
}

void playMelody() {
  melodyPlaying = true;
  melodyIndex = 0;
  melodyStepStart = 0;
  melodyNoteOffTime = 0;
  melodyAdvanceTime = 0;
  melodyToneActive = false;
  startCurrentMelodyNote();
}

void updateMelody() {
  if (!melodyPlaying) {
    return;
  }

  unsigned long now = millis();
  int totalNotes = sizeof(melody) / sizeof(melody[0]);

  if (melodyIndex >= totalNotes) {
    melodyPlaying = false;
    melodyToneActive = false;
    noTone(buzzerPin);
    return;
  }

  if (melodyToneActive) {
    if (now >= melodyNoteOffTime) {
      noTone(buzzerPin);
      melodyToneActive = false;
    }
  }

  if (now < melodyAdvanceTime) {
    return;
  }

  melodyIndex++;
  if (melodyIndex >= totalNotes) {
    melodyPlaying = false;
    melodyToneActive = false;
    noTone(buzzerPin);
    return;
  }

  startCurrentMelodyNote();
}

bool isPushButtonJustPressed() {
  int currentReading = digitalRead(PUSHBUTTONPIN);
  bool pressed = PUSHBUTTON_ACTIVE_LOW ? (currentReading == LOW) : (currentReading == HIGH);
  bool lastPressed = PUSHBUTTON_ACTIVE_LOW ? (lastPushButtonReading == LOW) : (lastPushButtonReading == HIGH);
  lastPushButtonReading = currentReading;
  return pressed && !lastPressed;
}

void updateRotaryEncoder() {
  int currentState = digitalRead(CLOCKPIN);

  if (currentState != rotaryEncoderLastState) {
    if (digitalRead(DTPIN) != currentState) {
      rotaryEncoderCount++;
    } else {
      rotaryEncoderCount--;
    }

    Serial.print("Encoder count: ");
    Serial.println(rotaryEncoderCount);

    if (abs(rotaryEncoderCount) >= 5) {
      rotaryEncoderCount = 0;
      playMelody();
    }
  }

  rotaryEncoderLastState = currentState;
}

void updatePushSequence() {
  if (!pushSequenceActive) {
    return;
  }

  unsigned long now = millis();

  if (now - lastLightToggle >= 100) {
    lastLightToggle = now;
    if (lightRedPhase) {
      digitalWrite(REDLEDPIN, LOW);
      digitalWrite(GREENLEDPIN, HIGH);
    } else {
      digitalWrite(GREENLEDPIN, LOW);
      digitalWrite(REDLEDPIN, HIGH);
    }
    lightRedPhase = !lightRedPhase;
  }

  if (now - lastMessageChange >= 1000) {
    lastMessageChange = now;
    pushMessageStep++;
    if (pushMessageStep > 4) {
      stopPushSequence();
      return;
    }

    switch (pushMessageStep) {
      case 1:
        printWrapped("My Bagel Jewish");
        break;
      case 2:
        printWrapped("Christian Dior");
        break;
      case 3:
        printWrapped("Knicks in 4!");
        break;
      case 4:
        printWrapped("FUCK TRAE YOUNG");
        break;
    }
  }
}

void updateJoystickMode(char keyPressed) {
  static unsigned long lastLcdUpdate = 0;
  static int lastDisplayedValue = -1;

  if (keyPressed == 'B') {
    joystickMode = false;
    showJoystickExit();
    return;
  }

  if (millis() - lastLcdUpdate >= 50) {
    lastLcdUpdate = millis();

    int x = analogRead(A0);
    int y = analogRead(A1);
    int joyStickValue = sqrt((long)x * x + (long)y * y);

    if (lastDisplayedValue == -1 || abs(joyStickValue - lastDisplayedValue) >= 2) {
      lcd.setCursor(0, 0);
      lcd.print("Joy:            ");
      lcd.setCursor(5, 0);
      lcd.print(joyStickValue);
      lastDisplayedValue = joyStickValue;
    }
  }
}

void lightShow() {
  digitalWrite(REDLEDPIN, HIGH);
  delay(100);
  digitalWrite(REDLEDPIN, LOW);
  digitalWrite(GREENLEDPIN, HIGH);
  delay(100);
  digitalWrite(GREENLEDPIN, LOW);
}

void loop() {
  keypad.tick();

  char keyPressed = 0;
  while (keypad.available()) {
    keypadEvent e = keypad.read();
    if (e.bit.EVENT == KEY_JUST_PRESSED) {
      keyPressed = (char)e.bit.KEY;
      Serial.print("Key pressed: ");
      Serial.println(keyPressed);
    }
  }

  switchState = digitalRead(PUSHBUTTONPIN);

  if (isPushButtonJustPressed() && !pushSequenceActive) {
    startPushSequence();
  }

  updatePushSequence();
  updateRotaryEncoder();
  updateMelody();

  if (joystickMode) {
    updateJoystickMode(keyPressed);
    return;
  }

  if (keyPressed == 'A') {
    joystickMode = true;
    showJoystickEntry();
    return;
  }

  if (keyPressed == '*') {
    digitalWrite(REDLEDPIN, HIGH);
    delay(1000);
    digitalWrite(REDLEDPIN, LOW);
    return;
  }

  if (keyPressed == '#') {
    digitalWrite(GREENLEDPIN, HIGH);
    delay(1000);
    digitalWrite(GREENLEDPIN, LOW);
    return;
  }

  if (keyPressed) {
    printWrapped(String(keyPressed));
    Serial.println(keyPressed);
  }
}
