#include <cstdio>
#include <cstring>
#include <cmath>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/timer.h"
#include "PicoUart.h"
#include "hardware/gpio.h"
#include <hardware/i2c.h>
//#include "IPStack.h"
#include "IPStack.h"
#include "Countdown.h"
//#include "paho.mqtt.embedded-c/MQTTClient-C/src/MQTTClient.h"
// #include "garageclasses.h"


// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#if 0
#define UART_NR 0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#else
#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#endif

#define BAUD_RATE 9600
#define STOP_BITS 1 // for simulator
//#define STOP_BITS 2 // for real system

#define USE_MODBUS
#define USE_MQTT


static const uint8_t raspberry26x32[] =
{
    0x0, 0x0, 0xe, 0x7e, 0xfe, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xfe, 0xfe, 0xfc, 0xf8, 0xfc, 0xfe,
    0xfe, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfe, 0x7e,
    0x1e, 0x0, 0x0, 0x0, 0x80, 0xe0, 0xf8, 0xfd,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xfd,
    0xf8, 0xe0, 0x80, 0x0, 0x0, 0x1e, 0x7f, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff,
    0xff, 0xff, 0xff, 0xff, 0x7f, 0x1e, 0x0, 0x0,
    0x0, 0x3, 0x7, 0xf, 0x1f, 0x1f, 0x3f, 0x3f,
    0x7f, 0xff, 0xff, 0xff, 0xff, 0x7f, 0x7f, 0x3f,
    0x3f, 0x1f, 0x1f, 0xf, 0x7, 0x3, 0x0, 0x0
};

static const char *topic = "test-topic";

// Pin assignments
#define IN1 2
#define IN2 3
#define IN3 6
#define IN4 13
#define ENCODER_A 27
#define ENCODER_B 28
#define LIMIT_SWITCH_UP 16
#define LIMIT_SWITCH_DOWN 17
#define BUTTON_SW0 9
#define BUTTON_SW1 8
#define BUTTON_SW2 7
#define LED_STATUS 20
#define LED_ERROR 21
#define EEPROM_ADDR 0x50
#define CALIBRATION_NUMBER 0x40
#define MOVING_DOOR 0x80
#define MOVING_UP_AND_DOWN 0x20
#define HALF_STEP_SEQUENCE_LENGTH 8
#define OPTICAL_SENSOR_PIN 28
#define FINE_ADJUSTMENT_STEPS = 128 // Get an actual value for this.
#define EEPROM_ADDR 0x50
#define I2C_PORT i2c0


// Global variables
bool isCalibrated = false;
bool doorMoving = false;
bool doorOpen = false;
bool errorState = false;
int stepCount = 0; // Stores steps between open and close
bool topLimitSwitchHit = false;
bool bottomLimitSwitchHit = false;
//We need volatile because we never know when these functions might change based on the interrupts.
volatile bool sw1Pressed = false;
volatile bool sw1StateChanged = false;

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

// Define the door state
enum DoorState {
    DOOR_CLOSED,
    DOOR_OPENING,
    DOOR_OPEN,
    DOOR_CLOSING,
    DOOR_STOPPED
};

extern DoorState doorState; // Declare it as an external variable


void irq_handler(uint gpio, uint32_t event_mask) {
    if (isCalibrated) {
        sw1Pressed = !sw1Pressed;
        sw1StateChanged = true;
    }
}

void initAll() {
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
}

void set_motor_pins(const uint8_t *step) {
    gpio_put(IN1, step[0]);
    gpio_put(IN2, step[1]);
    gpio_put(IN3, step[2]);
    gpio_put(IN4, step[3]);
}


class Eeprom {
private:
    i2c_inst_t *i2cPort; // I2C port instance
    uint8_t eepromAddr; // EEPROM device address

public:
    // Constructor
    Eeprom(i2c_inst_t *i2cPort, uint8_t eepromAddr)
        : i2cPort(i2cPort), eepromAddr(eepromAddr) {
    }

    // Read a single byte from EEPROM
    int singleRead(const int addr) {
        const uint8_t addrToWrite[2] = {
            static_cast<uint8_t>((addr >> 8) & 0xFF),
            static_cast<uint8_t>(addr & 0xFF)
        };

        uint8_t returnedData[1];

        i2c_write_blocking(i2cPort, eepromAddr, addrToWrite, 2, true);
        sleep_ms(10); // Necessary delay for EEPROM read operation
        int result = i2c_read_blocking(i2cPort, eepromAddr, returnedData, 1, false);

        if (result > 0) {
            printf("EEPROM Read: Addr 0x%X -> Value: 0x%X\n", addr, returnedData[0]);
            return returnedData[0];
        } else {
            printf("EEPROM Read Failed\n");
            return -1;
        }
    }

    // Write a single byte dynamically (only writes if data is different)
    void singleWrite(const int addr, const int data) {
        int existingData = singleRead(addr);
        if (existingData == data) {
            printf("EEPROM Write Skipped (Same Data at Addr 0x%X)\n", addr);
            return; // Skip writing if the value is already the same
        }

        const uint8_t dataToWrite[3] = {
            static_cast<uint8_t>((addr >> 8) & 0xFF),
            static_cast<uint8_t>(addr & 0xFF),
            static_cast<uint8_t>(data)
        };

        if (i2c_write_blocking(i2cPort, eepromAddr, dataToWrite, 3, false) > 0) {
            printf("EEPROM Write: Addr 0x%X -> New Value: 0x%X\n", addr, data);
        } else {
            printf("EEPROM Write Failed\n");
        }
    }
};

class Door {
private:
    const int stepperMotorPin;
    const int limitSwitchPin;
    const int secondLimitSwitchDown;
    const int ledPin;

public:
    // Constructor
    Door(const int stepperMotorPin, const int limitSwitchPin, const int secondLimitSwitchDown,
         const int ledPin)
        : stepperMotorPin(stepperMotorPin),
          limitSwitchPin(limitSwitchPin),
          secondLimitSwitchDown(secondLimitSwitchDown),
          ledPin(ledPin){
    }

    void blink() {
        while (true) {
            gpio_put(ledPin, true);
            sleep_us(500);
            gpio_put(ledPin, false);
            sleep_us(500);
        }
    }

    static bool ifDoorNotStuck() {
        //Assuming that the second if statement will be untrue if the thing is not turning.
        const int stateA = gpio_get(ENCODER_A);
        const int stateB = gpio_get(ENCODER_B);
        if (stateA == stateB) {
            doorState = DOOR_OPENING; // Clockwise (CW) → Opening
            return false;
        }
        if (stateA == 1 && stateB == 0) {
            doorState = DOOR_CLOSING; // Counterclockwise (CCW) → Closing
            return false;
        }
        return true;
    }

    void updateDoorState() {
        //Static is needed since many function use the same key word when updating the state.
        static DoorState previousState = DOOR_CLOSED;

        if (!ifDoorNotStuck()) {
            blink();
        }
        if (sw1Pressed && isCalibrated) {
            sw1Pressed = false; // Reset flag
            switch (doorState) {
                case DOOR_CLOSED:
                    if (ifDoorNotStuck()) {
                        doorState = DOOR_OPENING;
                        moveUntilTop();
                    }
                    break;

                case DOOR_OPENING:
                    doorState = DOOR_STOPPED;
                    break;

                case DOOR_OPEN:
                    if (ifDoorNotStuck()) {
                        doorState = DOOR_CLOSING;
                        moveUntilBottom();
                    }
                    break;

                case DOOR_CLOSING:
                    doorState = DOOR_STOPPED;
                    break;

                case DOOR_STOPPED:
                    doorState = (previousState == DOOR_OPENING) ? DOOR_CLOSING : DOOR_OPENING;
                    break;
            }
            previousState = doorState;
        }
    }

    static void moveMotorUp(int step) {
        set_motor_pins(half_step_sequence[step]);
        sleep_us(500);
    }

    static void moveMotorDown(int steps) {
        for (int i = 0; i < steps; i++) {
            int step = (HALF_STEP_SEQUENCE_LENGTH - (i % HALF_STEP_SEQUENCE_LENGTH)) % HALF_STEP_SEQUENCE_LENGTH;
            set_motor_pins(half_step_sequence[step]);
            sleep_us(1200);
        }
    }

    void moveUntilBottom() {
        int step = 0;
        while (!gpio_get(secondLimitSwitchDown)) {
            moveMotorDown(step);
            step = (step + 1) % HALF_STEP_SEQUENCE_LENGTH;
        }
        doorState = DOOR_CLOSED;
    }

    void moveUntilTop() {
        int step = 0;
        while (gpio_get(limitSwitchPin)) {
            moveMotorUp(step);
            step = (step + 1) % HALF_STEP_SEQUENCE_LENGTH;
        }
        doorState = DOOR_OPEN;
    }
};


class GPIOPin {

private:
    int pin; // GPIO pin number
    bool invert; // Whether the pin state is inverted
    bool input; // Whether the pin is an input or output

public:
    // Constructor: Initializes the GPIO pin, can be configured for input, pull-up, and inversion
    explicit GPIOPin(const int pin, const bool input = true, bool pullup = true, const bool invert = true)
        : pin(pin), invert(invert), input(input) {
        gpio_init(pin);

        if (input) {
            gpio_set_dir(pin, GPIO_IN);
            if (pullup) {
                gpio_pull_up(pin); // Enable pull-up resistor
            }
            if (invert) {
                gpio_set_inover(pin, GPIO_OVERRIDE_INVERT); // Invert input pin
            }
        } else {
            gpio_set_dir(pin, GPIO_OUT);
            gpio_put(pin, false);
            if (invert) {
                gpio_set_outover(pin, GPIO_OVERRIDE_INVERT); // Invert output pin
            }
        }
    }

    // Delete the copy constructor to make the object non-copyable
    GPIOPin(const GPIOPin &) = delete;

    // Read the pin state. Returns the inverted value if required
    [[nodiscard]] bool read() const {
        return gpio_get(pin);
    }

    // Write a value to the pin (only for output pins)
    void write(const bool value) const {
        if (!input) {
            gpio_put(pin, invert ? !value : value); // Write inverted value if needed
        }
    }

    // Overload operator() to read the state of the pin
    bool operator()() const { return read(); }

    // Overload operator() to write a value to the pin
    void operator()(bool value) const { write(value); }

    // Conversion operator to return the pin-number
    explicit operator int() const { return pin; }
};

// I have these three functions. I need you to tell me how to put them into a seprate file named classes.cpp and classes.h and how to use them in main.


DoorState doorState = DOOR_OPEN;

Eeprom crowtailEeprom(I2C_PORT, EEPROM_ADDR);

void moveMotorUp(int steps) {
    for (int i = 0; i < steps; i++) {
        int step = (i % HALF_STEP_SEQUENCE_LENGTH); // Normal stepping order
        set_motor_pins(half_step_sequence[step]);
        sleep_us(1000);
    }
}

void moveMotorDown(int steps) {
    for (int i = 0; i < steps; i++) {
        int step = (HALF_STEP_SEQUENCE_LENGTH - (i % HALF_STEP_SEQUENCE_LENGTH)) % HALF_STEP_SEQUENCE_LENGTH;
        set_motor_pins(half_step_sequence[step]);
        sleep_us(1000);
    }
}

void moveUntilBottom() {
    while (gpio_get(LIMIT_SWITCH_DOWN)) {
        moveMotorDown(8);
    }
}

void moveUntilTop() {
    while (gpio_get(LIMIT_SWITCH_UP)) {
        moveMotorUp(8);
    }
}

void fine_adjustment(int step, const int adjustment) {
    for (int i = 0; i < adjustment; i++) {
        moveMotorUp(step);
        step = (step + 1) % HALF_STEP_SEQUENCE_LENGTH;
    }
}

void find_falling_edge_for_calibration() {
    static int step = 0;
    bool last_state = false;
    bool current_state = false;
    while (true) {
        moveMotorUp(step);
        step = (step + 1) % HALF_STEP_SEQUENCE_LENGTH;
        last_state = current_state;
        current_state = gpio_get(OPTICAL_SENSOR_PIN);
        if (last_state && !current_state) {
            printf("Calibrating starting.\n");
            sleep_ms(1500);
            //Sleeping helps with getting a more accurate number
            break;
        }
    }
}

void calibrate() {
    if (!isCalibrated && !gpio_get(LIMIT_SWITCH_UP)) {
        moveUntilTop();
    }
    if (!isCalibrated) {
        int step = 0;
        // Now move down and count steps
        stepCount = 0; // Reset counter before counting
        while (gpio_get(LIMIT_SWITCH_DOWN)) {
            // While bottom switch is NOT pressed
            moveMotorDown(step);
            step = (step + 1) % HALF_STEP_SEQUENCE_LENGTH;
            stepCount++;
        }
        doorState = DOOR_CLOSED;
        isCalibrated = true;
        // crowtailEeprom.singleWrite(isCalibrated);
    }
}


GPIOPin sw0(BUTTON_SW0);
GPIOPin sw1(BUTTON_SW1);
GPIOPin sw2(BUTTON_SW2);
// Configure LEDs for status indication (GP20 - GP22)
GPIOPin LED1(20, false, false, false);
GPIOPin LED2(21, false, false, false);
GPIOPin LED3(22, false, false, false);

void waitingButtonPress() {
    bool buttons_pressed = false;
    while (buttons_pressed == false) {
        if (!sw0.read() || !sw2.read()) {
            buttons_pressed = true;
            calibrate();
        }
    }
};

int main() {
    // Door garageDoor(16, 17);
    stdio_init_all();
    initAll();
    // moveUntilBottom();
    waitingButtonPress();
    return 0;
};
