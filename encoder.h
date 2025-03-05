#ifndef ENCODER_H
#define ENCODER_H

#include <cstdint>
#include <pico/types.h>

class Encoder {
private:
    // Encoder state tracking variables
    bool rot_a_prev;
    bool rot_b_prev;
    int step_count;
    int direction;
    // Existing previous variables
    int previous_step_count;
    unsigned long last_movement_time;
    const unsigned int STUCK_TIMEOUT_MS = 1000; // 1 second timeout
    bool was_motor_running;
    // Pins for encoder
    uint8_t pin_rot_a;
    uint8_t pin_rot_b;
    absolute_time_t last_meaningful_update;
    int last_tracked_step_count;


public:
    // Constructor
    Encoder(uint8_t rot_a_pin, uint8_t rot_b_pin);

    void resetStuckDetection();

    // Update encoder state
    void update();

    bool encoderStuck();

    bool isEncoderStuck(bool isMotorRunning);

    // Getter methods
    [[nodiscard]] int getStepCount() const;
    [[nodiscard]] int getDirection() const;

    // Reset step count
    void resetStepCount();

    // Check for rotation
    bool hasRotated() const;
};

#endif // ENCODER_H