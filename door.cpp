//
// Created by nischal on 2/26/25.
//

#include "door.h"
#include "mqtt.h"
#include <pico/time.h>
#include "hardware/gpio.h"
#include "mqtt.h"

extern MQTTManager *mqttManager;  // External global MQTTManager instance



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
           mqttManager->publish("garage/door/status", "{ \"state\": \"opening\" }"); // Status update
            stopMotor = false;
            break;

            case DOOR_OPEN:
                doorState = DOOR_CLOSING;
           mqttManager->publish("garage/door/status", "{ \"state\": \"closing\" }"); // Status update
            stopMotor = false;
            break;

            case DOOR_CLOSING:
                doorState = DOOR_STOPPED;
             mqttManager->publish("garage/door/status", "{ \"state\": \"stopped\" }");  // Status Update
                mqttManager->publish("garage/door/response", "{ \"response\": \"command completed\", \"action\": \"gate stopped\" }") // Response when stopping
            break;

            case DOOR_STOPPED:
                if (previousState == DOOR_OPENING) {
                    doorState = DOOR_CLOSING;
                    mqttManager->publish("garage/door/status", "{ \"state\": \"closing\" }");
                    stopMotor = false;
                } else if (previousState == DOOR_CLOSING) {
                    doorState = DOOR_OPENING;
                    mqttManager->publish("garage/door/status", "{ \"state\": \"opening\" }");
                    stopMotor = false;
                }
            break;

            default:
                break;
        }

        // Publish a response when the action is **fully completed**
        if (doorState == DOOR_CLOSED) {
            mqttManager->publish("garage/door/response", "{ \"response\": \"command completed\", \"action\": \"gate closed\" }");
        } else if (doorState == DOOR_OPEN) {
             mqttManager->publish("garage/door/response", "{ \"response\": \"command completed\", \"action\": \"gate opened\" }");
        }

        if (doorState == DOOR_OPENING || doorState == DOOR_CLOSING) {
            previousState = doorState;
        }
    }
}
