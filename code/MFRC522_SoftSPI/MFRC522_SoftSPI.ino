// INCLUDES
#include "src/SoftMFRC522.h"

// CONSTANTS
/*
const byte csPin = 4;   //
const byte mosiPin = 23; // Arduino 11, ESP32 23
const byte misoPin = 25;  // Arduino 12, ESP32 19
const byte clkPin = 18; // Arduino 13, ESP32 18
const byte rstPin = -1;
*/
const byte numReaders = 4;

// GLOBALS
SoftMFRC522 rfid[] = {
  // Use -1 if no RESET pin
  SoftMFRC522(4, 23, 25, 18, -1),
  SoftMFRC522(16, 23, 26, 18, -1),
  SoftMFRC522(17, 23, 27, 18, -1),
  SoftMFRC522(5, 23, 14, 18, -1),
};

void setup() {
  Serial.begin(115200);
  while(!Serial);
  Serial.println(__FILE__ __DATE__);

  for(int i=0; i<numReaders; i++){
    // Set pin modes used for the software SPI interface
    rfid[i].begin();

    // Sets the reader settings
    rfid[i].init();
    delay(100);

    // Dump some debug information to the serial monitor
    Serial.print(F("Reader #"));
    Serial.print(i);
    Serial.print(F(". Antenna strength: "));
    byte s = rfid[i].readFrom(RFCfgReg) & (0x07<<4);
    Serial.print(s);
    byte v = rfid[i].readFrom(VersionReg);
	  Serial.print(F(". Firmware Version: 0x"));
	  Serial.print(v, HEX);
    // Lookup which version
    switch(v) {
      case 0x88: Serial.println(F(" = (clone)"));  break;
      case 0x90: Serial.println(F(" = v0.0"));     break;
      case 0x91: Serial.println(F(" = v1.0"));     break;
      case 0x92: Serial.println(F(" = v2.0"));     break;
      case 0x12: Serial.println(F(" = counterfeit chip"));     break;
      default:   Serial.println(F(" = (unknown)"));
    }
  }
}

void loop() {
  for(int i=0; i<numReaders; i++){
    uint8_t str[MAX_LEN];
    // Read all cards in the vicinity
    uint8_t status = rfid[i].request(PICC_REQALL, str);
    if (status == MI_OK) {
      // Prevent conflict
      status = rfid[i].anticoll(str);
      // If successful
      if (status == MI_OK) {
        Serial.print(i);
        Serial.print(":");
        rfid[i].showCardID(str);//show the card ID
        Serial.println();
      }
    // Put the card into sleep mode
    // rfid.halt(); 
    }
  }
}