#include "tinyAVRI2C.h"
#include <stdint.h>
#include "Arduino.h"
#include <avr/sleep.h>
#define wif (TWI0.MSTATUS & (1<<6))
#define rif  (TWI0.MSTATUS & 0x80)
#define rxack  (TWI0.MSTATUS & 0x10)
#define clkhold (TWI0.MSTATUS & 0x20)
 


I2CDevice I2CDev;

//res: 1 -> done reading register bytes. 2-> recieved NACK from client after register byte sent. 3 -> missing ack when sending the address byte. 5-> lost arbitration
ISR(TWI0_TWIM_vect){	

  if (wif && I2CDev.stepz == 0){
		//if WIF set, and done transmitting the slave addr (address packet)
    
    TWI0.MSTATUS |= (1<<6); //clears WIF flag
    if (clkhold && !rxack){
			//Case M1, this is the place to be
      TWI0.MDATA = I2CDev.regAddr; //send reg address, auto clears WIF flag    
      I2CDev.stepz = 1; //signifies new thread for readRegister / writeRegister. Used in further if conditions for new code
      return;
		}
		else if (rxack){ //RXACK = 1
			//M3
			I2CDev.res = 3;
			TWI0.MCTRLB |= 0x03; //STOP cmd
		}
		else {
      //M4
			I2CDev.res = 5;
      TWI0.MSTATUS |= (1<<6); //manually clears int flags
      return; //need to handle this properly. Wait for bus IDLE then send stop.   
		}


	}
  else if (wif && I2CDev.stepz == 1){ //wif set, in register transmission mode
    if (!rxack){
      //recieved ACK from slave
      if (!I2CDev.writing){
        uint8_t addrR = I2CDev.slaveAddr << 1 | 1;
        
        TWI0.MADDR = addrR; //need to add interrupt for wif on stepz == 2 for lost arbitration etc (not M2); The only interrupt code after this for stepz == 2 is for when a byte has been sent, which is assuming transmission all good.
        I2CDev.stepz = 2;
        return;
      }
      else {
        //writeReg pathway after sending register addr
        //send data over
        //this is the return spot when data byte sent and ready for new one
        I2CDev.stepz = 1; //still sits on same WIF interrupt code for writing data bytes
        if (I2CDev.byteWriteCount == I2CDev.numberBytes){ //if finished sending all data bytes
          TWI0.MCTRLB |= 0x3; //STOP cmd
          I2CDev.res = 1; //all good, intended behaviour
        }
        TWI0.MDATA = I2CDev.dataWrite[I2CDev.byteWriteCount]; //sends byte of data, auto clears flag
        I2CDev.byteWriteCount++;
      }
    }
    else if ((TWI0.MSTATUS & 0x10) == 1){
      //recieved nack from client after sending register address, something went wrong, stop condition.
      TWI0.MCTRLB |= 0x3; //STOP cmd, auto clears flag
      I2CDev.res = 2;
      return;
    }
  }

  else if (rif && I2CDev.stepz == 2){
    if (clkhold){
      //clk hold set, recieved byte of data from slave
      
      I2CDev.dataRead[I2CDev.byteReadCount] = TWI0.MDATA; //auto clears RIF flag
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
  //TWI peripheral set up, sits idle in meantime
	TWI0.MBAUD = 20; //sets baud
	TWI0.MCTRLA |= 1; //enables TWI Host
  TWI0.MSTATUS = 1; //bus state idle
	TWI0.MCTRLA |= (1<<6) | (1<<7); //enabling WIF/RIF interrupts
  I2CDev.stepz = 0;
  I2CDev.byteReadCount = 0;

  //timeout


	sei();
}


void I2CDevice::setSlaveAddress(uint8_t saddr){
  //re-defines I2C Slave Address
  I2CDev.slaveAddr = saddr;
}

void I2CDevice::writeRegister(uint8_t regaddr, uint8_t* val, uint8_t sizeData){
  memset(I2CDev.dataWrite, 0, sizeof(I2CDev.dataWrite));
  //vars to keep track
  I2CDev.stepz = 0;
  I2CDev.byteWriteCount = 0;
  I2CDev.res = 0; //response
  I2CDev.writing = 1; //indeed we are writting

  I2CDev.regAddr = regaddr;
  I2CDev.numberBytes = sizeData;
  if ((sizeData-1) >> 5){
    //only accepts 32 bytes of data (32 bit). Memory constrained, can be increased if you want. 
    return;
  }
  else{
    for (uint8_t i = 0; i < sizeData; i++){
      I2CDev.dataWrite[i] = val[i]; //transfers data to be written
    }
  }

  TWI0.MADDR = (I2CDev.slaveAddr << 1); //writes the host address (i2c slave address), inits the communication. DIR = 0

  while (!I2CDev.res){
    //waits for an interrupt
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();
    sleep_cpu();
    if (I2CDev.res == 5){
      //wait until bus idle before moving on
      while ((TWI0.MSTATUS & 0x03) != 0x01);
    }
    
  }
}


void I2CDevice::readRegister(uint8_t regaddr, uint8_t numBytes){
  memset(I2CDev.dataRead, 0, sizeof(I2CDev.dataRead)); //clears dataRead ready for new
  I2CDev.stepz = 0;
  I2CDev.byteReadCount = 0;
  I2CDev.res = 0;
  I2CDev.writing = 0;
  I2CDev.regAddr = regaddr;
  I2CDev.numberBytes = numBytes;

	TWI0.MADDR = (I2CDev.slaveAddr << 1);
	
	while (!I2CDev.res){
		//idle sleep
    set_sleep_mode(SLEEP_MODE_IDLE);
    sleep_enable();
    sleep_cpu();
    if (I2CDev.res == 5){
      //wait until bus idle before moving on
      while ((TWI0.MSTATUS & 0x03) != 0x01);
    }
    
	}
}

uint32_t I2CDevice::data32(bool lsb){
  //lsb: 1 if I2C slave outputs least significant byte first
  if (sizeof(I2CDev.dataRead) < 4){
    return 0;
  }
  uint32_t output;
  if (lsb){
    for (uint8_t i = 0; i < 4; i++){
      output |= (uint32_t)(I2CDev.dataRead[i] << (i << 3));
    }
  }
  else {
    for (uint8_t i = 0; i < 4; i++){
      output |= (uint32_t)(I2CDev.dataRead[3 ^ i] << (i << 3));
    }
  }

  return output;

}

uint16_t I2CDevice::data16(bool lsb){
  if (sizeof(I2CDev.dataRead) < 2){
    return 0;
  }

  uint16_t output;

  if (lsb){
    for (uint8_t i = 0; i < 2; i++){
      output |= (uint16_t)(I2CDev.dataRead[i] << (i << 3));
    }
  }
  else {
    for (uint8_t i = 0; i < 2; i++){
      output |= (uint16_t)(I2CDev.dataRead[1 ^ i] << (i << 3));
    }
  }

  return output;

}


