#include <cstdio>
#include <cstring>
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


#define ENCODER_A 27
#define ENCODER_B 28
#define LIMIT_SWITCH_UP 16
#define LIMIT_SWITCH_DOWN 17
#define BUTTON_SW0 9
#define BUTTON_SW1 8
#define BUTTON_SW2 7
#define LED_STATUS 20
#define EEPROM_ADDR 0x50
#define CALIBRATION_NUMBER 0x40
#define MOVING_DOOR 0x80
#define MOVING_UP_AND_DOWN 0x20
#define HALF_STEP_SEQUENCE_LENGTH 8
#define OPTICAL_SENSOR_PIN 28
#define FINE_ADJUSTMENT_STEPS = 128 // Get an actual value for this.
#define EEPROM_ADDR 0x50
#define I2C_PORT i2c0
#define DEBOUNCE_TIME_MS  100  // Debounce time in milliseconds


// Global variables
bool isCalibrated = false;
bool doorMoving = false;
bool doorOpen = false;
bool errorState = false;
bool topLimitSwitchHit = false;
bool bottomLimitSwitchHit = false;

//We need volatile because we never know when these functions might change based on the interrupts.
volatile bool stopMotor; // Flag to stop motor movement
volatile bool sw1StateChanged = false;
volatile int encoderCount = 0;
volatile uint32_t lastInterruptTime = 0;  // Last time the button was pressed

void irq_handler(uint gpio, uint32_t event_mask) {
    if (uint32_t currentTime = to_ms_since_boot(get_absolute_time()); currentTime - lastInterruptTime > DEBOUNCE_TIME_MS) {
        if (isCalibrated) {
            lastInterruptTime = currentTime;  // Update the last interrupt time
            sw1StateChanged = true;
            stopMotor = !stopMotor;
        }
    }
}

void rot_handler(uint gpio, uint32_t event_mask) {
    if (!isCalibrated) {
        if (gpio_get(ENCODER_A) && gpio_get(ENCODER_B)) {
            encoderCount++;
        } else {
            encoderCount--;
        }
    }
};

void initAll() {
    stdio_init_all();

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

    gpio_init(OPTICAL_SENSOR_PIN);
    gpio_set_dir(OPTICAL_SENSOR_PIN, GPIO_IN);
    gpio_pull_up(OPTICAL_SENSOR_PIN);

    // Initialize encoder pins as inputs
    gpio_init(ENCODER_A);
    gpio_set_dir(ENCODER_A, GPIO_IN);
    gpio_pull_up(ENCODER_A);

    gpio_init(ENCODER_B);
    gpio_set_dir(ENCODER_B, GPIO_IN);
    gpio_pull_up(ENCODER_B);

    gpio_set_irq_enabled_with_callback(BUTTON_SW1, GPIO_IRQ_EDGE_FALL, true, irq_handler);
    // gpio_set_irq_enabled_with_callback(ENCODER_A, GPIO_IRQ_EDGE_RISE, true, rot_handler);
}


GPIOPin sw0(BUTTON_SW0);
GPIOPin sw1(BUTTON_SW1);
GPIOPin sw2(BUTTON_SW2);
// Configure LEDs for status indication (GP20 - GP22)
GPIOPin LED1(20, false, false, false);
GPIOPin LED2(21, false, false, false);
GPIOPin LED3(22, false, false, false);

Motor motor;
Eeprom eeprom(I2C_PORT, EEPROM_ADDR);

void waitingButtonPress() {
    bool buttons_pressed = false;
    while (buttons_pressed == false && !isCalibrated) {
        if (!sw0.read() || !sw2.read()) {
            buttons_pressed = true;
            motor.calibrate();
            isCalibrated = true;
        }
    }
};


int main() {
    initAll();
    waitingButtonPress();
    while (isCalibrated) {
        if (sw1StateChanged) {
            motor.updateMotorState();
            sleep_ms(10);
            sw1StateChanged = false;
        }
    }
}
