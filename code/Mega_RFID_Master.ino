/**
* Master/Slave Mega-RFID Puzzle - MASTER code
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
*
* A separate "Master" Arduino uses an I2C interface to regularly poll each of the 
* slave devices and retrieves the IDs detected by their sensors. It then compares
* that to the master solution to determine whether the puzzle has been solved.
*
* The master Arduino can address up to 112 I2C devices. If each slave has 3 RFID
* sensors, this would create a theoretical maximum of 336 objects that had to be
* placed in the correct order!
*/

// DEFINES
// Provides debugging information over serial connection if defined
#define DEBUG

// LIBRARIES
// Used for I2C communication between devices
#include <Wire.h>

// CONSTANTS
// The number of slave devices
const byte numSlaves = 3;
// The number of RFID readers each slave controls
const byte numReadersPerSlave = 2;
// The length in bytes of the unique RFID. For Mifare tags, this is 4-byte ID.
const byte numBytesInID = 4;
// Unique address of each slave
// The slave address is a 7-bit address, in the range 0 to 127 decimal (0x00 to 0x7F hex).
// However addresses 0-7 and 120-127 are reserved.
const int slaveIDs[numSlaves] = {8, 9, 10};
// The set of 4-byte NFC tag IDs required to solve the puzzle
const byte correctIDs[numSlaves*numReadersPerSlave][numBytesInID] = {
  {0x3a, 0xcb, 0xbd, 0x49},
  {0xca, 0x76, 0x75, 0x64},
  {0x3a, 0xcb, 0xbd, 0x49},
  {0xca, 0x76, 0x75, 0x64},
  {0x3a, 0xcb, 0xbd, 0x49},
  {0xca, 0x76, 0x75, 0x64},  
};
// This pin will be driven HIGH when the puzzle is solved
const byte lockPin = 2;

// GLOBALS
// Create an array to hold the current value of tags read by the sensors
byte currentIDs[numSlaves*numReadersPerSlave][numBytesInID];

/**
 * Initialisation
 */
void setup() {

  #ifdef DEBUG
  // Initialise serial communications channel with the PC
  Serial.begin(9600);
  Serial.println(F("Serial communication started"));
  #endif
  
  // Set the lock pin as output and secure the lock
  pinMode(lockPin, OUTPUT);
  digitalWrite(lockPin, LOW);

  // Initialise the I2C interface
  Wire.begin(); // no address needed for Master
}


/**
 *  Sends to serial output hex-formatted value of all IDs currently being read
 */
void printIDs() {

  // Loop over every reader of every slave
  for(int i=0; i<numSlaves*numReadersPerSlave; i++) {
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

  bool changedValue = false;

  // Request data from all slave devices
  for(int i=0; i<numSlaves; i++) {

    #ifdef DEBUG
    //  Serial.print(F("Requesting data..."));
    #endif

    // Request data from each slave as a stream of bytes for each ID of each reader
    // and populate the currentIDs array
    int j=0;    
    if(Wire.requestFrom(slaveIDs[i], numReadersPerSlave * numBytesInID)){
      while(Wire.available() > 0) {

        byte readValue = Wire.read();

        // Test whether the value has changed and update flag
        if(currentIDs[0][i*numReadersPerSlave*numBytesInID + j] != readValue) {
          changedValue = true;
        }

        // Update the stored value
        currentIDs[0][i*numReadersPerSlave*numBytesInID + j] = readValue;
        j++;
      }
    }
  }

  #ifdef DEBUG
    if(changedValue) { printIDs(); }
  #endif


  // Compare the byte array received to the correct byte array
  if(memcmp(currentIDs, correctIDs, numSlaves*numReadersPerSlave) == 0){
    // If matched, solve the puzzle
    onSolve();
  }

  // Introduce a slight delay before next polling the slave devices  
  delay(1000);
}

/**
 * Called when correct puzzle solution has been entered
 */
void onSolve(){
  #ifdef DEBUG
    // Print debugging message
    Serial.println(F("Puzzle Solved!"));
  #endif
  
  // Release the lock
  digitalWrite(lockPin, HIGH);

  // Loop forever
  while(true) {
    delay(1000);
  }
}
