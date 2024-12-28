#ifndef I2C_SLAVE_DEVICE
#include "i2c_master.h"

void I2CMaster::begin() {
    Serial.printf("I2CMaster::Initializing I2C: SDA=%d, SCL=%d\n", SDA_PIN, SCL_PIN);
    Wire.begin(SDA_PIN, SCL_PIN);
    Wire.setClock(I2C_FREQ);
}

void I2CMaster::printI2CStatus(uint8_t status) {
    switch(status) {
        case 0: Serial.print(":I2CMaster:: Success\n"); break;
        case 1: Serial.print(":I2CMaster:: Data too long"); break;
        case 2: Serial.print(":I2CMaster:: NACK on address"); break;
        case 3: Serial.print(":I2CMaster:: NACK on data"); break;
        case 4: Serial.print(":I2CMaster:: Other error"); break;
        default: Serial.print(":I2CMaster:: Unknown error"); break;
    }
}

void I2CMaster::scanBus() {
    byte error, address;
    int devicesFound = 0;
    Serial.println("\n=== I2CMaster::I2C Bus Scan ===");
    Serial.printf("I2CMaster::Clock frequency: %d Hz\n", I2C_FREQ);
    Serial.println("\n\nI2CMaster::Testing addresses ..");

    for(address = 1; address < 127; address++) {
        Wire.beginTransmission(address);
        error = Wire.endTransmission();
        Serial.print("\t0x");
        Serial.print(address, HEX);

        printI2CStatus(error);
        if (error == 0) {
            devicesFound++;
            Serial.printf("\nI2CMaster::Attempting to read from device 0x%02X: ", address);
            uint8_t bytesReceived = Wire.requestFrom(address, (uint8_t)1);
            if (bytesReceived) {
                Serial.printf("\nI2CMaster::Received %d bytes\n", bytesReceived);
                while(Wire.available()) {
                    byte data = Wire.read();
                    Serial.printf(" Data: 0x%02X\n", data);
                }
            } else {
                Serial.println("I2CMaster::No data received");
            }
        }
        delay(10);
    }
    Serial.printf("\nScan complete. Found %d device(s)\n", devicesFound);
    Serial.println("===================\n");
}

void I2CMaster::checkPinStates() {
    Serial.println("I2CMaster::Checking I2C bus state:");
    Serial.printf("I2CMaster::SDA line state: %d\n", digitalRead(SDA_PIN));
    Serial.printf("I2CMaster::SCL line state: %d\n", digitalRead(SCL_PIN));
}

String I2CMaster::communicateWithSlave(uint8_t slaveAddr, const char* message) {
    size_t messageLength = strlen(message);
    if (messageLength > I2C_BUFFER_LIMIT) {
        return String("I2CMaster::Error: Message length ") + messageLength + 
               " exceeds I2C buffer limit of " + I2C_BUFFER_LIMIT + " bytes";
    }

    Serial.printf("\nI2CMaster::Attempting communication with device #%02X", slaveAddr);
    Wire.beginTransmission(slaveAddr);
    Serial.printf("\nI2CMaster::Sending data to slave device: #%02X", slaveAddr);
    Wire.write((uint8_t*)message, messageLength);
    uint8_t result = Wire.endTransmission(slaveAddr);

    Serial.print("... connection attempt result: ");
    printI2CStatus(result);
    if (result == 0) {
        Serial.printf("\nI2CMaster::Requesting %d bytes from device #%02X", I2C_BUFFER_LIMIT, slaveAddr);
        uint8_t bytesReceived = Wire.requestFrom(slaveAddr, I2C_BUFFER_LIMIT);        String response = "\n\nReceived byte: >>>>> ";
        while (Wire.available()) {
            char c = Wire.read();
            response += c;
            response += " ";
        }
        response += String("\nI2CMaster::Received ") + bytesReceived + " bytes";
        response += String("\nI2CMaster::RESPONSE") + response + "...";
        Wire.endTransmission();
        return response;
    }
    return String("I2CMaster::Communication failed with error code: ") + result;
}

#endif