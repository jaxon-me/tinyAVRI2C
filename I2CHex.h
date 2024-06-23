#include <stdint.h> 

class I2CDevice {
  
	    
	public:
		void begin();
		void readRegister(uint8_t saddr, uint8_t regaddr, uint8_t numBytes, uint8_t* buff);
		void writeReg(uint8_t saddr, uint8_t regaddr, uint8_t* val, uint8_t sizeData);
    void changeTimeout(uint32_t* usTime);
    uint8_t res = 0;
    uint8_t stepz = 0;
    uint8_t byteReadCount = 0;
    uint8_t numberBytes;
    uint8_t regAddr;
    uint8_t bufferData[4];
    uint8_t byteWriteCount = 0;
    uint8_t writing = 0;
    uint8_t slaveAddr;


};

extern I2CDevice I2CDev;
