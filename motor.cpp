#include "motor.h"
#include "encoder.h"
#include <iostream>
#include <pico/time.h>
#include "hardware/gpio.h"
#include "eeprom.h"
#include "defines.h"


// External variables from main.cpp
extern volatile bool sw1StateChanged;
extern volatile bool stopMotor;
extern volatile bool isCalibrated;
extern volatile bool stopCalib;
extern volatile bool motorStuck;

#define ENCODER_A 27
#define ENCODER_B 28

int currentSteps = 0; // Tracks steps when motor is moving.
int stepCount = 0; // How many steps to up and down.
int minSteps; // Will be automatically updated when the power goes from 0->1.
int maxSteps; // Will be automatically updated when the power goes from 0->1.
bool doorOpen; // Will be automatically updated when the power goes from 0->1.
bool doorClosed; // Will be automatically updated when the power goes from 0->1.
#define SLEEP_MS 1200;

void set_motor_pins(const uint8_t *step) {
    gpio_put(IN1, step[0]);
    gpio_put(IN2, step[1]);
    gpio_put(IN3, step[2]);
    gpio_put(IN4, step[3]);
}


//How close we would like to run the motor to the doors.
void Motor::setMinMax() {
    maxSteps = (m_Eeprom->singleRead(STEP_COUNT, true) * 96) / 100;
    minSteps = (m_Eeprom->singleRead(STEP_COUNT, true) * 2) / 100;
    std::cout << maxSteps << " max steps" << std::endl;
    std::cout << minSteps << " min steps" << std::endl;
}
//When a new state occurs we save the change.
void Motor::saveCurrentState() {
    // Only save door open/closed status
    if (isDoorOpen()) {
        m_Eeprom->singleWrite(DOOR_OPEN, true, false);
        m_Eeprom->singleWrite(DOOR_CLOSED, false, false);
    } else if (isDoorClosed()) {
        m_Eeprom->singleWrite(DOOR_OPEN, false, false);
        m_Eeprom->singleWrite(DOOR_CLOSED, true, false);
    } else {
        // Door is in between
        m_Eeprom->singleWrite(DOOR_OPEN, false, false);
        m_Eeprom->singleWrite(DOOR_CLOSED, false, false);
        m_Eeprom->singleWrite(CALIBRATION, false, false);
    }
}
//We load the state of the mashine using this function, when the power goes 0->1.
void Motor::loadSavedState() {
    // Load door status
    const bool savedDoorOpen = m_Eeprom->singleRead(DOOR_OPEN, false);
    const bool savedDoorClosed = m_Eeprom->singleRead(DOOR_CLOSED, false);

    // Set motor direction based on door state
    if (savedDoorOpen) {
        currentSteps = (m_Eeprom->singleRead(STEP_COUNT, true) * 97) / 100;
        std::cout << "NEW CURRENT STEP LOADING " << currentSteps << std::endl;
        std::cout << "SAVED OPEN" << std::endl;
        doorOpen = true;
        doorClosed = false;
        lastDirection = DOOR_LAST_OPENING;
        currentState = MOTOR_STOPPED;
        previousState = MOTOR_MOVING_UP;
    } else if (savedDoorClosed) {
        currentSteps = (m_Eeprom->singleRead(STEP_COUNT, true) * 3) / 100;
        std::cout << "NEW CURRENT STEP LOADING" << currentSteps << std::endl;
        std::cout << "SAVED CLOSED" << std::endl;
        doorOpen = false;
        doorClosed = true;
        lastDirection = DOOR_LAST_CLOSING;
        currentState = MOTOR_STOPPED;
        previousState = MOTOR_MOVING_DOWN;
    } else {
        isCalibrated = false;
        doorOpen = false;
        doorClosed = false;
        lastDirection = DOOR_LAST_CLOSING;
        currentState = MOTOR_STOPPED;
        previousState = MOTOR_STOPPED;
    }


    std::cout << "Loaded door state from EEPROM: " << std::endl;
    std::cout << "Door open: " << doorOpen << std::endl;
    std::cout << "Door closed: " << doorClosed << std::endl;
    std::cout << "Current state: " << currentState << std::endl;
    std::cout << "Last direction: " << lastDirection << std::endl;
}
//Handles the buttons press logic to run the motor.
void Motor::updateMotorState() {

    // Store the previous state before changing it
    previousState = currentState;

    // Check if the motor is currently moving
    if (currentState == MOTOR_MOVING_UP || currentState == MOTOR_MOVING_DOWN) {
        if (currentState == MOTOR_MOVING_UP) {
            lastDirection = DOOR_LAST_OPENING;
        } else {
            lastDirection = DOOR_LAST_CLOSING;
        }
        // Door is currently moving, so stop it
        std::cout << "Door was moving, now stopped." << std::endl;
        currentState = MOTOR_STOPPED;
        stop();
        // Save the direction we were moving for the next button press
    } else {
        // Door was stopped, determine which way to move

        // If the door is fully closed, we should open it
        if (isDoorClosed()) {
            std::cout << "Door is closed, now opening." << std::endl;
            currentState = MOTOR_MOVING_UP;
            moveUntilTop();
            lastDirection = DOOR_LAST_OPENING;
        }
        // If the door is fully open, we should close it
        else if (isDoorOpen()) {
            std::cout << "Door is open, now closing." << std::endl;
            currentState = MOTOR_MOVING_DOWN;
            moveUntilBottom();
            lastDirection = DOOR_LAST_CLOSING;
        }
        // Door was stopped mid-movement by a previous button press
        // Move in the opposite direction of the last movement
        else {
            if (lastDirection == DOOR_LAST_OPENING) {
                std::cout << "Door was stopped while opening, now closing." << std::endl;
                currentState = MOTOR_MOVING_DOWN;
                moveUntilBottom();
                lastDirection = DOOR_LAST_CLOSING;
            } else {
                std::cout << "Door was stopped while closing, now opening." << std::endl;
                currentState = MOTOR_MOVING_UP;
                moveUntilTop();
                lastDirection = DOOR_LAST_OPENING;
            }
        }
    }

    // Save updated state to EEPROM
    saveCurrentState();
}

void Motor::stop() {
    stopMotor = true;
}

//To know how long to run the motor towards one direction.
bool Motor::isDoorClosed() {
    // Door is closed when either the bottom limit switch is triggered OR currentSteps <= minSteps
    return currentSteps <= minSteps;
}
//To know how long to run the motor towards one direction.
bool Motor::isDoorOpen() {
    // Door is open when either the top limit switch is triggered OR currentSteps >= maxSteps
    return currentSteps >= maxSteps;
}
//These for are for calibration they see if the motor is stuck inside of them.
void Motor::moveDown() {
    for (int i = 0; i < 8; i++) {
        int step = (HALF_STEP_SEQUENCE_LENGTH - (i % HALF_STEP_SEQUENCE_LENGTH)) % HALF_STEP_SEQUENCE_LENGTH;
        set_motor_pins(half_step_sequence[step]);
        sleep_us(1200);
    }
    if (stopCalib) {
        std::cout << "Stopping calib down." << std::endl;
        return;
    }
    m_Encoder->update();
}

void Motor::moveUp() {
    for (int i = 0; i < 8; i++) {
        int step = (i % HALF_STEP_SEQUENCE_LENGTH);
        set_motor_pins(half_step_sequence[step]);
        sleep_us(1200);
    }
    if (stopCalib) {
        std::cout << "Stopping calib up." << std::endl;
        return;
    }
    m_Encoder->update();
}

void Motor::moveToBottom() {
    if (!stopCalib) {
        while (gpio_get(LIMIT_SWITCH_DOWN) && !stopCalib) {
            moveDown();
        }
    }
}

void Motor::moveToTop() {
    if (!stopCalib) {
        while (gpio_get(LIMIT_SWITCH_UP) && !stopCalib) {
            moveUp();
        }
    }
}
//When a new motor is made, it needs a eeprom and an encode for its functionality.
Motor::Motor(std::shared_ptr<Eeprom> eeprom, std::shared_ptr<Encoder> encoder)
    : m_Eeprom(std::move(eeprom)), m_Encoder(std::move(encoder)) {
}


//******************************
//Up calib. below limit
//******************************

//Constant check on the loop to move the motor one direction.
void Motor::moveMotorUp() {
    for (int i = 0; i < 8; i++) {
        if (stopMotor || currentSteps >= maxSteps) {
            return;
        }
        int step = (i % HALF_STEP_SEQUENCE_LENGTH);
        set_motor_pins(half_step_sequence[step]);
        sleep_us(1200);
        currentSteps++; // Increment step count
    }
    m_Encoder->update();
}
//Constant check on the loop to move the motor one direction.
void Motor::moveMotorDown() {
    for (int i = 0; i < 8; i++) {
        if (stopMotor || currentSteps <= minSteps) {
            return;
        }
        int step = (HALF_STEP_SEQUENCE_LENGTH - (i % HALF_STEP_SEQUENCE_LENGTH)) % HALF_STEP_SEQUENCE_LENGTH;
        set_motor_pins(half_step_sequence[step]);
        sleep_us(1200);
        currentSteps--; // Decrement step count
    }
    m_Encoder->update();
}
//To know when to stop
void Motor::moveUntilTop() {
    currentState = MOTOR_MOVING_UP;
    lastDirection = DOOR_LAST_OPENING;
    std::cout << "MOVING TOP" << std::endl;
    m_Eeprom->singleWrite(CALIBRATION, false, false);//NEW LINE
    while (currentSteps < maxSteps) {
        if (stopMotor) {
            currentState = MOTOR_STOPPED;
            stop();
            return;
        }
        moveMotorUp();
    }
    if (currentSteps == maxSteps) {
        m_Eeprom->singleWrite(CALIBRATION, true, false);
    }
    std::cout << currentSteps << "CURRENT STEPS" << std::endl;
    currentState = MOTOR_STOPPED;
    stop();
}
//To know when to stop
void Motor::moveUntilBottom() {
    currentState = MOTOR_MOVING_DOWN;
    lastDirection = DOOR_LAST_CLOSING;
    std::cout << "MOVING DOWN" << std::endl;
    m_Eeprom->singleWrite(CALIBRATION, false, false);//NEW LINE
    while (currentSteps > minSteps) {
        if (stopMotor) {
            currentState = MOTOR_STOPPED;
            stop();
            return;
        }
        moveMotorDown();
    }
    if (currentSteps == minSteps) {
        m_Eeprom->singleWrite(CALIBRATION, true, false);
    }
    std::cout << currentSteps << "CURRENT STEPS" << std::endl;
    currentState = MOTOR_STOPPED;
    stop();
}

//Calibrates and saves the machine state rightaway
void Motor::calibrate() {
    motorStuck = false;
    stopCalib = false;
    isCalibrated = false;
    std::cout << "Calibrating." << std::endl;
    std::cout << stopMotor << " STOP MOTOR" << std::endl;
    std::cout << motorStuck << " STUCK" << std::endl;
    std::cout << stopCalib << " STOP CALIB" << std::endl;
    std::cout << isCalibrated << " IS CALIBRATED" << std::endl;
    if (!stopCalib) {
        moveToTop();
    }
    int step = 0;
    stepCount = 0;
    // Now move down and count steps
    while (gpio_get(LIMIT_SWITCH_DOWN) && !stopCalib) {
        // While top switch is NOT pressed
        moveDown();
        step = (step + 1) % HALF_STEP_SEQUENCE_LENGTH;
        stepCount += 8;
    }
    if (!stopCalib) {
        isCalibrated = true;
        motorStuck = false;
        stopMotor = true;
        //Because once the button is pressed it will put this bool to false.
        currentState = MOTOR_STOPPED;
        previousState = MOTOR_MOVING_DOWN;
        lastDirection = DOOR_LAST_CLOSING; // Default direction because it will close when calib is finished.
        m_Eeprom->singleWrite(STEP_COUNT, stepCount, true);
        std::cout << "STEP COUNT AFTER " << stepCount << std::endl;
        m_Eeprom->singleWrite(CALIBRATION, true, false);
        // **Manual EEPROM Writes for Door State**
        m_Eeprom->singleWrite(DOOR_OPEN, false, false);
        m_Eeprom->singleWrite(DOOR_CLOSED, true, false);
        setMinMax();
    } else {
        maxSteps = 0;
        minSteps = 0;
        stepCount = 0;
        step = 0;
    }
}


// Getter methods
//I think can be removed they are not ussed.
int Motor::getStepCount() const {
    std::cout << maxSteps << "max" << std::endl;
    std::cout << minSteps << "min" << std::endl;
    return stepCount;
}

MotorState Motor::getState() const {
    std::cout << currentState << "state" << std::endl;
    return currentState;
}
