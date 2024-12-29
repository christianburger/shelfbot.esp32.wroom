#ifndef SHELFBOT_MOTOR_H
#define SHELFBOT_MOTOR_H

#define NUM_MOTORS 6

#include <FastAccelStepper.h>

class ShelfbotMotor {
public:
    static void begin();
    static void moveAllMotors(long position);

    static void nonBlockingMoveAllMotors(long position);
    static bool areAllMotorsStopped();

    static String setMotor(uint8_t index, long position);
    static String getMotorPosition(uint8_t index);
    static String getMotorVelocity(uint8_t index);
    static String stopMotor(uint8_t index);
    static String stopAllMotors();
    static void setAllMotorSpeeds(long speed);
    static bool isMotorRunning(uint8_t index);
    static void printMotorSpeeds();
    
private:
    static FastAccelStepperEngine engine;
    static FastAccelStepper* steppers[6];
    static const int motorPins[6][2];
};

#endif