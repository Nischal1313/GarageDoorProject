//
// Created by nischal on 2/26/25.
//

#include "door.h"
#include "mqtt.h"
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
                // If the door is closed, pressing SW1 will start opening it
                doorState = DOOR_OPENING;
                publishDoorStatus("{ \"state\": \"opening\", \"source\": \"button_press\" }"); // 🔹 Indicate SW1 was pressed
                stopMotor = false;  // Clear the stop flag
                break;

            case DOOR_OPEN:
                // If the door is open, pressing SW1 will start closing it
                doorState = DOOR_CLOSING;
                publishDoorStatus("{ \"state\": \"closing\", \"source\": \"button_press\" }"); // 🔹 Indicate SW1 was pressed
                stopMotor = false;
                break;

            case DOOR_CLOSING:
                // If the door is currently closing, pressing SW1 will stop it
                doorState = DOOR_STOPPED;
                publishDoorStatus("{ \"state\": \"stopped\", \"source\": \"button_press\" }"); // 🔹 Indicate SW1 stopped movement
                break;

            case DOOR_STOPPED:
                // If the door was stopped mid-movement, pressing SW1 resumes movement in the opposite direction
                if (previousState == DOOR_OPENING) {
                    // If the door was opening before stopping, resume closing
                    doorState = DOOR_CLOSING;
                    publishDoorStatus("{ \"state\": \"closing\", \"source\": \"button_press\" }");
                    stopMotor = false;
                } else if (previousState == DOOR_CLOSING) {
                    // If the door was closing before stopping, resume opening
                    doorState = DOOR_OPENING;
                    publishDoorStatus("{ \"state\": \"opening\", \"source\": \"button_press\" }");
                    stopMotor = false;
                }
                break;

            default:
                // No changes if no valid state is found
                break;
        }

        // Save the previous movement state for use when restarting from DOOR_STOPPED
        if (doorState == DOOR_OPENING || doorState == DOOR_CLOSING) {
            previousState = doorState;
        }
    }
}
