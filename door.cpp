//
// Created by nischal on 2/26/25.
//

#include "door.h"
#include <pico/time.h>

#include "hardware/gpio.h"


// Blink function to indicate an error state using an LED
void Door::blink() {
    for (int i = 0; i < 2; i++) {
        gpio_put(LED_ERROR, true);
        sleep_ms(200);
        gpio_put(LED_ERROR, false);
        sleep_ms(200);
    }
}

int Door:: returnDoorState() const{
  return doorState;
}

// Update the door state based on button press or previous movement
void Door::updateDoorState() {
    if (sw1StateChanged && isCalibrated) {
        sw1StateChanged = false;  // Reset the flag first

        switch (doorState) {
            case DOOR_CLOSED:
                doorState = DOOR_OPENING;
                stopMotor = false;  // Clear the stop flag
                break;

            case DOOR_OPEN:
                doorState = DOOR_CLOSING;
                stopMotor = false;
                break;

            case DOOR_CLOSING:
                doorState = DOOR_STOPPED;
                break;

            case DOOR_STOPPED:
                if (previousState == DOOR_OPENING) {
                    doorState = DOOR_CLOSING;
                    stopMotor = false;
                } else if (previousState == DOOR_CLOSING) {
                    doorState = DOOR_OPENING;
                    stopMotor = false;
                }
                break;
            default: ;
        }

        // Save the previous movement state for use when restarting from DOOR_STOPPED
        if (doorState == DOOR_OPENING || doorState == DOOR_CLOSING) {
            previousState = doorState;
        }
    }
}
