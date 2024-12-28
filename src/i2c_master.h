#ifndef I2C_SLAVE_DEVICE
#ifndef I2C_MASTER_H
#define I2C_MASTER_H

#include <Arduino.h>
#include <Wire.h>

#define SDA_PIN 4  // GPIO04
#define SCL_PIN 2  // GPIO02
#define I2C_FREQ 10000  // Very low frequency (10kHz)
#define I2C_SLAVE_ADDR 12
#define I2C_BUFFER_LIMIT 32

class I2CMaster {
public:
    static void begin();
    static void scanBus();
    static void checkPinStates();
    static String communicateWithSlave(uint8_t slaveAddr, const char* message);


private:
    static void printI2CStatus(uint8_t status);
};

#endif
#endif