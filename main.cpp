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

// Global variables
//We need volatile because we never know when these functions might change based on the interrupts.
volatile bool stopMotor = true; // Flag to stop motor movement. It starts as true since for the first time it will be turned to false.
volatile bool sw1StateChanged = false;
volatile bool motorError = false;
volatile bool isCalibrated  = false;

const auto encoder = std::make_shared<Encoder>(ENCODER_A, ENCODER_B);

void irq_handler(const uint gpio, uint32_t event_mask) {
    if (gpio == 8) {
        stopMotor = !stopMotor;
        sw1StateChanged = true;
    } else if (encoder->isEncoderStuck(stopMotor)) {
        std::cout << "ENCODER STUCK - MOTOR STOPPED" << std::endl;
    }
}

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

//Buttons
GPIOPin sw0(BUTTON_SW0);
GPIOPin sw1(BUTTON_SW1);
GPIOPin sw2(BUTTON_SW2);
//LEDs
GPIOPin LED1(20, false, false, false);
GPIOPin LED2(21, false, false, false);
GPIOPin LED3(22, false, false, false);

void waitingCalibration(Motor &motor) {
    bool buttons_pressed = false;
    while (buttons_pressed == false && !isCalibrated) {
        if (!sw0.read() || !sw2.read()) {
            buttons_pressed = true;
            motor.calibrate();
            isCalibrated = true;
            motorError = false;
        }
    }
}

void ifNotCalibrated(const std::shared_ptr<Eeprom> &eeprom, Motor &motor) {
    if (eeprom->singleRead(CALIBRATION, false) == false) {
        std::cout << "not calibrated" << std::endl;
        std::cout << eeprom->singleRead(STEP_COUNT, true) << std::endl;
        isCalibrated = false;
        motorError = false;
        waitingCalibration(motor);
    } else {
        std::cout << "Calibrated" << std::endl;
        motor.setMinMax();
        motor.setDoorState();
        isCalibrated = true;
    }
}

int main() {
    const auto eeprom = std::make_shared<Eeprom>(I2C_PORT, EEPROM_ADDR);
    Motor motor(eeprom, encoder);
    initAll();
    // encoder->resetStepCount();
    // waitingCalibration(motor);
     while (true) {
        if (isCalibrated) {
            if (sw1StateChanged) {
                motor.updateMotorState();
                sleep_ms(10);
                sw1StateChanged = false;
            } else if (motorError) {
                eeprom->singleWrite(CALIBRATION, false, false);
                isCalibrated = false;
                motorError = false;
            }
        }
         if(!isCalibrated) {
             ifNotCalibrated(eeprom, motor);
             waitingCalibration(motor);
         }
    }
}
