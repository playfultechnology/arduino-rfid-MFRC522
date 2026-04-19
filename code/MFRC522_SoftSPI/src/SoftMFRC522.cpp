#include <Arduino.h>  
#include "SoftMFRC522.h"

SoftMFRC522::SoftMFRC522(const uint8_t csPin, const int8_t mosiPin, const int8_t misoPin, const uint8_t sckPin, const int8_t rstPin, const unsigned long delayUs){
  _csPin = csPin;
  _mosiPin = mosiPin;
  _misoPin = misoPin;
  _sckPin = sckPin;
  _rstPin = rstPin;
  _delayUs = delayUs;
}

// Bit Banged SPI transfer
byte SoftMFRC522::transfer (byte c) {       
  // loop for each bit  
  for (byte bit = 0; bit < 8; bit++) {
    // set up MOSI on falling edge of previous SCK (sampled on rising edge)
    if (_mosiPin != -1) {
      if (c & 0x80)
          digitalWrite (_mosiPin, HIGH);
      else
          digitalWrite (_mosiPin, LOW);
      }
    // finished with MS bit, get read to receive next bit      
    c <<= 1;
 
    // read MISO
    if (_misoPin != -1)
      c |= digitalRead (_misoPin) != 0;
 
    // clock high
    digitalWrite (_sckPin, HIGH);
 
    // delay between rise and fall of clock
    delayMicroseconds (_delayUs);
 
    // clock low
    digitalWrite (_sckPin, LOW);

    // delay between rise and fall of clock
    delayMicroseconds (_delayUs);
    }  // end of for loop, for each bit
   
  return c;
  }  // end of SPI transfer  


void SoftMFRC522::begin() {
  pinMode(_csPin, OUTPUT);
  digitalWrite(_csPin, LOW);
  pinMode (_sckPin, OUTPUT);
  if(_mosiPin != -1) { pinMode (_mosiPin, OUTPUT); }
  if(_misoPin != -1) { pinMode (_misoPin, INPUT); }
  if(_rstPin != -1){ pinMode(_rstPin, OUTPUT); }
}
/**********************************************************
 * Function：ShowCardID
 * Description：Show Card ID
 * Input parameter：ID string
 * Return：Null
 **********************************************************/
void SoftMFRC522::showCardID(uint8_t *id) {
	int IDlen=4;
    for(int i=0; i<IDlen; i++){
        Serial.print(0x0F & (id[i]>>4), HEX);
        Serial.print(0x0F & id[i],HEX);
    }
}
/**********************************************************
 * Function：ShowCardType
 * Description：Show Card type
 * Input parameter：Type string
 * Return：Null
 **********************************************************/
void SoftMFRC522::showCardType(uint8_t* type) {
    Serial.print("Card type: ");
    if(type[0]==0x04&&type[1]==0x00) 
        Serial.println("MFOne-S50");
    else if(type[0]==0x02&&type[1]==0x00)
        Serial.println("MFOne-S70");
    else if(type[0]==0x44&&type[1]==0x00)
        Serial.println("MF-UltraLight");
    else if(type[0]==0x08&&type[1]==0x00)
        Serial.println("MF-Pro");
    else if(type[0]==0x44&&type[1]==0x03)
        Serial.println("MF Desire");
    else
        Serial.println("Unknown");
}
/**********************************************************
 * Function：Write_MFRC522
 * Description：write a single byte value into one register of MFRC522
 * Input parameter：addr--register address；val--the value that need to write in
 * Return：Null
 **********************************************************/
void SoftMFRC522::writeTo(uint8_t addr, uint8_t val) {
    digitalWrite(_csPin, LOW);

    //address format：0XXXXXX0
    transfer((addr<<1)&0x7E); 
    transfer(val);
    
    digitalWrite(_csPin, HIGH);
}
/**********************************************************
 * Function：Read_MFRC522
 * Description：read one byte data of from one register of MFRC522
 * Input parameter：addr--register address
 * Return：return the read value
 **********************************************************/
uint8_t SoftMFRC522::readFrom(uint8_t addr) {
    uint8_t val;

    digitalWrite(_csPin, LOW);

    //address format：1XXXXXX0
    transfer(((addr<<1)&0x7E) | 0x80); 
	
	// Should this not just be SPI.transfer(0x80 | reg<<1); ?
    val = transfer(0);
    
    digitalWrite(_csPin, HIGH);
    
    return val; 
}
/**********************************************************
 * Function：SetBitMask
 * Description：set RC522 register bit
 * Input parameter：reg--register address;mask--value
 * Return：null
 **********************************************************/
void SoftMFRC522::setBitMask(uint8_t reg, uint8_t mask) {
    uint8_t tmp;
    tmp = readFrom(reg);
    writeTo(reg, tmp | mask); // set bit mask
}
/**********************************************************
 * Function：ClearBitMask
 * Description：clear RC522 register bit
 * Input parameter：reg--register address;mask--value
 * Return：null
 **********************************************************/
void SoftMFRC522::clearBitMask(uint8_t reg, uint8_t mask) {
    uint8_t tmp;
    tmp = readFrom(reg);
    writeTo(reg, tmp & (~mask)); // clear bit mask
}
/**********************************************************
 * Function：AntennaOn
 * Description：Turn on antenna, every time turn on or shut down antenna need at least 1ms delay
 * Input parameter：null
 * Return：null
 * Return：null
 **********************************************************/
void SoftMFRC522::antennaOn(void) {
    uint8_t temp;

    temp = readFrom(TxControlReg);
    if (!(temp & 0x03)) {
        setBitMask(TxControlReg, 0x03);
    }
}
/**********************************************************
 * Function：AntennaOff
 * Description：Turn off antenna, every time turn on or shut down antenna need at least 1ms delay
 * Input parameter：null
 * Return：null
 **********************************************************/
void SoftMFRC522::antennaOff(void) {
    clearBitMask(TxControlReg, 0x03);
}
/*
 * Function：ResetMFRC522
 * Description： reset RC522
 * Input parameter：null
 * Return：null
 */
void SoftMFRC522::reset(void) {
    writeTo(CommandReg, PCD_RESETPHASE);
}
/*
 * Function：InitMFRC522
 * Description：initilize RC522
 * Input parameter：null
 * Return：null
 */
void SoftMFRC522::init(void) {
	// If we've defined a reset pin
	if(_rstPin != -1){
		// Bring the reset pin HIGH to ensure the reader is active
		digitalWrite(_rstPin,HIGH);
	}
    reset();
         
    //Timer: TPrescaler*TreloadVal/6.78MHz = 24ms
    writeTo(TModeReg, 0x8D); //Tauto=1; f(Timer) = 6.78MHz/TPreScaler
    writeTo(TPrescalerReg, 0x3E); //TModeReg[3..0] + TPrescalerReg
    writeTo(TReloadRegL, 30); 
    writeTo(TReloadRegH, 0);
    
    writeTo(TxAutoReg, 0x40); //100%ASK
    writeTo(ModeReg, 0x3D); //CRC initilizate value 0x6363 ???

    //ClearBitMask(Status2Reg, 0x08); //MFCrypto1On=0
    //Write_MFRC522(RxSelReg, 0x86); //RxWait = RxSelReg[5..0]
    //Write_MFRC522(RFCfgReg, 0x7F); //RxGain = 48dB

    antennaOn(); //turn on antenna
}
/*
 * Function：MFRC522_Request
 * Description：Searching card, read card type
 * Input parameter：reqMode--search methods，
 * TagType--return card types
 * 0x4400 = Mifare_UltraLight
 * 0x0400 = Mifare_One(S50)
 * 0x0200 = Mifare_One(S70)
 * 0x0800 = Mifare_Pro(X)
 * 0x4403 = Mifare_DESFire
 * return：return MI_OK if successed
 */
uint8_t SoftMFRC522::request(uint8_t reqMode, uint8_t *TagType) {
	
    uint8_t status; 
    uint8_t backBits; //the data bits that received

    writeTo(BitFramingReg, 0x07); //TxLastBists = BitFramingReg[2..0] ???
    
    TagType[0] = reqMode;
    status = toCard(PCD_TRANSCEIVE, TagType, 1, TagType, &backBits);

    if ((status != MI_OK) || (backBits != 0x10)) { 
        status = MI_ERR;
    }
   
    return status;
}
/*
 * Function：MFRC522_ToCard
 * Description：communicate between RC522 and ISO14443
 * Input parameter：command--MF522 command bits
 * sendData--send data to card via rc522
 * sendLen--send data length 
 * backData--the return data from card
 * backLen--the length of return data
 * return：return MI_OK if successed
 */
uint8_t SoftMFRC522::toCard(uint8_t command, uint8_t *sendData, uint8_t sendLen, uint8_t *backData, uint8_t *backLen) {
    uint8_t status = MI_ERR;
    uint8_t irqEn = 0x00;
    uint8_t waitIRq = 0x00;
    uint8_t lastBits;
    uint8_t n;
    uint8_t i;

    switch (command) {
        case PCD_AUTHENT: //verify card password
        {
            irqEn = 0x12;
            waitIRq = 0x10;
            break;
        }
        case PCD_TRANSCEIVE: //send data in the FIFO
        {
            irqEn = 0x77;
            waitIRq = 0x30;
            break;
        }
        default:
            break;
    }
   
    writeTo(CommIEnReg, irqEn|0x80); //Allow interruption
    clearBitMask(CommIrqReg, 0x80); //Clear all the interrupt bits
    setBitMask(FIFOLevelReg, 0x80); //FlushBuffer=1, FIFO initilizate
    
    writeTo(CommandReg, PCD_IDLE); //NO action;cancel current command ???

    // Write data into FIFO
    for (i=0; i<sendLen; i++) { 
        writeTo(FIFODataReg, sendData[i]); 
    }

    // Process it
    writeTo(CommandReg, command);
    if (command == PCD_TRANSCEIVE) { 
        setBitMask(BitFramingReg, 0x80); //StartSend=1,transmission of data starts 
    } 
    
    // Wait while data is received
    i = 2000; //i should adjust according the clock, the maximum waiting time should be 25 ms???
    do {
        //CommIrqReg[7..0]
        //Set1 TxIRq RxIRq IdleIRq HiAlerIRq LoAlertIRq ErrIRq TimerIRq
        n = readFrom(CommIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x01) && !(n&waitIRq));

    clearBitMask(BitFramingReg, 0x80); //StartSend=0
    
    if (i != 0) { 
		//BufferOvfl Collerr CRCErr ProtecolErr
        if(!(readFrom(ErrorReg) & 0x1B)) {
            status = MI_OK;
            if (n & irqEn & 0x01) { 
                status = MI_NOTAGERR; //?? 
            }
            if (command == PCD_TRANSCEIVE) {
                n = readFrom(FIFOLevelReg);
                lastBits = readFrom(ControlReg) & 0x07;
                if (lastBits){ 
                    *backLen = (n-1)*8 + lastBits; 
                }
                else { 
                    *backLen = n*8; 
                }
                
                if (n == 0) { 
                    n = 1; 
                }
                if (n > MAX_LEN){ 
                    n = MAX_LEN; 
                }
                
                // Read the data from FIFO
                for (i=0; i<n; i++) { 
                    backData[i] = readFrom(FIFODataReg); 
                }
            }
        }
        else { 
            status = MI_ERR; 
        }
    }
    
    //SetBitMask(ControlReg,0x80); //timer stops
    //Write_MFRC522(CommandReg, PCD_IDLE); 

    return status;
}
/*
 * Function：MFRC522_Anticoll
 * Description：Prevent conflict, read the card serial number 
 * Input parameter：serNum--return the 4 bytes card serial number, the 5th byte is recheck byte
 * return：return MI_OK if successed
 */
uint8_t SoftMFRC522::anticoll(uint8_t *serNum) {
    uint8_t status;
    uint8_t i;
    uint8_t serNumCheck=0;
    uint8_t unLen;
    
    //ClearBitMask(Status2Reg, 0x08); //strSensclear
    //ClearBitMask(CollReg,0x80); //ValuesAfterColl
    writeTo(BitFramingReg, 0x00); //TxLastBists = BitFramingReg[2..0]
 
    serNum[0] = PICC_ANTICOLL;
    serNum[1] = 0x20;
    status = toCard(PCD_TRANSCEIVE, serNum, 2, serNum, &unLen);

    if (status == MI_OK) {
        // Verify card serial number
        for (i=0; i<4; i++) { 
            serNumCheck ^= serNum[i];
        }
        if (serNumCheck != serNum[i]) { 
            status = MI_ERR; 
        }
    }

    //SetBitMask(CollReg, 0x80); //ValuesAfterColl=1

    return status;
}
/*
 * Function：CalculateCRC
 * Description：Use MF522 to caculate CRC
 * Input parameter：pIndata--the CRC data need to be read，len--data length，pOutData-- the caculated result of CRC
 * return：Null
 */
void SoftMFRC522::calculateCRC(uint8_t *pIndata, uint8_t len, uint8_t *pOutData) {
    uint8_t i, n;

    clearBitMask(DivIrqReg, 0x04); //CRCIrq = 0
    setBitMask(FIFOLevelReg, 0x80); //Clear FIFO pointer
    //Write_MFRC522(CommandReg, PCD_IDLE);

    // Write data into FIFO 
    for (i=0; i<len; i++) { 
        writeTo(FIFODataReg, *(pIndata+i)); 
    }
    writeTo(CommandReg, PCD_CALCCRC);

    // Wait for CRC calculation to finish
    i = 0xFF;
    do {
        n = readFrom(DivIrqReg);
        i--;
    }
    while ((i!=0) && !(n&0x04)); //CRCIrq = 1

    //read CRC caculation result
    pOutData[0] = readFrom(CRCResultRegL);
    pOutData[1] = readFrom(CRCResultRegM);
}
/*
 * Function：MFRC522_Write
 * Description：write block data
 * Input parameters：blockAddr--block address;writeData--Write 16 bytes data into block
 * return：return MI_OK if successed
 */
uint8_t SoftMFRC522::write(uint8_t blockAddr, uint8_t *writeData) {
    uint8_t status;
    uint8_t recvBits;
    uint8_t i;
    uint8_t buff[18]; 
    
    buff[0] = PICC_WRITE;
    buff[1] = blockAddr;
    calculateCRC(buff, 2, &buff[2]);
    status = toCard(PCD_TRANSCEIVE, buff, 4, buff, &recvBits);

    if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A)) { 
        status = MI_ERR; 
    }
        
    if (status == MI_OK){
		// Write 16 bytes data into FIFO
        for (i=0; i<16; i++) { 
            buff[i] = *(writeData+i); 
        }
        calculateCRC(buff, 16, &buff[16]);
        status = toCard(PCD_TRANSCEIVE, buff, 18, buff, &recvBits);
        
        if ((status != MI_OK) || (recvBits != 4) || ((buff[0] & 0x0F) != 0x0A)) { 
            status = MI_ERR; 
        }
    }
    
    return status;
}
/*
 * Function：MFRC522_Halt
 * Description：Command the cards into sleep mode
 * Input parameters：null
 * return：null
 */
void SoftMFRC522::halt(void) {
    uint8_t status;
    uint8_t unLen;
    uint8_t buff[4]; 

    buff[0] = PICC_HALT;
    buff[1] = 0;
    calculateCRC(buff, 2, &buff[2]);
 
    status = toCard(PCD_TRANSCEIVE, buff, 4, buff,&unLen);
}