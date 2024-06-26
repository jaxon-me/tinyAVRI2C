#include <stdint.h> 

#ifndef tinyAVRI2C_H
#define tinyAVRI2C_H
class I2CDevice {
  
	    
	public:
		void begin();
    void setSlaveAddress(uint8_t saddr);
		void readRegister(uint8_t regaddr, uint8_t numBytes);
		void writeRegister(uint8_t regaddr, uint8_t* val, uint8_t sizeData);

    
    uint32_t data32(bool lsb);
    uint16_t data16(bool lsb);
    uint8_t res;
    uint8_t stepz;
    uint8_t byteReadCount;
    uint8_t numberBytes;
    uint8_t regAddr;
    uint8_t dataRead[4];
    uint8_t dataWrite[32];
    uint8_t byteWriteCount;
    uint8_t writing;
    uint8_t slaveAddr;


};
extern I2CDevice I2CDev;


#endif


