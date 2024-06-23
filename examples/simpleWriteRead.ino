
//Copyright Jaxon Anthony
//I2CDev.begin() initialises the TWI peripheral. readRegister requests and stores 2 bytes of data from a device with slave address of 0x48, register address 0x00, storing it in `data`. Likewise, writeReg writes 2 bytes from `data` to 0x02 rregister on the same device. 
//refer to https://github.com/jaxon-me/tinyAVRI2C for more information.

#include "I2CHex.h"

uint8_t data[2];
void setup() {
  // put your setup code here, to run once:
  I2CDev.begin();
    
}

void loop() {
   I2CDev.readRegister(0x48, 0x00, 2, data);
   delay(1000);
   I2CDev.writeReg(0x48, 0x02, data, 2);
   delay(1000);
}
