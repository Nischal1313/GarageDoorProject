#include "encoder.h"
#include <iostream>
#include <cstdint>
#include "pico/stdlib.h"
#include <complex>
#include <memory>
#include <hardware/i2c.h>
#include "defines.h"
#include "eeprom.h"

extern volatile bool motorStuck;
extern volatile bool stopMotor;
extern volatile bool stopCalib;
extern volatile bool isCalibrated;
//A shared eeprom between main and this file.
const auto eeprom = std::make_shared<Eeprom>(I2C_PORT, EEPROM_ADDR);

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
//If we are not stuck we know that the time needs to be reevaluated for the stuck time.
void Encoder::resetStuckDetection() {
    previous_step_count = step_count;
    was_motor_running = false;
    last_movement_time = get_absolute_time();
}
//Constant updating of the encoders movement to track if we are stuck or not.
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
            stuckTimes++;
            if (stuckTimes > 100) {
                encoderStuck();
                stuckTimes = 0;
            }
        } else {
            stuckTimes = 0;
        }
    }
}
//Handles the main stuck logic even when not calibrated.
void Encoder::encoderStuck() {
    std::cout << "Successful STUCK branch." << std::endl;
    // Blink LED to indicate state change
    for (int i = 0; i < 2; i++) {
        gpio_put(20, true);
        sleep_ms(100);
        gpio_put(20, false);
        sleep_ms(100);
    }
    stopMotor = true;
    motorStuck = true;
    stopCalib = true;
    isCalibrated = false;
    eeprom->singleWrite(CALIBRATION, false, false);
    stuckTimes = 0;

    std::cout <<"STATE AFTER STUCK:"<< std::endl;
    std::cout << stopMotor <<" STOP MOTOR"<< std::endl;
    std::cout << motorStuck <<" STUCK"<< std::endl;
    std::cout << stopCalib <<" STOP CALIB"<< std::endl;
    std::cout << isCalibrated <<" IS CALIBRATED"<< std::endl;
}
//Can handle the main stuck logic when calibrated.
bool Encoder::isEncoderStuck(const bool isMotorRunning) {
    // Only check for stuck encoder if motor is explicitly running
    if (!isMotorRunning) {
        // Reset all tracking when motor is not running
        resetStuckDetection();
        return false;
    }
    if (stuckTimes > 300 ) {
        stuckTimes = 0;
        return true;
    }

    return false;
}
