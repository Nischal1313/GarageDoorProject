//
// Created by nischal on 2/26/25.
//

#ifndef MOTOR_H
#define MOTOR_H
#include <cstdint>
#include <hardware/gpio.h>
// Define constants and enums if needed
#define HALF_STEP_SEQUENCE_LENGTH 8
#define EEPROM_ADDR 0x50

// Pin assignments
#define IN1 2
#define IN2 3
#define IN3 6
#define IN4 13

#define LIMIT_SWITCH_UP 16
#define LIMIT_SWITCH_DOWN 17





const uint8_t half_step_sequence[HALF_STEP_SEQUENCE_LENGTH][4] = {
    {1, 0, 0, 0},
    {1, 1, 0, 0},
    {0, 1, 0, 0},
    {0, 1, 1, 0},
    {0, 0, 1, 0},
    {0, 0, 1, 1},
    {0, 0, 0, 1},
    {1, 0, 0, 1}
};

// Enum to represent door states

class Motor {
private:
    int stepCount;
    bool isCalibrated;

    // Private methods (if any)
    static void moveMotorUp();

    static void moveMotorDown();

    static void moveUntilBottom();

    static void moveUntilTop();

public:
    Motor(); // Constructor

    // Public methods
    void calibrate();

    // You can also add getters and setters for private variables if needed
    [[nodiscard]] int getStepCount() const;

    [[nodiscard]] bool getIsCalibrated() const;
};

#endif // MOTOR_H
