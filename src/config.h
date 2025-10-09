#ifndef CONFIG_H
#define CONFIG_H

#include <Arduino.h>

// Version
#define FIRMWARE_VERSION "1.0.0"

// EEPROM/NVS Settings
#define CONFIG_NAMESPACE "meshcore_mqtt"
#define CONFIG_MAGIC 0x4D435147  // "MCQG" - MeshCore MQTT Gateway

// Default LoRa Settings
#define DEFAULT_LORA_FREQ 915.0f
#define DEFAULT_LORA_BW 125.0f
#define DEFAULT_LORA_SF 7
#define DEFAULT_LORA_CR 5
#define DEFAULT_LORA_TX_POWER 20

// Default MQTT Settings
#define DEFAULT_MQTT_SERVER "mqtt.example.com"
#define DEFAULT_MQTT_PORT 1883
#define DEFAULT_MQTT_USER ""
#define DEFAULT_MQTT_PASSWORD ""
#define DEFAULT_MQTT_CLIENT_ID "meshcore_gateway"
#define DEFAULT_MQTT_TOPIC_PREFIX "meshcore"

// WiFi Settings
#define DEFAULT_WIFI_SSID ""
#define DEFAULT_WIFI_PASSWORD ""

// Pin definitions for common ESP32 LoRa boards
#ifdef HELTEC_V3
    #define LORA_SCK 9
    #define LORA_MISO 11
    #define LORA_MOSI 10
    #define LORA_CS 8
    #define LORA_RST 12
    #define LORA_DIO0 14
    #define LORA_DIO1 -1
    #define LORA_BUSY 13
#elif defined(LILYGO_LORA32_V21)
    // LilyGo LoRa32 V2.1_1.6
    #define LORA_SCK 5
    #define LORA_MISO 19
    #define LORA_MOSI 27
    #define LORA_CS 18
    #define LORA_RST 23
    #define LORA_DIO0 26
    #define LORA_DIO1 33
    #define LORA_DIO2 32
    // Optional OLED display pins
    #define OLED_SDA 21
    #define OLED_SCL 22
    #define OLED_RST 16
#else
    // Generic ESP32 with LoRa module
    #define LORA_SCK 5
    #define LORA_MISO 19
    #define LORA_MOSI 27
    #define LORA_CS 18
    #define LORA_RST 14
    #define LORA_DIO0 26
    #define LORA_DIO1 35
    #define LORA_BUSY 32
#endif

// Configuration structures
struct WiFiConfig {
    char ssid[64];
    char password[64];
    bool enabled;
};

struct MQTTConfig {
    char server[128];
    uint16_t port;
    char username[64];
    char password[64];
    char clientId[64];
    char topicPrefix[64];
    bool enabled;
    bool publishRaw;         // Publish raw hex data
    bool publishDecoded;     // Publish decoded messages
    bool subscribeCommands;  // Subscribe to command topics
};

struct LoRaConfig {
    float frequency;
    float bandwidth;
    uint8_t spreadingFactor;
    uint8_t codingRate;
    uint8_t txPower;
    uint8_t syncWord;
    bool enableCRC;
};

struct RepeaterConfig {
    char nodeName[32];
    uint32_t nodeId;
    uint8_t maxHops;
    bool autoAck;
    bool broadcastEnabled;
    uint16_t routeTimeout;  // seconds
};

struct GatewayConfig {
    uint32_t magic;
    WiFiConfig wifi;
    MQTTConfig mqtt;
    LoRaConfig lora;
    RepeaterConfig repeater;
};

// Default configuration
inline GatewayConfig getDefaultConfig() {
    GatewayConfig config;
    config.magic = CONFIG_MAGIC;
    
    // WiFi defaults
    strncpy(config.wifi.ssid, DEFAULT_WIFI_SSID, sizeof(config.wifi.ssid));
    strncpy(config.wifi.password, DEFAULT_WIFI_PASSWORD, sizeof(config.wifi.password));
    config.wifi.enabled = false;
    
    // MQTT defaults
    strncpy(config.mqtt.server, DEFAULT_MQTT_SERVER, sizeof(config.mqtt.server));
    config.mqtt.port = DEFAULT_MQTT_PORT;
    strncpy(config.mqtt.username, DEFAULT_MQTT_USER, sizeof(config.mqtt.username));
    strncpy(config.mqtt.password, DEFAULT_MQTT_PASSWORD, sizeof(config.mqtt.password));
    strncpy(config.mqtt.clientId, DEFAULT_MQTT_CLIENT_ID, sizeof(config.mqtt.clientId));
    strncpy(config.mqtt.topicPrefix, DEFAULT_MQTT_TOPIC_PREFIX, sizeof(config.mqtt.topicPrefix));
    config.mqtt.enabled = false;
    config.mqtt.publishRaw = true;
    config.mqtt.publishDecoded = true;
    config.mqtt.subscribeCommands = true;
    
    // LoRa defaults
    config.lora.frequency = DEFAULT_LORA_FREQ;
    config.lora.bandwidth = DEFAULT_LORA_BW;
    config.lora.spreadingFactor = DEFAULT_LORA_SF;
    config.lora.codingRate = DEFAULT_LORA_CR;
    config.lora.txPower = DEFAULT_LORA_TX_POWER;
    config.lora.syncWord = 0x12;
    config.lora.enableCRC = true;
    
    // Repeater defaults
    strncpy(config.repeater.nodeName, "MQTT-Gateway", sizeof(config.repeater.nodeName));
    config.repeater.nodeId = 0;  // Will be auto-generated
    config.repeater.maxHops = 3;
    config.repeater.autoAck = true;
    config.repeater.broadcastEnabled = true;
    config.repeater.routeTimeout = 300;
    
    return config;
}

#endif // CONFIG_H

