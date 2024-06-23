#include "I2CHex.h"
#include "stdint.h"
#include "Arduino.h"
//volatile uint8_t wif = TWI0.MSTATUS & (1<<6) ;
//uint8_t rif = TWI0.MSTATUS & 0x80;
//uint8_t rxack = TWI0.MSTATUS & 0x10;
//uint8_t clkhold = TWI0.MSTATUS & 0x20;
//uint8_t arblost = TWI0.MSTATUS & 0x08;
//uint8_t buserr = (TWI0.MSTATUS & 0x04);

I2CDevice I2CDev;

//res: 1 -> done reading register bytes. 2-> recieved NACK from client after register byte sent. 3 -> missing ack when sending the address byte. 5-> lost arbitration, 6-> connection timeout
ISR(TWI0_TWIM_vect){
  	
	if ((TWI0.MSTATUS & (1<<6)) && I2CDev.stepz == 0){
		//if WIF set, and done transmitting the slave addr (address packets)
		TWI0.MSTATUS |= (1<<6); //clears flag, is this needed ?? Isnt it being cleared by the readReg func
		if ((TWI0.MSTATUS & 0x20) && !(TWI0.MSTATUS & 0x10)){
			//M1
      TWI0.MDATA = I2CDev.regAddr; //send reg address      
      I2CDev.stepz = 1; //signifies new int thread for readRegister / writeRegister.
      return;
		}
		else if ((TWI0.MSTATUS & 0x20) && (TWI0.MSTATUS & 0x10)){
			//M3
			I2CDev.res = 3;
			TWI0.MCTRLB |= 0x3; //STOP cmd
		}
		else {
			I2CDev.res = 5;
      return;
		}
	}
  else if ((TWI0.MSTATUS & (1<<6)) && I2CDev.stepz == 1){
    if ((TWI0.MSTATUS & 0x10) == 0){
      //recieved ACK from slave after register address write.
      if (!I2CDev.writing){
        uint8_t addrR = I2CDev.slaveAddr << 1 | 1;
        TWI0.MADDR = addrR; //need to add interrupt for wif on stepz == 2 for lost arbitration etc (not M2); The only interrupt code after this for stepz == 2 is for when a byte has been sent, which is assuming transmission all good.
        I2CDev.stepz = 2;
        return;
      }
      else {
        
        //writeReg pathway after sending register addr
        //send data over
        I2CDev.stepz = 1; //still sits on same WIF interrupt for writing data bytes
        if (I2CDev.byteWriteCount == I2CDev.numberBytes){
          TWI0.MCTRLB |= 0x3; //STOP cmd
          I2CDev.res = 1;
        }
        TWI0.MDATA = I2CDev.bufferData[I2CDev.byteWriteCount];
        //TWI0.MCTRLB |= 0x3; //STOP cmd
        I2CDev.byteWriteCount++;
        
//        if (I2CDev.byteWriteCount == I2CDev.numberBytes){
//          //end of the road, all done
//          TWI0.MCTRLB |= 0x3; //STOP cmd
//          I2CDev.res = 1;
//          PORTA.OUTCLR = (1<<4);
//          return;
//        }
//        return;
//        
      }
    }
    else if ((TWI0.MSTATUS & 0x10) == 1){
      //recieved nack from client after sending register address, something went wrong, stop condition.
      TWI0.MCTRLB |= 0x3; //STOP cmd
      I2CDev.res = 2;
      return;
    }
  }

  else if ((TWI0.MSTATUS & 0x80) && I2CDev.stepz == 2){
    if (TWI0.MSTATUS & 0x20){
      
      //clk hold set, recieved byte of data from slave
      I2CDev.bufferData[I2CDev.byteReadCount] = TWI0.MDATA; //auto clears RIF flag
      I2CDev.byteReadCount++;
      if (I2CDev.byteReadCount == I2CDev.numberBytes){
        //send NACK
        TWI0.MCTRLB = 0x07;
        I2CDev.res = 1; //exits out
      }
      else{
        //send ACK for next byte
        TWI0.MCTRLB = 0x02;
      }
      return;
    }
  }
  return;
	
}

void I2CDevice::begin(){
	TWI0.MBAUD = 20; //sets baud
	TWI0.MCTRLA |= 1; //enables TWI Host
  TWI0.MSTATUS = 1;
	TWI0.MCTRLA |= (1<<6) | (1<<7); //enabling WIF interrupt and RIF
	PORTA.DIRSET = (1<<4);
  PORTA.OUTCLR = (1<<4);
  I2CDev.stepz = 0;
  I2CDev.byteReadCount = 0;
	sei();
}

void I2CDevice::writeReg(uint8_t saddr, uint8_t regaddr, uint8_t* val, uint8_t sizeData){
  I2CDev.stepz = 0;
  I2CDev.byteWriteCount = 0;
  I2CDev.res = 0;
  I2CDev.writing = 1;
  I2CDev.regAddr = regaddr;
  I2CDev.numberBytes = sizeData;
  if (sizeData > 4){
    return;
  }
  else{
    for (uint8_t i = 0; i < sizeData; i++){
      I2CDev.bufferData[i] = val[i];
    }
  }
  TWI0.MADDR = (saddr << 1);

  while (!I2CDev.res){
    delay(500);
    I2CDev.res = 6;
  }

  
  
  
}


void I2CDevice::readRegister(uint8_t saddr, uint8_t regaddr, uint8_t numBytes, uint8_t* buff){
  I2CDev.stepz = 0;
  I2CDev.byteReadCount = 0;
  I2CDev.res = 0;
  I2CDev.writing = 0;
	uint8_t maddr = saddr << 1; //R/!W set to zero, host writing
  I2CDev.slaveAddr = saddr;
  I2CDev.regAddr = regaddr;
  I2CDev.numberBytes = numBytes;
	TWI0.MADDR = maddr;
	
	while (!I2CDev.res){
		delay(500); //change to RTC
    I2CDev.res = 6; //connection error
	}

  for (uint8_t i = 0; i <numBytes; i++){ //fix this, sizeof
    buff[i] = I2CDev.bufferData[i];
  }
  
	return;

}
