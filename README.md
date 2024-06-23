This library allows I2C communication between a host tinyAVR microcontroller and a slave device. It sacrifices rigorous error handling and cross compatibility for memory improvements, suited for lower memory models. It has two pre-programmed functions: writeRegister and readRegister.

```writeRegister(uint8_t saddr, uint8_t regaddr, uint8_t* val, uint8_t sizeData)```

This function writes to a register on the slave device. It does not do the typical read-write-modify sequence (future update), hence all bits of the register are overwritten. The function can only write a maximum of 4 bytes of data, excluding the slave and register address bytes. This is to conserve on memory but can be altered by the user.

```saddr``` : 7 bit slave device address 

```regaddr```: register address

```val```: pointer of value buffer to be written (the data byte/s)

```sizeData```: number of data bytes to be written




```readRegister(uint8_t saddr, uint8_t regaddr, uint8_t numBytes, uint8_t* buff)```

This function reads the value of a register and stores the byte/s of in a buffer.

```saddr```: 7 bit slave address

```regaddr```: register address

```numBytes```: number of bytes to read *

```buff```: pointer to the buffer storing the register bits



*: ```numBytes``` should not exceed the specific register size


# Code explanation

Calling ```begin()``` initiates the TWI peripheral, setting the baud rate, enabling interrupts, sets the microcontroller as host, and places the peripheral in IDLE mode. 

** Variables used to keep track of location in data transmission: **

```stepz```: 0 when writing slave address packet, 1 when writing the register address, 2 when sending or receiving data bytes. 

```byteWriteCount```: keeps count of number of bytes written to slave and used to check for completion.

```byteReadCount```: counts the number of bytes read from slave, used to check for completion.

```res```: 0 means it's awaiting for a response, 1 means it successfully wrote/read all bytes to the slave, 2 means it received and NACK after sending slave data bytes (error), 3 means it missed an acknowledgement after sending slave address (error), 5 means it lost arbitration and 6 means it timed out without response. Call error cases are handled by stopping communication. 

```writing```: Boolean indicating whether it is writing to the slave or reading. 

## writeRegister

| Variable | Initial Value | 
| -------- | ------|
| writing | 1 |
| stepz | 0 | 
| byteWriteCount | 0 | 
 
```TWI0.MADDR = (saddr << 1);```: MADDR[7:1] = slave address, MADDR[0] = direction bit = 0, indicating writing.

While it has not received a response (by default this is true), the host waits 500ms. During this time, interrupts can occur which will hopefully give a response. If not, then a timeout response is returned. 

### ISR code

Once the host writes the MADDR to the slave, it sets a WIF flag, causing an interrupt. ```if (wif && I2CDev.stepz == 0)```` checks whether WIF flag is set and we are currently in the slave address write section. If true, then we have confirmed the device has sent the slave address. We know need to check for potential transmission/acknowledgement errors. There are 3 cases, case M1 is if there where no errors, M3 is if the host did not get an acknowledgement and M4 is if there was an arbitration or bus error. Only on M1 does the writeRegister function continue, else it just stops communication and gives up. 

If M1, found via: ```if (clkhold && !rxack)```: 

```TWI0.MDATA = I2CDev.regAddr``` writes the register address packet to the host. This automatically initiates data transmission and clears the interrupt flag. ```stepz = 1```.

For a short period while the register address is being transferred, the ISR is no longer running and the device returns to waiting. 

Upon transfer, the WIF flag is set again, we return to the ISR. Instead, the next if condition is satisfied: ```else if (wif && I2CDev.stepz == 1)```. 
Inside is code that runs if we are indeed in register address transfer stage (indeed we are). 









