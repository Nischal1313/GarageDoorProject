//
// Created by theom on 13/03/2025 -> mqttmanager.cpp
//

#include "MQTTManager.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/timer.h"
#include "IPStack.h"
#include "MQTTClient.h"

// Define Topics
const char *hostname = "192.168.209.188"; // Broker IP
const char *command_topic = "garage/door/command";
const char *status_topic = "garage/door/status";
const char *response_topic = "garage/door/response";
extern volatile bool sw1StateChanged;
extern volatile bool stopMotor;
extern volatile bool isCalibrated;
extern volatile bool stopCalib;

MQTTManager* MQTTManager::instance = nullptr;

// Constructor
MQTTManager::MQTTManager(const char* broker, int port) : broker(broker), port(port) {
    ipstack = new IPStack("TGalaxy S20 FE", "poliuyt12"); // Wi-Fi credentials
    client = new MQTT::Client<IPStack, Countdown>(*ipstack);
    if (!ipstack || !client) {
        printf("ERROR: Memory allocation failed!\n");
        while (1) {} // Halt execution if allocation fails
    }

    instance = this;

}

// Destructor
MQTTManager::~MQTTManager() {
    delete client;
    delete ipstack;
}

// Connect to MQTT broker
/* bool MQTTManager::connect() {
    printf("Connecting to MQTT broker at %s:%d...\n", broker, port);

    int rc = ipstack->connect(broker, port);
    if (rc != 1) {
        printf("TCP Connection failed (rc=%d). Retrying...\n", rc);
        return false;
    }

    printf("TCP Connected. Attempting MQTT handshake...\n");

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = (char *)"PicoW-sample";

    rc = client->connect(data);
    if (rc != 0) {
        printf("MQTT handshake failed (rc=%d). Retrying...\n", rc);
        ipstack->disconnect();
        return false;
    }

    printf("MQTT Connected!\n");
    return true;

    // ✅ Subscribe when connection is established
    subscribe("garage/door/command");

}
*/

bool MQTTManager::connect() {
#ifdef USE_MQTT
    printf("Connecting to MQTT broker at %s:%d...\n", hostname, "1883");

    int rc = ipstack->connect(hostname, port);
    if (rc != 1) {
        printf("rc from TCP is %d)", rc);
        //return false;
    }

    printf("TCP Connected. Attempting MQTT handshake...\n");

    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = (char *)"PicoW-sample";

    rc = client->connect(data);
    if (rc != 0) {
        printf("rc from MQTT connect is %d\n", rc);
        while (true) {
            tight_loop_contents();
        }
        //return false;
    }

    printf("MQTT Connected!\n");

    // Subscribe to "garage/door/command"
    rc = client->subscribe("garage/door/command", MQTT::QOS2, messageArrived);
    if (rc != 0) {
        printf("MQTT subscription failed (rc=%d)\n", rc);
    } else {
        printf("MQTT subscribed to garage/door/command\n");
    }
#endif
    return true;
}

MQTT::Client<IPStack, Countdown>* MQTTManager::getClient() {
    return client;
}


// Disconnect from MQTT broker
void MQTTManager::disconnect() {
    printf("Closing MQTT connection...\n");

    if (client->isConnected()) {
        client->disconnect();
    }

    ipstack->disconnect();
}

// Subscribe to a topic
void MQTTManager::subscribe(const char* topic) {
    int rc = client->subscribe(topic, MQTT::QOS2, messageArrived);
    if (rc != 0) {
        printf("Failed to subscribe to topic: %s, rc=%d\n", topic, rc);
    } else {
        printf("Subscribed to topic: %s\n", topic);
    }
}

// Publish a message
void MQTTManager::publishMessage(const char* topic, const char* message) {
    MQTT::Message mqttMessage;
    mqttMessage.qos = MQTT::QOS1;
    mqttMessage.retained = false;
    mqttMessage.payload = (void*)message;
    mqttMessage.payloadlen = strlen(message) + 1; // Include null terminator

    int rc = client->publish(topic, mqttMessage);

    if (rc != 0) {
        printf("MQTT publish failed, rc=%d\n", rc);
    } else {
        printf("Published to %s: %s\n", topic, message);
    }
}

// Publish door status updates
void MQTTManager::publishDoorStatus(const char* status) {

    MQTTManager::publishMessage(status_topic, status);
}

// Publish door response updates
void MQTTManager::publishResponse(const char* response) {
    MQTTManager::publishMessage(response_topic, response);
}


// Handle incoming MQTT messages
void MQTTManager::messageArrived(MQTT::MessageData &md) {
    MQTT::Message &message = md.message;
    char payload[message.payloadlen + 1];
    memcpy(payload, message.payload, message.payloadlen);
    payload[message.payloadlen] = '\0';  // Null-terminate

    printf("Received MQTT Command: %s\n", payload);

    if (strcmp(payload, "open") == 0) {
        printf("MQTT Command: OPENING DOOR\n");
        instance->publishMessage("garage/door/response", "{ \"response\": \"command received\", \"action\": \"open\" }");
        sw1StateChanged = true;
        stopMotor = false;
    } else if (strcmp(payload, "close") == 0) {
        printf("MQTT Command: CLOSING DOOR\n");
        instance->publishMessage("garage/door/response", "{ \"response\": \"command received\", \"action\": \"close\" }");
        sw1StateChanged = true;
        stopMotor = false;
    } else if (strcmp(payload, "stop") == 0) {
        printf("MQTT Command: STOPPING DOOR\n");
        instance->publishMessage("garage/door/response", "{ \"response\": \"command received\", \"action\": \"stop\" }");
        stopMotor = true;
    } else if (strcmp(payload, "calibrate") == 0) {
        printf("MQTT Command: CALIBRATING DOOR\n");
        instance->publishMessage("garage/door/response", "{ \"response\": \"command received\", \"action\": \"calibrate\" }");
        isCalibrated = false;
    } else {
        printf("Unknown Command: %s\n", payload);
    }
}