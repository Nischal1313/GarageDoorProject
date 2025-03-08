//
// Created by theom on 06/03/2025.
//

#ifndef MQTT_MANAGER_H
#define MQTT_MANAGER_H

#include "MQTTClient.h"
#include "IPStack.h"
#include "Countdown.h"

class MQTTManager {
private:
    MQTT::Client<IPStack, Countdown> *client;
    IPStack *ipstack;
    const char *hostname;
    int port;

public:
    // Constructor
    MQTTManager(const char *broker, int port);

    // Destructor
    ~MQTTManager();

    // Initialize MQTT connection
    bool connect();

    // Subscribe to topics
    void subscribe(const char *topic);

    // Publish a message
    void publish(const char *topic, const char *message);

    // Callback for handling incoming messages
    static void messageArrived(MQTT::MessageData &md);
};

#endif // MQTT_MANAGER_H
