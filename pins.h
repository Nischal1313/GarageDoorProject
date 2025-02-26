//
// Created by nischal on 2/26/25.
//

#ifndef pins
#define pins


class GPIOPin {
private:
    int pin;       // GPIO pin number
    bool invert;   // Whether the pin state is inverted
    bool input;    // Whether the pin is an input or output

public:
    // Constructor: Initializes the GPIO pin, can be configured for input, pull-up, and inversion
    explicit GPIOPin(int pin, bool input = true, bool pullup = true, bool invert = true);

    // Delete the copy constructor to make the object non-copyable
    GPIOPin(const GPIOPin &) = delete;

    // Read the pin state. Returns the inverted value if required
    [[nodiscard]] bool read() const;

    // Write a value to the pin (only for output pins)
    void write(bool value) const;

    // Overload operator() to read the state of the pin
    bool operator()() const;

    // Overload operator() to write a value to the pin
    void operator()(bool value) const;

    // Conversion operator to return the pin-number
    explicit operator int() const;
};


#endif //pins
