#include "motor.h"
#include "encoder.h"
#include <iostream>
#include <pico/time.h>
#include "door.h"
#include "hardware/gpio.h"
#include "eeprom.h"
#include "defines.h"


// External variables from main.cpp
extern volatile bool sw1StateChanged;
extern volatile bool stopMotor;
extern volatile bool isCalibrated;

#define ENCODER_A 27
#define ENCODER_B 28

int currentSteps = 0; // Tracks steps when motor is moving.
int stepCount = 0; // How many steps to up and down.

int minSteps;
int maxSteps;


// Encoder encoder(ENCODER_A, ENCODER_B);
// Eeprom eeprom(I2C_PORT, EEPROM_ADDR);

void set_motor_pins(const uint8_t *step) {
    gpio_put(IN1, step[0]);
    gpio_put(IN2, step[1]);
    gpio_put(IN3, step[2]);
    gpio_put(IN4, step[3]);
}

void Motor::setMinMax() {
    maxSteps = (m_Eeprom->singleRead(STEP_COUNT, true) * 98) / 100;
    minSteps = (m_Eeprom->singleRead(STEP_COUNT, true) * 2) / 100;
    std::cout << maxSteps << " max steps" << std::endl;
    std::cout << minSteps << " min steps" << std::endl;
}

void Motor::setDoorState() {
    currentState = static_cast<MotorState>(m_Eeprom->singleRead(DOOR_STATE, false));
    lastDirection = static_cast<DoorDirection>(m_Eeprom->singleRead(MOVING_UP_AND_DOWN, false));
    std::cout << "current-state " << currentState << std::endl;
    std::cout << "last-dir " << lastDirection << std::endl;
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

void Motor::moveToBottom() {
    while (gpio_get(LIMIT_SWITCH_DOWN)) {
        moveDown();
    }
}


Motor::Motor(std::shared_ptr<Eeprom> eeprom, std::shared_ptr<Encoder> encoder)
    : m_Eeprom(std::move(eeprom)), m_Encoder(std::move(encoder)) {
}


void Motor::moveToTop() {
    while (gpio_get(LIMIT_SWITCH_UP)) {
        moveUp();
    }
}

//******************************
//Up calib. below limit
//******************************


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
    m_Encoder->update();
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
    m_Encoder->update();
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

void Motor::calibrate() {
    moveToTop();
    int step = 0;
    // Now move down and count steps
    while (gpio_get(LIMIT_SWITCH_DOWN)) {
        // While top switch is NOT pressed
        moveDown();
        step = (step + 1) % HALF_STEP_SEQUENCE_LENGTH;
        stepCount += 8;
        m_Encoder->update();
    }

    // After calibration, get the number of encoder turns
    isCalibrated = true;
    std::cout << m_Encoder->getStepCount() << "The encoder count." << std::endl;
    m_Eeprom->singleWrite(STEP_COUNT, stepCount, true);
    m_Eeprom->singleWrite(CALIBRATION, true, false);
    if (m_Eeprom->singleRead(CALIBRATION, false) == false) {
        std::cout << "Error" << std::endl;
    }

    std::cout << stepCount << std::endl;
    maxSteps = ((stepCount * 98) / 100);
    std::cout << maxSteps << std::endl;
    // Floors 0.96 * stepCount
    minSteps = ((stepCount * 2) / 100);
    std::cout << minSteps << std::endl;
    // Floors 0.04 * stepCount
}

void Motor::updateMotorState() {
    // Blink LED to indicate state change
    for (int i = 0; i < 2; i++) {
        gpio_put(LED_ERROR, true);
        sleep_ms(100);
        gpio_put(LED_ERROR, false);
        sleep_ms(100);
    }
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
    std::cout << m_Encoder-> getStepCount() << " The encoder count." << std::endl;
    m_Eeprom->singleWrite(DOOR_STATE, currentState, false);
    m_Eeprom->singleWrite(MOVING_UP_AND_DOWN, lastDirection, false);
}

// Getter methods
int Motor::getStepCount() const {
    std::cout << maxSteps << "max" << std::endl;
    std::cout << minSteps << "min" << std::endl;
    return stepCount;
}

MotorState Motor::getState() const {
    std::cout << currentState << "state" << std::endl;
    return currentState;
}
