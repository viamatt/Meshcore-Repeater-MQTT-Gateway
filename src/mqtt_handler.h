#ifndef MQTT_HANDLER_H
#define MQTT_HANDLER_H

#include <Arduino.h>
#include <WiFi.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include "config.h"

// Forward declarations
class MQTTHandler;

// Callback types
typedef std::function<void(const uint8_t* payload, size_t length)> MQTTMessageCallback;

class MQTTHandler {
public:
    MQTTHandler(GatewayConfig& cfg) 
        : config(cfg), wifiClient(), mqttClient(wifiClient), 
          lastReconnectAttempt(0), messageCallback(nullptr) {}
    
    bool begin() {
        if (!config.wifi.enabled || !config.mqtt.enabled) {
            return false;
        }
        
        // Connect to WiFi
        if (!connectWiFi()) {
            return false;
        }
        
        // Setup MQTT
        mqttClient.setServer(config.mqtt.server, config.mqtt.port);
        mqttClient.setCallback([this](char* topic, byte* payload, unsigned int length) {
            this->handleMQTTMessage(topic, payload, length);
        });
        mqttClient.setBufferSize(1024);  // Increase buffer for larger messages
        
        return connectMQTT();
    }
    
    void loop() {
        // Handle WiFi reconnection
        if (WiFi.status() != WL_CONNECTED) {
            Serial.println(F("WiFi disconnected, reconnecting..."));
            connectWiFi();
            return;
        }
        
        // Handle MQTT reconnection
        if (!mqttClient.connected()) {
            unsigned long now = millis();
            if (now - lastReconnectAttempt > 5000) {
                lastReconnectAttempt = now;
                if (connectMQTT()) {
                    lastReconnectAttempt = 0;
                }
            }
        } else {
            mqttClient.loop();
        }
    }
    
    bool isConnected() {
        return mqttClient.connected();
    }
    
    // Publish raw LoRa packet
    void publishRawPacket(const uint8_t* data, size_t length, int rssi, float snr) {
        if (!config.mqtt.publishRaw || !mqttClient.connected()) {
            return;
        }
        
        char topic[128];
        snprintf(topic, sizeof(topic), "%s/raw", config.mqtt.topicPrefix);
        
        // Create JSON payload
        StaticJsonDocument<512> doc;
        doc["timestamp"] = millis();
        doc["rssi"] = rssi;
        doc["snr"] = snr;
        
        // Convert data to hex string
        char hexStr[length * 2 + 1];
        for (size_t i = 0; i < length; i++) {
            sprintf(&hexStr[i * 2], "%02X", data[i]);
        }
        hexStr[length * 2] = '\0';
        doc["data"] = hexStr;
        doc["length"] = length;
        
        String output;
        serializeJson(doc, output);
        
        mqttClient.publish(topic, output.c_str(), false);
    }
    
    // Publish decoded message
    void publishDecodedMessage(uint32_t fromId, uint32_t toId, const char* message, 
                              uint8_t messageType, int rssi, float snr, uint8_t hopCount) {
        if (!config.mqtt.publishDecoded || !mqttClient.connected()) {
            return;
        }
        
        char topic[128];
        snprintf(topic, sizeof(topic), "%s/messages", config.mqtt.topicPrefix);
        
        StaticJsonDocument<1024> doc;
        doc["timestamp"] = millis();
        doc["from"] = fromId;
        doc["to"] = toId;
        doc["message"] = message;
        doc["type"] = messageType;
        doc["rssi"] = rssi;
        doc["snr"] = snr;
        doc["hops"] = hopCount;
        doc["gateway"] = config.mqtt.clientId;
        
        String output;
        serializeJson(doc, output);
        
        mqttClient.publish(topic, output.c_str(), false);
    }
    
    // Publish node info
    void publishNodeInfo(uint32_t nodeId, const char* nodeName, bool online) {
        if (!mqttClient.connected()) {
            return;
        }
        
        char topic[128];
        snprintf(topic, sizeof(topic), "%s/nodes/%08X", config.mqtt.topicPrefix, nodeId);
        
        StaticJsonDocument<256> doc;
        doc["nodeId"] = nodeId;
        doc["name"] = nodeName;
        doc["online"] = online;
        doc["timestamp"] = millis();
        doc["gateway"] = config.mqtt.clientId;
        
        String output;
        serializeJson(doc, output);
        
        mqttClient.publish(topic, output.c_str(), true);  // Retain node info
    }
    
    // Publish gateway statistics
    void publishStats(uint32_t packetsReceived, uint32_t packetsSent, 
                     uint32_t packetsForwarded, uint32_t packetsFailed) {
        if (!mqttClient.connected()) {
            return;
        }
        
        char topic[128];
        snprintf(topic, sizeof(topic), "%s/gateway/%s/stats", 
                config.mqtt.topicPrefix, config.mqtt.clientId);
        
        StaticJsonDocument<512> doc;
        doc["timestamp"] = millis();
        doc["uptime"] = millis() / 1000;
        doc["packetsReceived"] = packetsReceived;
        doc["packetsSent"] = packetsSent;
        doc["packetsForwarded"] = packetsForwarded;
        doc["packetsFailed"] = packetsFailed;
        doc["rssi"] = WiFi.RSSI();
        doc["freeHeap"] = ESP.getFreeHeap();
        
        String output;
        serializeJson(doc, output);
        
        mqttClient.publish(topic, output.c_str(), false);
    }
    
    // Publish gateway status
    void publishGatewayStatus(bool online) {
        if (!mqttClient.connected()) {
            return;
        }
        
        char topic[128];
        snprintf(topic, sizeof(topic), "%s/gateway/%s/status", 
                config.mqtt.topicPrefix, config.mqtt.clientId);
        
        StaticJsonDocument<256> doc;
        doc["online"] = online;
        doc["timestamp"] = millis();
        doc["ip"] = WiFi.localIP().toString();
        doc["rssi"] = WiFi.RSSI();
        
        String output;
        serializeJson(doc, output);
        
        mqttClient.publish(topic, output.c_str(), true);  // Retain status
    }
    
    // Set callback for incoming MQTT messages that should be sent to LoRa
    void setMessageCallback(MQTTMessageCallback callback) {
        messageCallback = callback;
    }
    
private:
    GatewayConfig& config;
    WiFiClient wifiClient;
    PubSubClient mqttClient;
    unsigned long lastReconnectAttempt;
    MQTTMessageCallback messageCallback;
    
    bool connectWiFi() {
        if (WiFi.status() == WL_CONNECTED) {
            return true;
        }
        
        Serial.print(F("\nConnecting to WiFi: "));
        Serial.println(config.wifi.ssid);
        
        WiFi.mode(WIFI_STA);
        WiFi.begin(config.wifi.ssid, config.wifi.password);
        
        int attempts = 0;
        while (WiFi.status() != WL_CONNECTED && attempts < 30) {
            delay(500);
            Serial.print(F("."));
            attempts++;
        }
        
        if (WiFi.status() == WL_CONNECTED) {
            Serial.println(F("\n✓ WiFi connected"));
            Serial.print(F("IP: "));
            Serial.println(WiFi.localIP());
            return true;
        } else {
            Serial.println(F("\n✗ WiFi connection failed"));
            return false;
        }
    }
    
    bool connectMQTT() {
        if (mqttClient.connected()) {
            return true;
        }
        
        Serial.print(F("Connecting to MQTT: "));
        Serial.println(config.mqtt.server);
        
        // Prepare last will message
        char willTopic[128];
        snprintf(willTopic, sizeof(willTopic), "%s/gateway/%s/status", 
                config.mqtt.topicPrefix, config.mqtt.clientId);
        
        StaticJsonDocument<128> willDoc;
        willDoc["online"] = false;
        willDoc["timestamp"] = millis();
        String willPayload;
        serializeJson(willDoc, willPayload);
        
        bool connected = false;
        if (strlen(config.mqtt.username) > 0) {
            connected = mqttClient.connect(
                config.mqtt.clientId,
                config.mqtt.username,
                config.mqtt.password,
                willTopic,
                1,
                true,
                willPayload.c_str()
            );
        } else {
            connected = mqttClient.connect(
                config.mqtt.clientId,
                willTopic,
                1,
                true,
                willPayload.c_str()
            );
        }
        
        if (connected) {
            Serial.println(F("✓ MQTT connected"));
            
            // Subscribe to command topics
            if (config.mqtt.subscribeCommands) {
                char cmdTopic[128];
                snprintf(cmdTopic, sizeof(cmdTopic), "%s/commands/#", config.mqtt.topicPrefix);
                mqttClient.subscribe(cmdTopic);
                Serial.print(F("Subscribed to: "));
                Serial.println(cmdTopic);
            }
            
            // Publish online status
            publishGatewayStatus(true);
            
            return true;
        } else {
            Serial.print(F("✗ MQTT connection failed, rc="));
            Serial.println(mqttClient.state());
            return false;
        }
    }
    
    void handleMQTTMessage(char* topic, byte* payload, unsigned int length) {
        Serial.print(F("MQTT message received: "));
        Serial.println(topic);
        
        // Parse topic to determine action
        String topicStr = String(topic);
        String prefix = String(config.mqtt.topicPrefix) + "/commands/";
        
        if (topicStr.startsWith(prefix)) {
            String command = topicStr.substring(prefix.length());
            
            if (command == "send" && messageCallback) {
                // Forward message to LoRa via callback
                messageCallback(payload, length);
            } else if (command == "restart") {
                Serial.println(F("Restart command received via MQTT"));
                delay(1000);
                ESP.restart();
            }
        }
    }
};

#endif // MQTT_HANDLER_H

