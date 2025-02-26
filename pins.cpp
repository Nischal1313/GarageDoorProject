//
// Created by nischal on 2/26/25.
//

#include "pins.h"
#include "hardware/gpio.h"

// Constructor: Initializes the GPIO pin, can be configured for input, pull-up, and inversion
GPIOPin::GPIOPin(const int pin, const bool input, bool pullup, const bool invert)
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

// Read the pin state. Returns the inverted value if required
bool GPIOPin::read() const {
    return gpio_get(pin);
}

// Write a value to the pin (only for output pins)
void GPIOPin::write(const bool value) const {
    if (!input) {
        gpio_put(pin, invert ? !value : value); // Write inverted value if needed
    }
}

// Overload operator() to read the state of the pin
bool GPIOPin::operator()() const {
    return read();
}

// Overload operator() to write a value to the pin
void GPIOPin::operator()(const bool value) const {
    write(value);
}

// Conversion operator to return the pin-number
GPIOPin::operator int() const {
    return pin;
}
