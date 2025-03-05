//
// Created by nischal on 2/26/25.
//

#ifndef MOTOR_H
#define MOTOR_H
#include <cstdint>
#include <memory>
#include <hardware/gpio.h>

#include "eeprom.h"
#include "encoder.h"
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

enum MotorState {
    MOTOR_STOPPED,
    MOTOR_MOVING_UP,
    MOTOR_MOVING_DOWN
};

// Define door states to track for SW1 functionality
enum DoorDirection {
    DOOR_LAST_OPENING,
    DOOR_LAST_CLOSING
};

class Motor {
private:
    std::shared_ptr<Eeprom> m_Eeprom;
    std::shared_ptr<Encoder> m_Encoder;
    MotorState currentState = MOTOR_STOPPED;
    MotorState previousState = MOTOR_MOVING_DOWN;
    DoorDirection lastDirection = DOOR_LAST_CLOSING; // Default direction because it will close when calib is finished.

public:
    explicit Motor(std::shared_ptr<Eeprom> eeprom, std::shared_ptr<Encoder> encoder);

    void moveToTop();

    void moveUntilTop();

    void moveUntilBottom();

    void moveDown();

    void moveUp();

    void moveToBottom();

    void moveMotorUp();

    void moveMotorDown();

    Motor() = default; // Constructor
    bool isDoorOpen();

    bool isDoorClosed();

    // Public methods
    void calibrate();

    void setMinMax();

    void setDoorState();

    // Stop motor
    static void stop();

    [[nodiscard]] MotorState getState() const;

    // Handle motor movement based on door state
    void updateMotorState();

    // You can also add getters and setters for private variables if needed
    [[nodiscard]] int getStepCount() const;
};

#endif // MOTOR_H
