#include "motor.h"
#include "eeprom.h"
#include <pico/time.h>

// Constructor definition
Motor::Motor() : stepCount(0), isCalibrated(false) {}

void set_motor_pins(const uint8_t *step) {
    gpio_put(IN1, step[0]);
    gpio_put(IN2, step[1]);
    gpio_put(IN3, step[2]);
    gpio_put(IN4, step[3]);
}

// Definition of private methods
void Motor::moveMotorUp() {
    for (int i = 0; i < 8; i++) {
        int step = (i % HALF_STEP_SEQUENCE_LENGTH); // Normal stepping order
        set_motor_pins(half_step_sequence[step]);
        sleep_us(1000);
    }
}

void Motor::moveMotorDown() {
    for (int i = 0; i < 8; i++) {
        int step = (HALF_STEP_SEQUENCE_LENGTH - (i % HALF_STEP_SEQUENCE_LENGTH)) % HALF_STEP_SEQUENCE_LENGTH;
        set_motor_pins(half_step_sequence[step]);
        sleep_us(1000);
    }
}

void Motor::moveUntilBottom() {
    while (gpio_get(LIMIT_SWITCH_DOWN)) {
        moveMotorDown();
    }
}

void Motor::moveUntilTop() {
    while (gpio_get(LIMIT_SWITCH_UP)) {
        moveMotorUp();
    }
}

// Definition of public methods
void Motor::calibrate() {
    moveUntilTop();
    sleep_ms(1000);
    while (!isCalibrated) {
        int step = 0;
        // Now move down and count steps
        stepCount = 0; // Reset counter before counting
        while (gpio_get(LIMIT_SWITCH_UP)) {
            // While top switch is NOT pressed
            moveMotorDown();
            step = (step + 1) % HALF_STEP_SEQUENCE_LENGTH;
            stepCount += 4;
        }
        isCalibrated = true;
    }
}

// Getter methods
int Motor::getStepCount() const {
    return stepCount;
}

bool Motor::getIsCalibrated() const {
    return isCalibrated;
}
