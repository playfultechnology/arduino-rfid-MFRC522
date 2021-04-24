/**
* Master/Slave Mega-RFID Puzzle - SLAVE A code
*
* This is an extensible version of the object-placement puzzle, which
* requires players to place RFID-tagged items in front of the correct 
* sensor.
* When all sensors read the tag of the correct object, the puzzle is solved.
*
* In this version of the puzzle, the RFID sensors are placed into groups, and
* each group is read by a separate "slave" Arduino. These slave Arduinos do not
* control any of the game logic - they are simply responsible for detecting and
* storing the IDs of any tags placed on their RFID sensors.
* Then, when requested (over I2C), it sends the values to a master device.
*/

// DEFINES
// Provides debugging information over serial connection if defined
#define DEBUG

// LIBRARIES
// SPI library is used to interface with RFID sensors
#include <SPI.h>
// RFID library can be downloaded and installed from https://github.com/miguelbalboa/rfid
#include <MFRC522.h>
// I2C library used to communicate with Master Arduino
#include <Wire.h>
// Unique device ID used to identify this slave
#define deviceID 9

// CONSTANTS
// The number of RFID readers this slave will read
const byte numReaders = 2;
// Each reader has a unique Slave Select pin
const byte ssPins[] = {2, 4};
// They'll share the same reset pin
const byte resetPin = 8;
// The length in bytes of the unique RFID. For Mifare tags, this is 4-byte ID.
const byte numBytesInID = 4;

// GLOBALS
// Initialise an array of MFRC522 instances representing each reader
MFRC522 mfrc522[numReaders];
// The tag IDs currently detected by each reader
byte currentIDs[numReaders][numBytesInID];

/** 
 *  This callback will be fired when a request is made by the Master Arduino
 *  It simply dumps the contents of the current ID array over the I2C interface
 */
void requestCallback(){
  
  // FOR TESTING ONLY
  #ifdef TEST
  for(int i=0; i< numReaders * numBytesInID; i++){
    currentIDs[0][i] = (byte)random(255);
  }
  #endif
  
  // Write all the IDs over the I2C connection
  for(int i=0; i<numReaders; i++) {
    Wire.write(currentIDs[i], 4);
  }
}

/**
 * Initialisation
 */
void setup() {

  #ifdef DEBUG
  // Initialise serial communications channel with the PC
  Serial.begin(9600);
  Serial.println(F("Serial communication started"));
  #endif


  // FOR TESTING ONLY
  #ifdef TEST
  randomSeed(analogRead(0));
  #endif
  
  // Initialise the SPI bus
  SPI.begin();

  // Initialise the I2C interface
  Wire.begin(deviceID);
  Wire.onRequest(requestCallback);

  // Initialise the readers
  for (uint8_t i=0; i<numReaders; i++) {

    // Note that SPI pins on the reader must always be connected to certain
    // Arduino pins (on an Uno, MOSI=> pin11, MISO=> pin12, SCK=>pin13)
    // The Slave Select (SS) pin and reset pin can be assigned to any pin
    mfrc522[i].PCD_Init(ssPins[i], resetPin);
    
    // Set the gain to max - not sure this makes any difference...
    // mfrc522[i].PCD_SetAntennaGain(MFRC522::PCD_RxGain::RxGain_max);
    
    // Slight delay before activating next reader
    delay(100);
  }
}


/**
 *  Sends to serial output hex-formatted value of all IDs currently being read
 */
void printIDs() {

  // Loop over every reader of every slave
  for(int i=0; i<numReaders; i++) {
    // Character buffer needs to be twice as long as the ID byte array 
    // (since the hex representation of byte 0x5A needs two characters - '5' and 'A'
    // +1 more character for the trailing '\0' that indicates the end of a string
    char s[(numBytesInID*2) + 1];
      
    // Use sprintf to create a formatted string from the individual byte array
    // No need to manually add the trailing '\0' because sprintf does this automatically
    sprintf(s, "%02X%02X%02X%02X", currentIDs[i][0], currentIDs[i][1], currentIDs[i][2], currentIDs[i][3]);
    
    // Print the output to the serial connection
    Serial.println(s); 
  }
  Serial.println(F("---"));
}


/**
 * Main loop
 */
void loop() {

  // Clear the array
  memset(currentIDs, 0, sizeof(currentIDs));

  // Loop through each reader
  for (uint8_t i=0; i<numReaders; i++) {

    // Initialise the sensor
    mfrc522[i].PCD_Init();

    // If the sensor detects a tag and is able to read it
    if(mfrc522[i].PICC_IsNewCardPresent()) {
      if(mfrc522[i].PICC_ReadCardSerial()) {
        // Copy the 4-byte ID from the tag
        memcpy(&currentIDs[i][0], &mfrc522[i].uid.uidByte, numBytesInID);
      }
    }

    // Halt PICC
    mfrc522[i].PICC_HaltA();
    // Stop encryption on PCD
    mfrc522[i].PCD_StopCrypto1(); 
  }

  // Add a short delay before next polling sensors
  delay(200);
  
  printIDs();
  
  
}
