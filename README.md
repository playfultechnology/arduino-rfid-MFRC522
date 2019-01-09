# arduino-rfid-MFRC522
Interfacing Arduino with 13.56MHz ISO14443 RFID tags using NXP MFRC522 modules
![MFRC522 module](https://raw.githubusercontent.com/playfultechnology/arduino-rfid-MFRC522/master/documentation/MFRC522.jpg)

By default, these readers use the SPI interface, although the chip also supports UART and I2C. Datasheet can be found <a href="https://www.nxp.com/docs/en/data-sheet/MFRC522.pdf
">here</a>. For a comparison of alternative readers, see https://www.patreon.com/posts/rfid-roundup-23115452


# Components
- [PN5180 Module](https://www.aliexpress.com/item/PN5180-NFC-IC-ISO15693-RFID-SLIX-ISO-IEC-18092-14443-A-B-Read-Write-module/32840851498.html) ~£8
- [Arduino](https://www.banggood.com/ATmega328P-Nano-V3-Controller-Board-Compatible-Arduino-p-940937.html) ~£3

# Wiring
There are 8 pins on the MFRC522 board, as follows:
- SDA : Serial Data (I2C mode) / (Not) Slave Select (SPI mode) is pulled LOW when communicating with Arduino *  
- SCK : System Clock signal sent from Arduino *
- MOSI : Master-Out, Slave-In used for transmitting data from Arduino to the reader *
- MISO : Master-In, Slave-Out used for transmitting data from the reader to the Arduino
- IRQ : Not required
- GND : Ground
- RST : Reset line from Arduino *
- +3.3V : +3.3V DC supply 

Arduino UNO/Nano/Mega operates at 5V logic. However, the MFRC522 works at 3.3V logic level, and is not 5V tolerant. If using one of these microprocessors:
- You **must** use a 5V <-> 3.3V level shifter on any lines that send HIGH output *from* the Arduino *to* the MFRC522. These are RST, NSS, MOSI, and SCK marked with an asterisk in the list above.
- You **may** use a 3.3V <-> 5V level shifter on those lines that send HIGH output *from* the MFRC522 *to* the Arduino (MISO) though this is not generally necessary - a HIGH 3.3V signal will normally be recognised as a HIGH signal on a 5V system too.

- MOSI, MISO, and SCK lines are shared between all readers, and use Arduino SPI pins 11(MOSI), 12(MISO), and 13(SCK). 
- NSS lines must be assigned to unique GPIO pins for each reader.

![MFRC522 to Arduino using SPI](https://raw.githubusercontent.com/playfultechnology/arduino-rfid-MFRC522/master/documentation/MFRC522_bb.jpg)
