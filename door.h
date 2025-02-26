//
// Created by nischal on 2/26/25.
//

#ifndef DOOR_H
#define DOOR_H
#define LED_ERROR 21

// Define the door state
enum DoorState {
    DOOR_CLOSED,
    DOOR_OPENING,
    DOOR_OPEN,
    DOOR_CLOSING,
    DOOR_STOPPED
};

// Door Class
class Door {
public:
    // Constructor
    Door() = default;

    DoorState doorState = DOOR_CLOSED;
    DoorState previousState = DOOR_CLOSED;
    bool stopMotor = false;  // Flag to stop the motor
    bool sw1StateChanged = false;  // Flag to track button state change
    bool isCalibrated = false;  // Calibration state flag

    // Methods
    static void blink();
    void updateDoorState();
    [[nodiscard]] int returnDoorState() const;

private:
    };

#endif // DOOR_H



