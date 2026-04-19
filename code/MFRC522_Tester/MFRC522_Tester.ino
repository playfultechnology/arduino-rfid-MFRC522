

// DEFINES
#define MFRC522_SPICLOCK (400000u)

#include <EEPROM.h>
#define EEPROM_SIZE 64

// INCLUDES
#include <SPI.h>
#include <MFRC522.h>
#include <U8g2lib.h>

// CONSTANTS
constexpr uint8_t ssPin = D8;
constexpr uint8_t clkPin = D1, dataPin = D2;
constexpr uint8_t beepPin = D0;
constexpr uint8_t buttonPin = A0;
constexpr int buttonThreshold = 600;


// GLOBALS
MFRC522 mfrc522(ssPin);
U8G2_SSD1306_128X32_UNIVISION_F_HW_I2C u8g2(
  U8G2_R0, U8X8_PIN_NONE, clkPin, dataPin
);

// Last UID seen by reader
byte currentUID[10] = {0};
byte currentUIDSize = 0;
bool tagPresent = false;
bool beepActive = false;
unsigned long beepEnd = 0;


// ---------- HELPERS ----------
void beep(uint16_t durationMs = 10) {
  digitalWrite(beepPin, HIGH);
  beepEnd = millis() + durationMs;
  beepActive = true;
}

void uidToString(const byte *uid, byte size, char *out) {
  for (byte i = 0; i < size; i++) {
    sprintf(&out[i * 2], "%02X", uid[i]);
  }
  out[size * 2] = '\0';
}

void printUID(const byte *uid, byte size) {
  for (byte i = 0; i < size; i++) {
    if (uid[i] < 0x10) Serial.print("0");
    Serial.print(uid[i], HEX);
  }
  Serial.println();
}

bool uidEquals(const byte *a, byte aSize, const byte *b, byte bSize) {
  if (aSize != bSize) return false;
  return memcmp(a, b, aSize) == 0;
}

void showUIDOnOLED(const byte *uid, byte size) {
  char tagStr[21];  // max 10-byte UID => 20 hex chars + NUL
  uidToString(uid, size, tagStr);

  //u8g2.clearBuffer();
  u8g2.setDrawColor(0); 
  u8g2.drawBox(0, 10, 128, 22);
  u8g2.setDrawColor(1);              // restore normal drawing

  if (size < 5) {
    u8g2.setFont(u8g2_font_inb16_mr);
  } else {
    u8g2.setFont(u8g2_font_t0_15b_mr);
  }

  u8g2.drawStr(8, 32, tagStr);
  u8g2.sendBuffer();
}

void clearOLEDTagArea() {
  // u8g2.clearBuffer();
  u8g2.setDrawColor(0); 
  u8g2.drawBox(0, 10, 128, 22);
  u8g2.setDrawColor(1);              // restore normal drawing
  u8g2.sendBuffer();
}

// Presence check for any PICC, including a card that is already being held there
bool PICC_IsAnyCardPresent() {
  byte atqa[2];
  byte atqaSize = sizeof(atqa);
  // Reset some reader settings expected by REQA/WUPA
  mfrc522.PCD_WriteRegister(MFRC522::TxModeReg, 0x00);
  mfrc522.PCD_WriteRegister(MFRC522::RxModeReg, 0x00);
  mfrc522.PCD_WriteRegister(MFRC522::ModWidthReg, 0x26);
  MFRC522::StatusCode result = mfrc522.PICC_WakeupA(atqa, &atqaSize);
  return (result == MFRC522::STATUS_OK || result == MFRC522::STATUS_COLLISION);
}

// Read current UID if possible
bool readCurrentUID(byte *uidOut, byte &sizeOut) {
  if (!mfrc522.PICC_ReadCardSerial()) {
    return false;
  }
  sizeOut = mfrc522.uid.size;
  memcpy(uidOut, mfrc522.uid.uidByte, sizeOut);
  mfrc522.PICC_HaltA();
  mfrc522.PCD_StopCrypto1();
  return true;
}

bool isSameAsStored() {
  byte storedSize = EEPROM.read(0);
  if (storedSize != currentUIDSize) return false;
  for (byte i = 0; i < storedSize; i++) {
    if (EEPROM.read(1 + i) != currentUID[i]) return false;
  }
  return true;
}

void saveUIDToEEPROM(const byte *uid, byte size) {
  EEPROM.write(0, size);
  for (byte i = 0; i < size; i++) {
    EEPROM.write(1 + i, uid[i]);
  }
  EEPROM.commit();
}

void printStoredUIDFromEEPROM() {
  byte storedSize = EEPROM.read(0);
  if (storedSize == 0 || storedSize > 10) {
    Serial.println("No UID stored");
    return;
  }
  byte storedUID[10];
  for (byte i = 0; i < storedSize; i++) {
    storedUID[i] = EEPROM.read(1 + i);
  }
  Serial.print("Stored UID: ");
  printUID(storedUID, storedSize);
}

// ---------- SETUP ----------
void setup() {
  Serial.begin(115200);
  delay(200);
  Serial.println();
  Serial.println(__FILE__ " " __DATE__);

  SPI.begin();
  mfrc522.PCD_Init();
  delay(50);
  mfrc522.PCD_DumpVersionToSerial();

  u8g2.setI2CAddress(0x3C << 1);
  u8g2.begin();
  u8g2.clearBuffer();
  u8g2.setFont(u8g2_font_5x7_mr);
  u8g2.drawStr(0, 8, "Playful Technology");
  u8g2.sendBuffer();

  pinMode(beepPin, OUTPUT);
  digitalWrite(beepPin, LOW);

  pinMode(buttonPin, INPUT);

  EEPROM.begin(EEPROM_SIZE);
  printStoredUIDFromEEPROM();
}

// ---------- LOOP ----------
void loop() {
  // 1) Check whether any card is physically present right now
  bool anyCardPresent = PICC_IsAnyCardPresent();

  if (anyCardPresent) {
    byte newUID[10];
    byte newUIDSize = 0;

    // 2) Try to read UID of the present card
    if (readCurrentUID(newUID, newUIDSize)) {
      bool changed = !tagPresent || !uidEquals(newUID, newUIDSize, currentUID, currentUIDSize);

      if (changed) {
        memcpy(currentUID, newUID, newUIDSize);
        currentUIDSize = newUIDSize;
        tagPresent = true;

        Serial.print("UID: ");
        printUID(currentUID, currentUIDSize);

        showUIDOnOLED(currentUID, currentUIDSize);
        beep();
      } else {
        // Same card still present
        tagPresent = true;
      }
    } else {
      // Card is still present but UID wasn't read this pass.
      // Do not mark it removed.
      tagPresent = true;
    }
  } else {
    // 3) Only treat as removed when no PICC responds at all
    if (tagPresent) {
      Serial.println("Tag removed");
      clearOLEDTagArea();

      tagPresent = false;
      currentUIDSize = 0;
    }
  }

  // 4) Button on A0: save currently detected UID
  static bool buttonHandled = false;
  int buttonValue = analogRead(buttonPin);
  if (buttonValue > buttonThreshold && !buttonHandled) {
    buttonHandled = true;
    Serial.print("Button pressed: ");
    Serial.println(buttonValue);
    if (tagPresent && currentUIDSize > 0) {
      if (!isSameAsStored()) {
        saveUIDToEEPROM(currentUID, currentUIDSize);
        Serial.println("UID saved to EEPROM");
      } else {
        Serial.println("UID already stored");
      }
      beep(50);
    } else {
      Serial.println("No UID to save");
    }
  }
  if (buttonValue < buttonThreshold) {
    buttonHandled = false;
  }
  // 5) Stop a beep if it's active 
  if (beepActive && millis() > beepEnd) {
    digitalWrite(beepPin, LOW);
    beepActive = false;
  }
  // 6) OPTIONAL After repeated PICC_WakeupA() calls, the reader can occasionally drift.
  // Add a soft reset occasionally: 
  static unsigned long lastReset = 0;
  if (millis() - lastReset > 10000) {
    mfrc522.PCD_Init();
    lastReset = millis();
  }

  //delay(5);
}