#include "mqtt.h"
#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/timer.h"
#include "IPStack.h"
#include "MQTTClient.h"

#define MQTT_BROKER "192.168.209.188"
#define MQTT_PORT 1883

// Global instance
MQTTManager *mqttManager = nullptr;

// Constructor
MQTTManager::MQTTManager(const char *broker, int port) : hostname(broker), port(port) {
    ipstack = new IPStack("TGalaxy S20 FE", "poliuyt12"); // Network credentials
    client = new MQTT::Client<IPStack, Countdown>(*ipstack);
}

// Destructor
MQTTManager::~MQTTManager() {
    delete client;
    delete ipstack;
}

// Connect to MQTT broker
bool MQTTManager::connect() {
    int rc = ipstack->connect(hostname, port);
    if (rc != 1) {
        printf("Failed to connect to broker, rc=%d\n", rc);
        return false;
    }

    printf("MQTT connecting\n");
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = (char *)"PicoW-sample";

    rc = client->connect(data);
    if (rc != 0) {
        printf("MQTT connection failed, rc=%d\n", rc);
        return false;
    }

    printf("MQTT connected!\n");
    return true;
}

// Subscribe to a topic
void MQTTManager::subscribe(const char *topic) {
    int rc = client->subscribe(topic, MQTT::QOS2, messageArrived);
    if (rc != 0) {
        printf("Failed to subscribe to topic: %s, rc=%d\n", topic, rc);
    } else {
        printf("Subscribed to topic: %s\n", topic);
    }
}

// Publish a message
void MQTTManager::publish(const char *topic, const char *message) {
    MQTT::Message mqttMessage;
    mqttMessage.qos = MQTT::QOS1;
    mqttMessage.retained = false;
    mqttMessage.payload = (void *)message;
    mqttMessage.payloadlen = strlen(message);

    int rc = client->publish(topic, mqttMessage);
    if (rc != 0) {
        printf("MQTT publish failed for topic: %s, rc=%d\n", topic, rc);
    }
}

// Handle incoming MQTT messages
void MQTTManager::messageArrived(MQTT::MessageData &md) {
    MQTT::Message &message = md.message;
    char payload[message.payloadlen + 1];
    memcpy(payload, message.payload, message.payloadlen);
    payload[message.payloadlen] = '\0';

    printf("Received MQTT Command: %s\n", payload);

    if (strcmp(payload, "open") == 0) {
        printf("MQTT Command: OPENING DOOR\n");
        mqttManager->publish("garage/door/response", "{ \"response\": \"command received\", \"action\": \"open\" }");
        sw1StateChanged = true;
        stopMotor = false;
    } else if (strcmp(payload, "close") == 0) {
        printf("MQTT Command: CLOSING DOOR\n");
        mqttManager->publish("garage/door/response", "{ \"response\": \"command received\", \"action\": \"close\" }");
        sw1StateChanged = true;
        stopMotor = false;
    } else if (strcmp(payload, "stop") == 0) {
        printf("MQTT Command: STOPPING DOOR\n");
        mqttManager->publish("garage/door/response", "{ \"response\": \"command received\", \"action\": \"stop\" }");
        stopMotor = true;
    } else if (strcmp(payload, "calibrate") == 0) {
        printf("MQTT Command: CALIBRATING DOOR\n");
        mqttManager->publish("garage/door/response", "{ \"response\": \"command received\", \"action\": \"calibrate\" }");
        isCalibrated = false;
        waitingCalibration();
    } else {
        printf("Unknown Command: %s\n", payload);
    }
}

// Main function
int main() {
    stdio_init_all();

    mqttManager = new MQTTManager(MQTT_BROKER, MQTT_PORT);
    if (!mqttManager->connect()) {
        while (true) {
            printf("MQTT connection failed, retrying...\n");
            sleep_ms(5000);
            mqttManager->connect();
        }
    }

    mqttManager->subscribe("garage/door/command");

    while (true) {
        if (!mqttManager->connect()) {
            printf("MQTT disconnected, reconnecting...\n");
            mqttManager->connect();
        }

        sleep_ms(100);
    }
}