//
// Created by theom on 13/03/2025 -> MQTTManager.h
//

#ifndef MQTTMANAGER_H
#define MQTTMANAGER_H
#ifndef USE_MQTT
#define USE_MQTT
#endif


#include "IPStack.h"
#include "Countdown.h"
#include "MQTTClient.h"
#include <stdio.h>
#include <string.h>

class MQTTManager {
public:
    MQTTManager(const char* broker, int port);
    ~MQTTManager();
    MQTT::Client<IPStack, Countdown>* getClient();
    bool connect();
    void disconnect();
    void subscribe(const char* topic);
    void publishMessage(const char* topic, const char* message);
    void publishDoorStatus(const char* status);
    void publishResponse(const char* response);

    static void messageArrived(MQTT::MessageData &md);

private:
    const char* broker;
    int port;
    IPStack* ipstack;
    MQTT::Client<IPStack, Countdown> *client;
    static MQTTManager* instance;
};

#endif // MQTTMANAGER_H