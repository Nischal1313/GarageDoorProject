#include <iostream>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/gpio.h"
#include <hardware/i2c.h>
#include "door.h"
#include "IPStack.h"
#include "pins.h"
#include "motor.h"
#include "eeprom.h"
#include "defines.h"
#include "encoder.h"
#include "MQTTManager.h"

// Global variables
//We need volatile because we never know when these functions might change based on the interrupts.
// All of them will automatically update when the power goes from 0->1.
volatile bool stopMotor = true;
volatile bool stopCalib = false;
// Flag to stop motor movement. It starts as true since for the first time it will be turned to false.
volatile bool sw1StateChanged = false;
volatile bool isCalibrated = false;
volatile bool motorStuck = false;
MQTTManager *mqttManager = nullptr;

//Making an encoder class that is shared.
const auto encoder = std::make_shared<Encoder>(ENCODER_A, ENCODER_B);

//Buttons
GPIOPin sw0(BUTTON_SW0);
GPIOPin sw1(BUTTON_SW1);
GPIOPin sw2(BUTTON_SW2);
//LEDs
GPIOPin LED1(20, false, false, false);
GPIOPin LED2(21, false, false, false);
GPIOPin LED3(22, false, false, false);


// Global variable to track last valid button press time
absolute_time_t lastSW1PressTime = get_absolute_time();
//Irq handler for the button and the encoder.
void irq_handler(const uint gpio, uint32_t event_mask) {
    absolute_time_t currentTime = get_absolute_time();

    if (gpio == BUTTON_SW1) {
        // Debounce logic: Ignore presses within 400ms
        if (absolute_time_diff_us(lastSW1PressTime, currentTime) / 1000 >= 400) {
            lastSW1PressTime = currentTime; // Update last press time

            stopMotor = !stopMotor;
            sw1StateChanged = true;
        }
    } else if (gpio == ENCODER_A) {
        if (encoder->isEncoderStuck(stopMotor) && isCalibrated) {
            std::cout << "IN ENCODER MOTOR STOPPED." << std::endl;
            stopMotor = true;
            motorStuck = false;
            isCalibrated = false;
            stopCalib = false;
        }
    }
}

//Init function
void initAll() {
    stdio_init_all();

    gpio_set_irq_enabled_with_callback(BUTTON_SW1, GPIO_IRQ_EDGE_FALL, true, irq_handler);
    gpio_set_irq_enabled_with_callback(ENCODER_A, GPIO_IRQ_EDGE_FALL, true, irq_handler);

    gpio_init(IN1);
    gpio_init(IN2);
    gpio_init(IN3);
    gpio_init(IN4);

    gpio_set_dir(IN1, GPIO_OUT);
    gpio_set_dir(IN2, GPIO_OUT);
    gpio_set_dir(IN3, GPIO_OUT);
    gpio_set_dir(IN4, GPIO_OUT);

    gpio_init(BUTTON_SW0);
    gpio_set_dir(BUTTON_SW0, GPIO_IN);
    gpio_pull_up(BUTTON_SW0);

    gpio_init(BUTTON_SW1);
    gpio_set_dir(BUTTON_SW1, GPIO_IN);
    gpio_pull_up(BUTTON_SW1);

    gpio_init(BUTTON_SW2);
    gpio_set_dir(BUTTON_SW2, GPIO_IN);
    gpio_pull_up(BUTTON_SW2);


    // Initialize LED pins
    gpio_init(LED_STATUS);
    gpio_set_dir(LED_STATUS, GPIO_OUT);

    gpio_init(LED_ERROR);
    gpio_set_dir(LED_ERROR, GPIO_OUT);


    // Initialize limit switches as input with pull-up resistors
    gpio_init(LIMIT_SWITCH_UP);
    gpio_set_dir(LIMIT_SWITCH_UP, GPIO_IN);
    gpio_pull_up(LIMIT_SWITCH_UP);

    gpio_init(LIMIT_SWITCH_DOWN);
    gpio_set_dir(LIMIT_SWITCH_DOWN, GPIO_IN);
    gpio_pull_up(LIMIT_SWITCH_DOWN);


    // Initialize encoder pins as inputs
    gpio_init(ENCODER_A);
    gpio_set_dir(ENCODER_A, GPIO_IN);
    gpio_pull_up(ENCODER_A);

    gpio_init(ENCODER_B);
    gpio_set_dir(ENCODER_B, GPIO_IN);
    gpio_pull_up(ENCODER_B);


    i2c_init(I2C_PORT, 100000);
    gpio_set_function(SDA_PIN, GPIO_FUNC_I2C);
    gpio_set_function(SCL_PIN, GPIO_FUNC_I2C);
    gpio_pull_up(SDA_PIN);
    gpio_pull_up(SCL_PIN);
}
//RUN THIS FOR THE FIRST TIME IF YOU HAVE NOT RAN THIS CODE EVER BEFORE ON YOUR FREAKING COMPUTER // EEPROM.
void waitingCalibration(Motor &motor) {
    isCalibrated = false;
    motorStuck = false;
    while (!isCalibrated) {
        if (!sw0.read() || !sw2.read()) {
            motor.calibrate();
            //Because once the button is pressed it will put this bool to false.
        }
    }
}
//Checks the state of the calibration when the power goes from 0->1.
void ifNotCalibrated(const std::shared_ptr<Eeprom> &eeprom, Motor &motor) {
    if (eeprom->singleRead(CALIBRATION, false) == false) {
        std::cout << "Not Calibrated." << std::endl;
        stopCalib = false;
        waitingCalibration(motor);
    } else {
        std::cout << "Calibrated" << std::endl;
        motor.setMinMax();
        motor.loadSavedState();
        isCalibrated = true;
        motorStuck = false;
    }
}

int main() {
    //Making an eeprom class that is shared.
    const auto eeprom = std::make_shared<Eeprom>(I2C_PORT, EEPROM_ADDR);
    Motor motor(eeprom, encoder);
    mqttManager = new MQTTManager("192.168.209.188", 1883); // Initialize MQTT
    initAll();
    std::cout << "POWER ON. INITIAL CONDITION OF THE MACHINE. " << std::endl;
    std::cout << stopMotor << " STOP MOTOR" << std::endl;
    std::cout << motorStuck << " STUCK" << std::endl;
    std::cout << stopCalib << " STOP CALIB" << std::endl;
    std::cout << isCalibrated << " IS CALIBRATED" << std::endl;
    //UNCOMMENT AND RUN THE // waitingCalibration(motor); LINE FIRST
    //IF YOU HAVE NOT RAN THIS CODE EVER BEFORE ON YOUR FREAKING COMPUTER // EEPROM.
    //DO NOT UNCOMMENT SINCE U MIGHT NEED TO REMEMBER FOR DEMO.
    // waitingCalibration(motor);
    ifNotCalibrated(eeprom, motor);
    while (true) {
        if (isCalibrated) {
            // std::cout << "IN CALIB BRANCH " << isCalibrated<<std::endl;
            if (sw1StateChanged) {
                motor.updateMotorState();
                sleep_ms(10);
                sw1StateChanged = false;
            }
            if (motorStuck) {
                // std::cout << "IN MOTOR-STUCK BRANCH" << std::endl;
                eeprom->singleWrite(CALIBRATION, false, false);
                isCalibrated = false;
                stopMotor = true;
                motorStuck = false;
                stopCalib = false;
            }
        }
        if (!isCalibrated) {
            // std::cout << "IN NON CALIB BRANCH" << std::endl;
            motorStuck = false;
            stopMotor = true;
            stopCalib = false;
            waitingCalibration(motor);
        }
    }
}
