#include "shelfbot_motor.h"
#include "shelfbot_comms.h"

FastAccelStepperEngine ShelfbotMotor::engine = FastAccelStepperEngine();
FastAccelStepper* ShelfbotMotor::steppers[6] = {nullptr};

const int ShelfbotMotor::motorPins[6][2] = {
    {27, 26}, // Motor 1: STEP: GPIO27   DIR: GPIO26 
    {14, 12}, // Motor 2: STEP: GPIO14   DIR: GPIO12
    {13, 15}, // Motor 3: STEP: GPIO13   DIR: GPIO15
    {4, 16},  // Motor 4: STEP: GPIO4    DIR: GPIO16
    {17, 5},  // Motor 5: STEP: GPIO17   DIR: GPIO5
    {18, 19}  // Motor 6: STEP: GPIO18   DIR: GPIO19
};

void ShelfbotMotor::begin() {
    engine.init();
    
    for (int i = 0; i < NUM_MOTORS; i++) {
        steppers[i] = engine.stepperConnectToPin(motorPins[i][0]);
        if (steppers[i]) {
            steppers[i]->setDirectionPin(motorPins[i][1]);
            steppers[i]->setAutoEnable(true);
            steppers[i]->setSpeedInHz(4000);     // Match original max speed
            steppers[i]->setAcceleration(5000);   // Match original acceleration
            steppers[i]->setCurrentPosition(0);   // Initialize position
        }
    }
}

void ShelfbotMotor::nonBlockingMoveAllMotors(long position) {
    Serial.print("\nShelfbotMotor::nonBlockingMoveAllMotors - Checking motors status before move to position: ");
    Serial.println(position);
    
    // Check if any motor is still running
    for (int i = 0; i < NUM_MOTORS; i++) {
        if (steppers[i] && steppers[i]->isRunning()) {
            Serial.print("ShelfbotMotor::nonBlockingMoveAllMotors - Motor ");
            Serial.print(i);
            Serial.println(" is still running - skipping move");
            return;
        }
    }
    
    Serial.println("ShelfbotMotor::nonBlockingMoveAllMotors - All motors stopped - starting move");
    
    // Set target positions for all motors
    for (int i = 0; i < NUM_MOTORS; i++) {
        if (steppers[i]) {
            Serial.print("ShelfbotMotor::nonBlockingMoveAllMotors - Setting motor ");
            Serial.print(i);
            Serial.print(" to position: ");
            Serial.println(position);
            steppers[i]->moveTo(position, false);
        }
    }
    
    Serial.println("ShelfbotMotor::nonBlockingMoveAllMotors - Move commands sent to all motors");
}

bool ShelfbotMotor::areAllMotorsStopped() {
    for (int i = 0; i < NUM_MOTORS; i++) {
        if (steppers[i] && steppers[i]->isRunning()) {
            return false;
        }
    }
    return true;
}
void ShelfbotMotor::moveAllMotors(long position) {
    // Set target positions
    for (int i = 0; i < NUM_MOTORS; i++) {
        if (steppers[i]) {
            steppers[i]->moveTo(position, false);  // non-blocking move
        }
    }
    
    // Wait until all motors have completed their moves
    while (true) {
        bool allDone = true;
        for (int i = 0; i < NUM_MOTORS; i++) {
            if (steppers[i] && steppers[i]->isRunning()) {
                allDone = false;
                break;
            }
        }
        if (allDone) break;
    }
}

String ShelfbotMotor::setMotor(uint8_t index, long position) {
    if (index >= NUM_MOTORS || !steppers[index]) {
        return ShelfbotComms::formatResponse(RESP_ERR_MOTOR, String(index));
    }
    
    steppers[index]->moveTo(position);
    String status = String(steppers[index]->getCurrentPosition()) + "," +
                   String(position) + "," +
                   String(steppers[index]->isRunning());
    
    return ShelfbotComms::formatResponse(RESP_MOVING, status);
}

String ShelfbotMotor::getMotorPosition(uint8_t index) {
    if (index >= NUM_MOTORS || !steppers[index]) {
        return "ERR_MOTOR";
    }
    return String(steppers[index]->getCurrentPosition());
}

String ShelfbotMotor::getMotorVelocity(uint8_t index) {
    if (index >= NUM_MOTORS || !steppers[index]) {
        return "ERR_MOTOR";
    }
    return String(steppers[index]->getCurrentSpeedInUs());
}

String ShelfbotMotor::stopMotor(uint8_t index) {
    if (index >= NUM_MOTORS || !steppers[index]) {
        return "ERR_MOTOR";
    }
    steppers[index]->forceStop();
    return String(steppers[index]->getCurrentPosition());
}

String ShelfbotMotor::stopAllMotors() {
    for (int i = 0; i < NUM_MOTORS; i++) {
        if (steppers[i]) {
            steppers[i]->forceStop();
        }
    }
    return "STOPPED";
}

void ShelfbotMotor::setAllMotorSpeeds(long speed) {
    for (int i = 0; i < NUM_MOTORS; i++) {
        if (steppers[i]) {
            steppers[i]->setSpeedInHz(speed);
        }
    }
}

bool ShelfbotMotor::isMotorRunning(uint8_t index) {
    if (index >= NUM_MOTORS || !steppers[index]) {
        return false;
    }
    return steppers[index]->isRunning();
}

void ShelfbotMotor::printMotorSpeeds() {
    for (int i = 0; i < NUM_MOTORS; i++) {
        if (steppers[i]) {
            Serial.print("Motor ");
            Serial.print(i + 1);
            Serial.print(" speed: ");
            Serial.println(steppers[i]->getCurrentSpeedInUs());
        }
    }
}
