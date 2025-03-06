#include <stdio.h>
#include <string.h>
#include <cmath>
#include "pico/stdlib.h"
#include "pico/time.h"
#include "hardware/timer.h"
#include "uart/PicoUart.h"

#include "IPStack.h"
#include "Countdown.h"
#include "MQTTClient.h"

// We are using pins 0 and 1, but see the GPIO function select table in the
// datasheet for information on which other pins can be used.
#if 0
#define UART_NR 0
#define UART_TX_PIN 0
#define UART_RX_PIN 1
#else
#define UART_NR 1
#define UART_TX_PIN 4
#define UART_RX_PIN 5
#endif

#define BAUD_RATE 9600
#define STOP_BITS 1 // for simulator
//#define STOP_BITS 2 // for real system

#define USE_MQTT
#define USE_7789

void publishDoorStatus(const char* status) {
    const char* topic = "garage/door/status";
    MQTT::Message message;
    message.qos = MQTT::QOS1;
    message.retained = false;
    message.payload = (void*)status;
    message.payloadlen = strlen(status);

    int rc = client.publish(topic, message);
    if (rc != 0) {
        printf("MQTT publish failed, rc=%d\n", rc);
    }
}

void messageArrived(MQTT::MessageData &md) {
    MQTT::Message &message = md.message;

    char payload[message.payloadlen + 1];
    memcpy(payload, message.payload, message.payloadlen);
    payload[message.payloadlen] = '\0';  // Null-terminate

    printf("Received MQTT Command: %s\n", payload);

    if (strcmp(payload, "led_on") == 0) {
        printf("Turning LED ON\n");
        gpio_put(22, true);
    }
    else if (strcmp(payload, "led_off") == 0) {
        printf("Turning LED OFF\n");
        gpio_put(22, false);
    }
    else if (strcmp(payload, "blink") == 0) {
        printf("Blinking LED 3 times\n");
        for (int i = 0; i < 3; i++) {
            gpio_put(22, true);
            sleep_ms(500);
            gpio_put(22, false);
            sleep_ms(500);
        }
    }
    else {
        printf("Unknown Command: %s\n", payload);
    }
}

const char * hostname = "192.168.209.188"; // Broker IP

int main() {

    const uint led_pin = 22;
    const uint button = 9;

    // Initialize LED pin
    gpio_init(led_pin);
    gpio_set_dir(led_pin, GPIO_OUT);

    gpio_init(button);
    gpio_set_dir(button, GPIO_IN);
    gpio_pull_up(button);

    // Initialize chosen serial port
    stdio_init_all();

    printf("\nBoot\n");

#ifdef USE_MQTT
    const char *topic = "test-topic";
    IPStack ipstack("TGalaxy S20 FE", "poliuyt12");
    auto client = MQTT::Client<IPStack, Countdown>(ipstack);

    int rc = ipstack.connect(hostname, 1883);
    if (rc != 1) {
        printf("rc from TCP connect is %d\n", rc);
    }

    printf("MQTT connecting\n");
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    data.MQTTVersion = 3;
    data.clientID.cstring = (char *) "PicoW-sample";

    rc = client.connect(data);
    if (rc != 0) {
        printf("rc from MQTT connect is %d\n", rc);
        while (true) {
            tight_loop_contents();
        }
    }
    printf("MQTT connected\n");

    // Subscribe to topic
    rc = client.subscribe(topic, MQTT::QOS2, messageArrived);
    if (rc != 0) {
        printf("rc from MQTT subscribe is %d\n", rc);
    }
    printf("MQTT subscribed\n");

#endif

    while (true) {
#ifdef USE_MQTT
        if (!client.isConnected()) {
            printf("Not connected...\n");
            rc = client.connect(data);
            if (rc != 0) {
                printf("rc from MQTT connect is %d\n", rc);
            }
        }

        client.yield(100); // Process MQTT messages
#endif
        tight_loop_contents();
    }
}
