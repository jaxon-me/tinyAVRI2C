//Copyright Jaxon Anthony
//I2CDev.begin() initialises the TWI peripheral. readRegister requests and stores 2 bytes of data from a device with slave address of 0x48, register address 0x00, storing it in `data`. Likewise, writeReg writes 2 bytes from `data` to 0x02 rregister on the same device. 
//refer to https://github.com/jaxon-me/tinyAVRI2C for more information.
#include "tinyAVRI2C.h"
void setup() {
  // put your setup code here, to run once:
  I2CDev.begin();
  I2CDev.setSlaveAddress(96);
  Serial.begin(115200);
  PORTA.DIRSET = (1<<5);
  PORTA.OUTCLR = (1<<5);


 
}

void loop() {

  
  // I2CDev.readRegister(0x1F, 4);
  // uint32_t data = I2CDev.data32(1);
  // Serial.printHexln(data);
   uint8_t dataWrite[4];

  dataWrite[0] = 0x6E;
  dataWrite[1] = 0x65;
  dataWrite[2] = 0x70;
  dataWrite[3] = 0x4F;

  uint8_t *ptr = dataWrite;
  I2CDev.writeRegister(0x2F,ptr, 4);
  delay(1000);
  PORTA.OUTTGL = (1<<5);
}
