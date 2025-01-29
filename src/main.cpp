#include <Arduino.h>
#include <WiFi.h>
#include <time.h>
#include <sys/time.h>
#include "esp32_logger.h"
#include "shelfbot_comms.h"
#include "shelfbot_motor.h"
#include "shelfbot_webserver.h"

// SSID and password of Wifi connection:
const char* ssid = "dlink-30C0";
const char* password = "ypics98298";

ShelfbotComms comms;
ShelfbotWebServer webServer;
void initLogging() {
    esp32_logger::ESP32Logger::init();
    esp32_logger::ESP32Logger::logSystem("ESP8266 System startup");
}

void testAllCommands() {
    Serial.println("\n=== Testing All Commands ===");

    // Basic system commands
    ShelfbotComms::sendCommand(CMD_GET_TEMP);
    ShelfbotComms::sendCommand(CMD_SET_LED, 1);
    ShelfbotComms::sendCommand(CMD_READ_ADC, 0);
    ShelfbotComms::sendCommand(CMD_SET_PWM, 128);
    ShelfbotComms::sendCommand(CMD_GET_STATUS);

    // Set all motors
    ShelfbotComms::sendCommand(CMD_SET_MOTOR_1, 1000);
    ShelfbotComms::sendCommand(CMD_SET_MOTOR_2, 1000);
    ShelfbotComms::sendCommand(CMD_SET_MOTOR_3, 1000);
    ShelfbotComms::sendCommand(CMD_SET_MOTOR_4, 1000);
    ShelfbotComms::sendCommand(CMD_SET_MOTOR_5, 1000);
    ShelfbotComms::sendCommand(CMD_SET_MOTOR_6, 1000);

    // Get all motor positions
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_1_POS);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_2_POS);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_3_POS);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_4_POS);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_5_POS);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_6_POS);

    // Get all motor velocities
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_1_VEL);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_2_VEL);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_3_VEL);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_4_VEL);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_5_VEL);
    ShelfbotComms::sendCommand(CMD_GET_MOTOR_6_VEL);

    // Stop individual motors
    ShelfbotComms::sendCommand(CMD_STOP_MOTOR_1);
    ShelfbotComms::sendCommand(CMD_STOP_MOTOR_2);
    ShelfbotComms::sendCommand(CMD_STOP_MOTOR_3);
    ShelfbotComms::sendCommand(CMD_STOP_MOTOR_4);
    ShelfbotComms::sendCommand(CMD_STOP_MOTOR_5);
    ShelfbotComms::sendCommand(CMD_STOP_MOTOR_6);

    // System commands
    ShelfbotComms::sendCommand(CMD_STOP_ALL);
    ShelfbotComms::sendCommand(CMD_GET_BATTERY);
    ShelfbotComms::sendCommand(CMD_GET_SYSTEM);

    Serial.println("=== Test Complete ===\n");
}

void nonBlockingMotorRoutine(int delayValue) {
    Serial.println("Moving to position 1000... ShelfbotMotor::nonBlockingMoveAllMotors(1000)");
    ShelfbotMotor::nonBlockingMoveAllMotors(1000);
    ShelfbotMotor::printMotorSpeeds();
    delay(delayValue);

    Serial.println("Moving to position 0... ShelfbotMotor::nonBlockingMoveAllMotors(0)");
    ShelfbotMotor::nonBlockingMoveAllMotors(0);
    ShelfbotMotor::printMotorSpeeds();
    delay(delayValue);

    Serial.println("Moving to position -2000... ShelfbotMotor::nonBlockingMoveAllMotors(-2000)");
    ShelfbotMotor::nonBlockingMoveAllMotors(-2000);
    ShelfbotMotor::printMotorSpeeds();
    delay(delayValue);

    Serial.println("Moving to position 2000... ShelfbotMotor::nonBlockingMoveAllMotors(-2000)");
    ShelfbotMotor::nonBlockingMoveAllMotors(2000);
    ShelfbotMotor::printMotorSpeeds();
    delay(delayValue);

    Serial.println("Moving to position -4000... ShelfbotMotor::nonBlockingMoveAllMotors(-2000)");
    ShelfbotMotor::nonBlockingMoveAllMotors(-4000);
    ShelfbotMotor::printMotorSpeeds();
    delay(delayValue);

    Serial.println("Moving to position 4000... ShelfbotMotor::nonBlockingMoveAllMotors(-2000)");
    ShelfbotMotor::nonBlockingMoveAllMotors(4000);
    ShelfbotMotor::printMotorSpeeds();
    delay(delayValue);
}

void motorRoutine(int delayValue) {
    Serial.println("Moving to position 1000...");
    ShelfbotMotor::moveAllMotors(1000);
    delay(2000);
    delay(delayValue);
    
    Serial.println("Moving to position 0...");
    ShelfbotMotor::moveAllMotors(0);
    delay(delayValue);
    
    Serial.println("Moving to position -1000...");
    ShelfbotMotor::moveAllMotors(-1000);
    delay(delayValue);
 
    Serial.println("Moving to position 0...");
    ShelfbotMotor::moveAllMotors(0);
    delay(delayValue);

    Serial.println("Moving to position -2000...");
    ShelfbotMotor::moveAllMotors(2000);
    delay(delayValue);
 
    Serial.println("Moving to position 0...");
    ShelfbotMotor::moveAllMotors(0);
    delay(delayValue);

    Serial.println("Moving to position -2000...");
    ShelfbotMotor::moveAllMotors(-2000);
    delay(delayValue);
    
    // Print motor speeds for monitoring
    ShelfbotMotor::printMotorSpeeds();
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    initLogging();

    testAllCommands();

    webServer.begin();
    
    ShelfbotMotor::begin();
    ShelfbotMotor::setAllMotorSpeeds(4000);

    for (int i=0; i<2; i++) {
        nonBlockingMotorRoutine(2000);
    }
}

void loop() {
    /*
    ShelfbotComms::sendCommand(CMD_SET_MOTOR_1, 1000);
    delay(4000);
    ShelfbotComms::sendCommand(CMD_SET_MOTOR_1, 0);
    delay(4000);
 
    ShelfbotComms::sendCommand(CMD_SET_MOTOR_2, 1000);
    delay(4000);
    ShelfbotComms::sendCommand(CMD_SET_MOTOR_2, 0);
    delay(4000);
 
    ShelfbotComms::sendCommand(CMD_SET_MOTOR_3, 1000);
    delay(4000);
    ShelfbotComms::sendCommand(CMD_SET_MOTOR_3, 0);
    delay(4000);

    ShelfbotComms::sendCommand(CMD_SET_MOTOR_4, 1000);
    delay(4000);
    ShelfbotComms::sendCommand(CMD_SET_MOTOR_4, 0);
    delay(4000);
    */

    delay(40);
    webServer.handle();
}
