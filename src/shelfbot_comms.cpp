#include "shelfbot_comms.h"

#ifdef I2C_SLAVE_DEVICE
extern AccelStepper steppers[];
#endif

void ShelfbotComms::begin() {
    Serial.begin(115200);
    #ifdef I2C_SLAVE_DEVICE
    Serial.println("\nStarting I2C as slave");
    I2CSlave::begin();
    #else
    Serial.println("\nStarting I2C as master");
    I2CMaster::begin();
    #endif
}
String ShelfbotComms::formatResponse(CommandResponse resp, const String& value) {
    String response;
    response.reserve(32);
    response += (char)(resp >> 8);      // High byte of response code
    response += (char)(resp & 0xFF);     // Low byte of response code
    response += value;                   // Append any additional data
    return response;
}

void ShelfbotComms::sendCommand(CommandType cmd, uint16_t value) {
    String formattedCmd = formatCommand(cmd, String(value));
    
    #ifndef I2C_SLAVE_DEVICE
    Serial.printf("\n=== COMMAND SENT ===\n");
    Serial.printf("Type: 0x%04X\n", cmd);
    Serial.printf("Value: %d\n", value);
    Serial.printf("Raw: %s\n", formattedCmd.c_str());
    
    String response = I2CMaster::communicateWithSlave(I2C_SLAVE_ADDR, formattedCmd.c_str());
    
    if (response.length() >= 2) {
        uint16_t respCode = (response[0] << 8) | response[1];
        CommandResponse responseType = static_cast<CommandResponse>(respCode);
        String responseValue = response.substring(2);
        
        Serial.printf("\n=== RESPONSE RECEIVED ===\n");
        Serial.printf("Status: %s\n", formatResponse(responseType, responseValue).c_str());
        Serial.printf("Response Code: 0x%04X\n", respCode);
        Serial.printf("Value: %s\n", responseValue.c_str());
        Serial.printf("Raw: %s\n", response.c_str());
        Serial.println("=====================\n");
    } else {
        Serial.println("ERROR: Invalid response format");
    }
    #endif
}

CommandType ShelfbotComms::parseCommand(const String& message) {
    if(message.length() < 4) return CMD_UNKNOWN;
    uint16_t cmdCode = (message[2] << 8) | message[3];
    return static_cast<CommandType>(cmdCode);
}

String ShelfbotComms::parseValue(const String& message) {
    int valueStart = message.indexOf((char)(CMD_VALUE_START >> 8));
    int valueEnd = message.indexOf((char)(CMD_VALUE_END >> 8));
    
    if(valueStart < 0 || valueEnd < 0) return "";
    valueStart += 2;
    return message.substring(valueStart, valueEnd);
}

String ShelfbotComms::formatCommand(CommandType cmd, const String& value) {
    String payload;
    payload.reserve(32);
    payload += (char)(cmd >> 8);
    payload += (char)(cmd & 0xFF);
    payload += (char)(CMD_VALUE_START >> 8);
    payload += (char)(CMD_VALUE_START & 0xFF);
    payload += value;
    payload += (char)(CMD_VALUE_END >> 8);
    payload += (char)(CMD_VALUE_END & 0xFF);
    
    byte checksum = 0;
    for(size_t i = 0; i < payload.length(); i++) {
        checksum ^= payload[i];
    }
    
    String command;
    command.reserve(payload.length() + 16);
    command += (char)(CMD_START_MARKER >> 8);
    command += (char)(CMD_START_MARKER & 0xFF);
    command += payload;
    command += (char)(CMD_CHECKSUM_START >> 8);
    command += (char)(CMD_CHECKSUM_START & 0xFF);
    command += String(checksum, HEX);
    command += (char)(CMD_CHECKSUM_END >> 8);
    command += (char)(CMD_CHECKSUM_END & 0xFF);
    command += (char)(CMD_END_MARKER >> 8);
    command += (char)(CMD_END_MARKER & 0xFF);
    
    return command;
}

bool ShelfbotComms::verifyChecksum(const String& message) {
    int valueStart = message.indexOf((char)(CMD_VALUE_START >> 8));
    int checksumStart = message.indexOf((char)(CMD_CHECKSUM_START >> 8));
    
    if(valueStart < 0 || checksumStart < 0) return false;
    
    byte calculatedChecksum = 0;
    for(int i = 2; i < checksumStart; i++) {
        calculatedChecksum ^= message[i];
    }
    
    String receivedChecksum = message.substring(checksumStart + 2, message.indexOf((char)(CMD_CHECKSUM_END >> 8)));
    return calculatedChecksum == strtol(receivedChecksum.c_str(), NULL, 16);
}

#ifdef I2C_SLAVE_DEVICE

String ShelfbotComms::getTemperature() {
    Serial.println("Reading temperature sensor");
    return String(analogRead(A0) * 0.48876f);
}

String ShelfbotComms::setLed(const String& value) {
    int state = value.toInt();
    if(state != 0 && state != 1) return "ERR_VALUE";
    digitalWrite(LED_BUILTIN, state);
    return String(digitalRead(LED_BUILTIN));
}

String ShelfbotComms::readAdc(const String& value) {
    int pin = value.toInt();
    if(pin < 0 || pin > A7) return "ERR_PIN";
    return String(analogRead(pin));
}

String ShelfbotComms::setPwm(const String& value) {
    int pin = value.toInt();
    int pwmValue = value.toInt();
    if(pin < 0 || pin > 13) return "ERR_PIN";
    if(pwmValue < 0 || pwmValue > 255) return "ERR_VALUE";
    analogWrite(pin, pwmValue);
    return String(analogRead(pin));
}

String ShelfbotComms::getStatus() {
    return String(millis());
}

#define NUM_MOTORS 6
void ShelfbotComms::moveAllMotors(long position) {
    Serial.print("\nShelfbotComms::moveAllMotors(");
    Serial.print(position, DEC);
    Serial.print(") <<<<<");

    // Set target position
    for (int i = 0; i < NUM_MOTORS; i++) {
        steppers[i].moveTo(position);
        steppers[i].setSpeed(steppers[i].maxSpeed());
    }
    
    // Run motors
    while (true) {
        bool allDone = true;
        for (int i = 0; i < NUM_MOTORS; i++) {
            if (steppers[i].distanceToGo() != 0) {
                steppers[i].runSpeedToPosition();
                allDone = false;
            }
        }
        if (allDone) break;
    }
}

String ShelfbotComms::setMotor(uint8_t index, const String& value) {
    Serial.print("\nShelfbotComms::setMotor - Setting motor ");
    Serial.print(index, DEC);
    Serial.print(" to position: ");
    long position = value.toInt();
    Serial.println(position, DEC);

    if(index >= 6) {
        return formatResponse(RESP_ERR_MOTOR, String(index));
    }

    steppers[index].moveTo(position);
    steppers[index].setSpeed(steppers[index].maxSpeed());

    String status = String(steppers[index].currentPosition()) + "," +
           String(steppers[index].targetPosition()) + "," +
           String(steppers[index].distanceToGo()) + "," +
           String(steppers[index].speed());
    
    return formatResponse(RESP_MOVING, status);
}

String ShelfbotComms::getMotorPosition(uint8_t index) {
    if(index >= 6) return "ERR_MOTOR";
    return String(steppers[index].currentPosition());
}

String ShelfbotComms::getMotorVelocity(uint8_t index) {
    if(index >= 6) return "ERR_MOTOR";
    return String(steppers[index].speed());
}

String ShelfbotComms::stopMotor(uint8_t index) {
    if(index >= 6) return "ERR_MOTOR";
    steppers[index].stop();
    return String(steppers[index].currentPosition());
}

String ShelfbotComms::stopAllMotors() {
    for(int i = 0; i < 6; i++) {
        steppers[i].stop();
    }
    return "STOPPED";
}

String ShelfbotComms::getBatteryLevel() {
    return String(analogRead(A0));
}

String ShelfbotComms::getSystemStatus() {
    return String(millis());
}

void ShelfbotComms::handleCommand(char* message) {
    String msg(message);
    if (!verifyChecksum(msg)) {
        #ifdef I2C_SLAVE_DEVICE
        I2CSlave::setResponse("ERR_CHECKSUM");
        #endif
        return;
    }

    CommandType cmd = parseCommand(msg);
    String value = parseValue(msg);
    String response;

    switch(cmd) {
        case CMD_GET_TEMP:
            response = getTemperature();
            break;
        case CMD_SET_LED:
            response = setLed(value);
            break;
        case CMD_READ_ADC:
            response = readAdc(value);
            break;
        case CMD_SET_PWM:
            response = setPwm(value);
            break;
        case CMD_GET_STATUS:
            response = getStatus();
            break;
        case CMD_SET_MOTOR_1:
            response = setMotor(0, value);
            break;
        case CMD_SET_MOTOR_2:
            response = setMotor(1, value);
            break;
        case CMD_SET_MOTOR_3:
            response = setMotor(2, value);
            break;
        case CMD_SET_MOTOR_4:
            response = setMotor(3, value);
            break;
        case CMD_SET_MOTOR_5:
            response = setMotor(4, value);
            break;
        case CMD_SET_MOTOR_6:
            response = setMotor(5, value);
            break;
        case CMD_GET_MOTOR_1_POS:
            response = getMotorPosition(0);
            break;
        case CMD_GET_MOTOR_2_POS:
            response = getMotorPosition(1);
            break;
        case CMD_GET_MOTOR_3_POS:
            response = getMotorPosition(2);
            break;
        case CMD_GET_MOTOR_4_POS:
            response = getMotorPosition(3);
            break;
        case CMD_GET_MOTOR_5_POS:
            response = getMotorPosition(4);
            break;
        case CMD_GET_MOTOR_6_POS:
            response = getMotorPosition(5);
            break;
        case CMD_GET_MOTOR_1_VEL:
            response = getMotorVelocity(0);
            break;
        case CMD_GET_MOTOR_2_VEL:
            response = getMotorVelocity(1);
            break;
        case CMD_GET_MOTOR_3_VEL:
            response = getMotorVelocity(2);
            break;
        case CMD_GET_MOTOR_4_VEL:
            response = getMotorVelocity(3);
            break;
        case CMD_GET_MOTOR_5_VEL:
            response = getMotorVelocity(4);
            break;
        case CMD_GET_MOTOR_6_VEL:
            response = getMotorVelocity(5);
            break;
        case CMD_STOP_MOTOR_1:
            response = stopMotor(0);
            break;
        case CMD_STOP_MOTOR_2:
            response = stopMotor(1);
            break;
        case CMD_STOP_MOTOR_3:
            response = stopMotor(2);
            break;
        case CMD_STOP_MOTOR_4:
            response = stopMotor(3);
            break;
        case CMD_STOP_MOTOR_5:
            response = stopMotor(4);
            break;
        case CMD_STOP_MOTOR_6:
            response = stopMotor(5);
            break;
        case CMD_STOP_ALL:
            response = stopAllMotors();
            break;
        case CMD_GET_BATTERY:
            response = getBatteryLevel();
            break;
        case CMD_GET_SYSTEM:
            response = getSystemStatus();
            break;
        default:
            response = "ERR_CMD";
            break;
    }
    I2CSlave::setResponse(response.c_str());
}
#endif

void ShelfbotComms::handleComms() {
    #ifdef I2C_SLAVE_DEVICE
    char* message = I2CSlave::getMessage();
    if(message != nullptr) {
        handleCommand(message);
    }
    #endif
}