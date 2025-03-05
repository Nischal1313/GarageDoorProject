#include "encoder.h"
#include "pico/stdlib.h"  // Adjust based on your microcontroller/platform
#include <chrono>
#include <complex>
#include <iostream>

extern volatile bool motorError;

// Constructor
Encoder::Encoder(const uint8_t rot_a_pin, const uint8_t rot_b_pin)
    : rot_a_prev(false),
      rot_b_prev(false),
      step_count(0),
      direction(0),
      previous_step_count(0),
      last_movement_time(get_absolute_time()),
      was_motor_running(
          false
      ),
      pin_rot_a(rot_a_pin),
      pin_rot_b(rot_b_pin) {
}

void Encoder::resetStuckDetection() {
    previous_step_count = step_count;
    was_motor_running = false;
    last_movement_time = get_absolute_time();
}

void Encoder::update() {
    // Read current states
    bool rot_a_state = gpio_get(pin_rot_a);
    bool rot_b_state = gpio_get(pin_rot_b);

    // Flag to track if there's a meaningful state change
    bool meaningful_change = false;

    // Determine rotation direction using state change logic
    if (rot_a_state != rot_a_prev || rot_b_state != rot_b_prev) {
        meaningful_change = true;

        if (rot_a_state != rot_a_prev) {
            // Rising edge on A
            if (rot_a_state) {
                if (rot_b_state == 0) {
                    // Clockwise rotation
                    direction = 1;
                    step_count++;
                } else {
                    // Counterclockwise rotation
                    direction = -1;
                    step_count--;
                }
            } else {
                // Falling edge on A
                if (rot_b_state == 1) {
                    // Clockwise rotation
                    direction = 1;
                    step_count++;
                } else {
                    // Counterclockwise rotation
                    direction = -1;
                    step_count--;
                }
            }
        }

        // Store previous states for next iteration
        rot_a_prev = rot_a_state;
        rot_b_prev = rot_b_state;
    }

    // Check for stuck condition
    absolute_time_t current_time = get_absolute_time();

    if (meaningful_change) {
        // Reset tracking when there's movement
        last_meaningful_update = current_time;
        last_tracked_step_count = step_count;
    } else {
        // Check for no movement
        int64_t time_diff_ms = absolute_time_diff_us(last_meaningful_update, current_time) / 1000;

        // If no meaningful change for more than STUCK_TIMEOUT_MS
        if (time_diff_ms > STUCK_TIMEOUT_MS) {
            std::cout << "Encoder appears to be stuck!" << std::endl;
            // Trigger stuck condition
            motorError = true;
        }
    }
}

// Optional: Callback or flag for stuck condition
bool Encoder::encoderStuck() {
    if (motorError) {
        std::cout << "Encoder appears to be stuck!" << std::endl;
        return true;
    }
    return false;
}

bool Encoder::isEncoderStuck(bool isMotorRunning) {
    std::cout << "WE ARE CHECKING IF STUCK" << std::endl;
    // Only check for stuck encoder if motor is explicitly running
    if (!isMotorRunning) {
        // Reset all tracking when motor is not running
        resetStuckDetection();
        return false;
    }
    if (motorError) {
        std::cout << "Encoder appears to be stuck!" << std::endl;
        return true;
    }
    std::cout << "Encoder appears to be moving!" << std::endl;
    return false;
}
//
//     // First time motor starts running
//     if (!was_motor_running) {
//         previous_step_count = step_count;
//         last_movement_time = get_absolute_time();
//         was_motor_running = true;
//         return false;
//     }
//
//     // Check if step count has changed significantly
//     bool encoderMoved = (std::abs(step_count - previous_step_count) > 0);
//
//     if (!encoderMoved) {
//         // Check if we've exceeded the stuck timeout
//         absolute_time_t current_time = get_absolute_time();
//         int64_t time_diff_ms = absolute_time_diff_us(last_movement_time, current_time) / 1000;
//
//         if (time_diff_ms > STUCK_TIMEOUT_MS) {
//             return true; // Encoder is stuck
//         }
//     } else {
//         // Encoder moved, reset tracking
//         previous_step_count = step_count;
//         last_movement_time = get_absolute_time();
//     }
//
//     return false;
// }

// Get current step count
int Encoder::getStepCount() const {
    return step_count;
}

// Get current rotation direction
int Encoder::getDirection() const {
    return direction;
}

// Reset step count and direction
void Encoder::resetStepCount() {
    step_count = 0;
    direction = 0;
}

// Check if encoder has rotated
bool Encoder::hasRotated() const {
    return step_count != 0;
}
