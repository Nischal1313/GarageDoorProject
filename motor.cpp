#include "motor.h"
#include <iostream>
#include <pico/time.h>
#include "door.h"
#include "hardware/gpio.h"


// External variables from main.cpp
extern volatile bool sw1StateChanged;
extern volatile bool stopMotor;
// int stepCount = 13544; // Stores steps between open and close
int currentSteps = 0; // We  are likely to have 13544 steps 13544 / 8 = 1693. We want to limit top to 1693 - 16.
int stepCount = 0; // Assume starting position is midway
int minSteps; // 22 steps above the bottom
int maxSteps; // 22 steps below the top


void set_motor_pins(const uint8_t *step) {
    gpio_put(IN1, step[0]);
    gpio_put(IN2, step[1]);
    gpio_put(IN3, step[2]);
    gpio_put(IN4, step[3]);
}

void Motor::stop() {
    stopMotor = true;
}


bool Motor::isDoorOpen() {
    // Door is open when the top limit switch is triggered
    return !gpio_get(LIMIT_SWITCH_UP);
}

bool Motor::isDoorClosed() {
    // Door is closed when the bottom limit switch is triggered
    return !gpio_get(LIMIT_SWITCH_DOWN);
}

void Motor::moveDown() {
    for (int i = 0; i < 8; i++) {
        int step = (HALF_STEP_SEQUENCE_LENGTH - (i % HALF_STEP_SEQUENCE_LENGTH)) % HALF_STEP_SEQUENCE_LENGTH;
        set_motor_pins(half_step_sequence[step]);
        sleep_us(1000);
    }
}

void Motor::moveUp() {
    for (int i = 0; i < 8; i++) {
        int step = (i % HALF_STEP_SEQUENCE_LENGTH);
        set_motor_pins(half_step_sequence[step]);
        sleep_us(1000);
    }
}

//
void Motor::moveToBottom() {
    while (gpio_get(LIMIT_SWITCH_DOWN)) {
        moveDown();
    }
}

void Motor::moveToTop() {
    while (gpio_get(LIMIT_SWITCH_UP)) {
        moveUp();
    }
}

//Up calib. below limit

void Motor::moveMotorUp() {
    for (int i = 0; i < 8; i++) {
        if (stopMotor || currentSteps >= maxSteps) {
            return;
        }
        int step = (i % HALF_STEP_SEQUENCE_LENGTH);
        set_motor_pins(half_step_sequence[step]);
        sleep_us(1000);
        currentSteps++; // Increment step count
    }
}

void Motor::moveMotorDown() {
    for (int i = 0; i < 8; i++) {
        if (stopMotor || currentSteps <= minSteps) {
            return;
        }
        int step = (HALF_STEP_SEQUENCE_LENGTH - (i % HALF_STEP_SEQUENCE_LENGTH)) % HALF_STEP_SEQUENCE_LENGTH;
        set_motor_pins(half_step_sequence[step]);
        sleep_us(1000);
        currentSteps--; // Decrement step count
    }
}

void Motor::moveUntilTop() {
    currentState = MOTOR_MOVING_UP;
    lastDirection = DOOR_LAST_OPENING;
    while (currentSteps < maxSteps) {
        if (stopMotor) {
            currentState = MOTOR_STOPPED;
            stop();
            return;
        }
        moveMotorUp();
    }
    currentState = MOTOR_STOPPED;
    stop();
}

void Motor::moveUntilBottom() {
    currentState = MOTOR_MOVING_DOWN;
    lastDirection = DOOR_LAST_CLOSING;

    while (currentSteps > minSteps) {
        if (stopMotor) {
            currentState = MOTOR_STOPPED;
            stop();
            return;
        }
        moveMotorDown();
    }
    currentState = MOTOR_STOPPED;
    stop();
}


// void Motor::moveMotorUp() {
//     for (int i = 0; i < 8; i++) {
//         if (stopMotor) {
//             return; // Exit if button pressed
//         }
//         int step = (i % HALF_STEP_SEQUENCE_LENGTH);
//         set_motor_pins(half_step_sequence[step]);
//         sleep_us(1000);
//     }
// }
//
// void Motor::moveMotorDown() {
//     for (int i = 0; i < 8; i++) {
//         if (stopMotor) {
//             return; // Exit if button pressed
//         }
//         int step = (HALF_STEP_SEQUENCE_LENGTH - (i % HALF_STEP_SEQUENCE_LENGTH)) % HALF_STEP_SEQUENCE_LENGTH;
//         set_motor_pins(half_step_sequence[step]);
//         sleep_us(1000);
//     }
// }
//
// void Motor::moveUntilTop() {
//     currentState = MOTOR_MOVING_UP;
//     lastDirection = DOOR_LAST_OPENING;
//
//     while (!isDoorOpen()) {
//         if (stopMotor) {
//             currentState = MOTOR_STOPPED;
//             stop();
//             return;
//         }
//         moveMotorUp();
//     }
//     // We've reached the top
//     currentState = MOTOR_STOPPED;
//     stop();
// }
//
// void Motor::moveUntilBottom() {
//     currentState = MOTOR_MOVING_DOWN;
//     lastDirection = DOOR_LAST_CLOSING;
//
//     while (!isDoorClosed()) {
//         if (stopMotor) {
//             currentState = MOTOR_STOPPED;
//             stop();
//             return;
//         }
//         moveMotorDown();
//     }
//
//     // We've reached the bottom
//     currentState = MOTOR_STOPPED;
//     stop();
// }
//


// Definition of public methods
void Motor::calibrate() {
    moveToTop();
    int step = 0;
    // Now move down and count steps
    stepCount = -100; // Reset counter before counting
    while (gpio_get(LIMIT_SWITCH_DOWN)) {
        // While top switch is NOT pressed
        moveDown();
        step = (step + 1) % HALF_STEP_SEQUENCE_LENGTH;
        stepCount += 8;
    }
    std::cout << stepCount << std::endl;
    maxSteps = ((stepCount * 97) / 100);
    std::cout <<  maxSteps << std::endl;
    // Floors 0.96 * stepCount
    minSteps = ((stepCount * 3) / 100);
    std::cout << minSteps << std::endl;
    // Floors 0.04 * stepCount
}

MotorState Motor::getState() const {
    return currentState;
}

void Motor::updateMotorState() {
    // Blink LED to indicate state change
    for (int i = 0; i < 2; i++) {
        gpio_put(LED_ERROR, true);
        sleep_ms(100);
        gpio_put(LED_ERROR, false);
        sleep_ms(100);
    }
    std::cout << currentSteps << std::endl;

    // Save the previous state
    previousState = currentState;

    // Determine new state based on current door position and movement
    if (currentState == MOTOR_STOPPED) {
        // Door was stopped
        if (isDoorClosed()) {
            // Door is closed → start opening
            moveUntilTop();
        } else if (isDoorOpen()) {
            // Door is open → start closing
            moveUntilBottom();
        } else {
            // Door was stopped mid-movement → continue in opposite direction
            if (lastDirection == DOOR_LAST_OPENING) {
                // Was opening, now close
                moveUntilBottom();
            } else {
                // Was closing, now open
                moveUntilTop();
            }
        }
    } else if (currentState == MOTOR_MOVING_UP || currentState == MOTOR_MOVING_DOWN) {
        // Door is currently moving → stop it
        currentState = MOTOR_STOPPED;
        stop();
    }
}

// Getter methods
int Motor::getStepCount() const {
    return stepCount;
}
