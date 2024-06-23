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

```stepz```: 0 when writing slave address packet, 1 when writing the register address or the data bytes, 2 when receiving data bytes. 

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

Once the host writes the MADDR to the slave, it sets a WIF flag, causing an interrupt. ```if (wif && I2CDev.stepz == 0)``` checks whether WIF flag is set and we are currently in the slave address write section. If true, then we have confirmed the device has sent the slave address. We now need to check for potential transmission/acknowledgement errors. There are 3 cases, case M1 is if there where no errors, M3 is if the host did not get an acknowledgement and M4 is if there was an arbitration or bus error. Only on M1 does the writeRegister function continue, else it just stops communication and gives up. 

If M1, found via: ```if (clkhold && !rxack)```: 

```TWI0.MDATA = I2CDev.regAddr``` writes the register address packet to the host. This automatically initiates data transmission and clears the interrupt flag. ```stepz = 1```.

For a short period while the register address is being transferred, the ISR is no longer running and the device returns to waiting. 

Upon transfer, the WIF flag is set again, we return to the ISR. Instead, the next if condition is satisfied: ```else if (wif && I2CDev.stepz == 1)```. 
Inside is code that runs if we are indeed in register address transfer stage (indeed we are). We check whether we got an ACK from the slave. If so, we can continue, else the program stops communication and gives up. 
Continuing through ```if (!rxack)``` we need to check whether we are writing or reading, as both read and write register functions have to send a register address, but do different things afterwards. 
Through the ```else``` condition, we continue on the writeRegister thread. 
Here we cycle through code that overwrites the MDATA host register with the next-in-line data byte. Each time we increment the byteWriteCount. While transmitting the data bytes, the host exits out of the ISR and waits for the next WIF interrupt. Upon the next interrupt, the program returns to the same location, where it checks whether it has finished sending all the bytes. If not, then it sends the next, the cycle continues. Only when ```byteWriteCount``` equalling the ```numberBytes``` variable does it exit ```writeRegister``` and finish. Hence, it is important that ```numberBytes``` is given the correct value. 

As you can see, if the program finds an error, it gives up without any automatic attempt to fix it, retry or indicate exactly what the error is. Consideration has been made to stop communication if errors occur, and the ```delay(500)``` ensures the program moves on regardless. 

## readRegister 

| Variable | Initial Value | 
| -------- | ------|
| writing | 0 |
| stepz | 0 | 
| byteReadCount | 0 |

For the slave address and register address transmission sections, it shares code with writeRegister. Only after confirming the register address was sent does it deviate. 

The starter code ```TWI0.MADDR = (saddr << 1);``` is the same as what is used in the writeRegister, causing shared ISR behaviour. 

### ISR code

After sending the register address and checking for errors
```
else if (wif && I2CDev.stepz == 1){ //wif set, in register transmission mode
    if (!rxack){

```

does the ```if (!I2CDev.writing)``` condition break it away. Inside it writes MADDR host register with the slave address, but with MADDR[0] = 1. This sets the DIR bit, indicating the host intends to read from the slave. ```stepz=2```.
After writing to MADDR, the WIF flag is cleared, the ISR exits and it waits for a new interrupt. Since DIR = 1, the next interrupt will be from the RIF. 

The final if conditions consider the RIF when CLKHOLD == 1:

```
else if (rif && I2CDev.stepz == 2){
    if (clkhold){
```

This is true if the slave has sent a byte to the host and is pending a response from the host. The byte read is stored in the hosts MDATA register. 

The program stores the MDATA byte in the ```bufferData``` variable prior to transfer to the ```buff```. If the host needs more bytes, it sends an ACK to the slave, else it sends a NACK, indicating the end of the communication. On ACK, it exits out of the ISR and waits for the next RIF interrupt, where it again pulls from the MDATA register, storing it in the next byte of ```bufferData``` **.


**: in the ```I2CHex.h``` file, ```bufferData``` is initialised as a 4 byte array, and hence can only store 4 bytes of slave data at a time. This is user configurable, along with the ```sizeData``` parameter, to allow for greater storage.















