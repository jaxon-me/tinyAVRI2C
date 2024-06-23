This library allows I2C communication between a host tinyAVR microcontroller and a slave device. It sacrifices rigorous error handling and cross compatibility for memory improvements, suited for lower memory models. It has two pre-programmed functions: writeRegister and readRegister.

writeRegister(uint8_t saddr, uint8_t regaddr, uint8_t* val, uint8_t sizeData)

This function writes to a register on the slave device. It does not do the typical read-write-modify sequence (future update), hence all bits of the register are overwritten. The function can only write a maximum of 4 bytes of data, excluding the slave and register address bytes. This is to conserve on memory but can be altered by the user.

saddr : 7 bit slave device address
regaddr: register address
val: pointer of value buffer to be written (the data byte/s)
sizeData: number of data bytes to be written

readRegister(uint8_t saddr, uint8_t regaddr, uint8_t numBytes, uint8_t* buff)

This function reads the value of a register and stores the byte/s of in a buffer.

saddr: 7 bit slave address
regaddr: register address
numBytes: number of bytes to read *
buff: pointer to the buffer storing the register bits


*: numBytes should not exceed the specific register size


Code explanation

Calling begin() initialised the TWI peripheral, setting the baud rate, enabling interrupts, sets the microcontroller as host, and places the peripheral in IDLE mode. 

Variables used to keep track of location in data transmission:

stepz: 0 when writing slave address packet, 1 when writing the register address, 2 when sending or receiving data bytes. 

byteWriteCount: keeps count of number of bytes written to slave and used to check for completion.

byteReadCount: counts the number of bytes read from slave, used to check for completion.

res: 0 means it's awaiting for a response, 1 means it successfully wrote/read all bytes to the slave, 2 means it received and NACK after sending slave data bytes (error), 3 means it missed an acknowledgement after sending slave address (error), 5 means it lost arbitration and 6 means it timed out without response. Call error cases are handled by stopping communication. 

writing: Boolean indicating whether it is writing to the slave or reading. 








