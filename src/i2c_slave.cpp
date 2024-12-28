#ifdef I2C_SLAVE_DEVICE

#include "i2c_slave.h"
#include "shelfbot_comms.h"

uint32_t I2CSlave::requestCount = 0;
uint32_t I2CSlave::receiveCount = 0;
uint32_t I2CSlave::lastStatus = 0;
char I2CSlave::lastMessage[MAX_MESSAGE_LENGTH] = {0};
char I2CSlave::response[MAX_MESSAGE_LENGTH] = {0};

void I2CSlave::requestCallback() {
    requestCount++;
    Serial.print("I2CSlave::Request #");
    Serial.print(requestCount);
    Serial.print(" - Sending: ");
    Serial.println(response);
    Wire.write(response, strlen(response));
}

void I2CSlave::receiveCallback(int numBytes) {
    if (numBytes > 0) {
        receiveCount++;
        uint8_t i = 0;
        while (Wire.available() && i < MAX_MESSAGE_LENGTH - 1) {
            lastMessage[i++] = Wire.read();
        }
        lastMessage[i] = '\0';
        
        Serial.print("\nI2CSlave::Receive #");
        Serial.print(receiveCount);
        Serial.print(" - Got ");
        Serial.print(numBytes);
        Serial.print(" bytes: \t\t MESSAGE: ");
        Serial.println(lastMessage);
        
        ShelfbotComms::handleCommand(lastMessage);
    }
}

void I2CSlave::begin() {
    Serial.println("\nI2CSlave::I2C Slave Starting");
    Serial.print("I2CSlave::Address: 0x");
    Serial.println(I2C_ADDRESS, HEX);
    Wire.begin(I2C_ADDRESS);
    Wire.setClock(I2C_CLOCK);
    Wire.onRequest(requestCallback);
    Wire.onReceive(receiveCallback);
    Serial.println("I2CSlave::I2C Slave Ready!");
}

char* I2CSlave::getMessage() {
    return lastMessage;
}

void I2CSlave::setResponse(const char* newResponse) {
    strncpy(response, newResponse, MAX_MESSAGE_LENGTH - 1);
    response[MAX_MESSAGE_LENGTH - 1] = '\0';
    Serial.print("I2CSlave::Response set to: ");
    Serial.println(response);
}

const char* I2CSlave::getLastResponse() {
    return response;
}

void I2CSlave::printStatus() {
    Serial.println("\nI2CSlave::Status Report:");
    Serial.print("I2CSlave::Uptime: ");
    Serial.print(millis());
    Serial.println(" ms");
    Serial.print("I2CSlave::Total requests handled: ");
    Serial.println(requestCount);
    Serial.print("I2CSlave::Total receives handled: ");
    Serial.println(receiveCount);
}

void I2CSlave::handleLoop() {
    static uint32_t lastStatusTime = 0;
    if (millis() - lastStatusTime >= 5000) {
        printStatus();
        lastStatusTime = millis();
    }
}

#endif