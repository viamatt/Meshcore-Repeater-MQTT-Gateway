#ifndef SERIAL_CONFIG_H
#define SERIAL_CONFIG_H

#include <Arduino.h>
#include "config.h"
#include "settings_manager.h"

class ConfigMenu {
public:
    ConfigMenu(GatewayConfig& cfg, SettingsManager& settings) 
        : config(cfg), settingsManager(settings) {}
    
    void begin() {
        Serial.println(F("\n╔════════════════════════════════════════════════════════╗"));
        Serial.println(F("║    MeshCore MQTT Gateway - Configuration Menu         ║"));
        Serial.println(F("╚════════════════════════════════════════════════════════╝"));
    }
    
    void showMainMenu() {
        Serial.println(F("\n┌────────────────────────────────────────────────────────┐"));
        Serial.println(F("│ MAIN MENU                                              │"));
        Serial.println(F("├────────────────────────────────────────────────────────┤"));
        Serial.println(F("│ 1. WiFi Settings                                       │"));
        Serial.println(F("│ 2. MQTT Settings                                       │"));
        Serial.println(F("│ 3. LoRa Settings                                       │"));
        Serial.println(F("│ 4. Repeater Settings                                   │"));
        Serial.println(F("│ 5. Show Current Configuration                          │"));
        Serial.println(F("│ 6. Save Configuration                                  │"));
        Serial.println(F("│ 7. Reset to Defaults                                   │"));
        Serial.println(F("│ 8. Restart Device                                      │"));
        Serial.println(F("│ 0. Exit Configuration                                  │"));
        Serial.println(F("└────────────────────────────────────────────────────────┘"));
        Serial.print(F("\nEnter choice: "));
    }
    
    void handleMenu() {
        if (Serial.available()) {
            String input = Serial.readStringUntil('\n');
            input.trim();
            
            if (input.length() == 0) return;
            
            int choice = input.toInt();
            
            switch (choice) {
                case 1: configureWiFi(); break;
                case 2: configureMQTT(); break;
                case 3: configureLoRa(); break;
                case 4: configureRepeater(); break;
                case 5: showConfiguration(); break;
                case 6: saveConfiguration(); break;
                case 7: resetToDefaults(); break;
                case 8: restartDevice(); break;
                case 0: exitConfig(); return;
                default:
                    Serial.println(F("Invalid choice!"));
                    break;
            }
            
            showMainMenu();
        }
    }
    
private:
    GatewayConfig& config;
    SettingsManager& settingsManager;
    
    String readLine(const char* prompt, const String& defaultValue = "") {
        Serial.print(prompt);
        if (defaultValue.length() > 0) {
            Serial.print(F(" ["));
            Serial.print(defaultValue);
            Serial.print(F("]"));
        }
        Serial.print(F(": "));
        
        while (!Serial.available()) delay(10);
        String input = Serial.readStringUntil('\n');
        input.trim();
        
        if (input.length() == 0 && defaultValue.length() > 0) {
            return defaultValue;
        }
        
        return input;
    }
    
    bool readBool(const char* prompt, bool defaultValue) {
        String def = defaultValue ? "y" : "n";
        String input = readLine(prompt, def);
        input.toLowerCase();
        return (input == "y" || input == "yes" || input == "1" || input == "true");
    }
    
    int readInt(const char* prompt, int defaultValue) {
        String input = readLine(prompt, String(defaultValue));
        return input.toInt();
    }
    
    float readFloat(const char* prompt, float defaultValue) {
        String input = readLine(prompt, String(defaultValue, 2));
        return input.toFloat();
    }
    
    void configureWiFi() {
        Serial.println(F("\n┌── WiFi Configuration ──────────────────────────────────┐"));
        
        config.wifi.enabled = readBool("Enable WiFi (y/n)", config.wifi.enabled);
        
        if (config.wifi.enabled) {
            String ssid = readLine("WiFi SSID", String(config.wifi.ssid));
            strncpy(config.wifi.ssid, ssid.c_str(), sizeof(config.wifi.ssid) - 1);
            
            String pass = readLine("WiFi Password", "********");
            if (pass != "********") {
                strncpy(config.wifi.password, pass.c_str(), sizeof(config.wifi.password) - 1);
            }
        }
        
        Serial.println(F("└────────────────────────────────────────────────────────┘"));
        Serial.println(F("✓ WiFi configuration updated"));
    }
    
    void configureMQTT() {
        Serial.println(F("\n┌── MQTT Configuration ──────────────────────────────────┐"));
        
        config.mqtt.enabled = readBool("Enable MQTT (y/n)", config.mqtt.enabled);
        
        if (config.mqtt.enabled) {
            String server = readLine("MQTT Server", String(config.mqtt.server));
            strncpy(config.mqtt.server, server.c_str(), sizeof(config.mqtt.server) - 1);
            
            config.mqtt.port = readInt("MQTT Port", config.mqtt.port);
            
            String user = readLine("MQTT Username", String(config.mqtt.username));
            strncpy(config.mqtt.username, user.c_str(), sizeof(config.mqtt.username) - 1);
            
            String pass = readLine("MQTT Password", "********");
            if (pass != "********") {
                strncpy(config.mqtt.password, pass.c_str(), sizeof(config.mqtt.password) - 1);
            }
            
            String clientId = readLine("Client ID", String(config.mqtt.clientId));
            strncpy(config.mqtt.clientId, clientId.c_str(), sizeof(config.mqtt.clientId) - 1);
            
            String prefix = readLine("Topic Prefix", String(config.mqtt.topicPrefix));
            strncpy(config.mqtt.topicPrefix, prefix.c_str(), sizeof(config.mqtt.topicPrefix) - 1);
            
            config.mqtt.publishRaw = readBool("Publish raw packets (y/n)", config.mqtt.publishRaw);
            config.mqtt.publishDecoded = readBool("Publish decoded messages (y/n)", config.mqtt.publishDecoded);
            config.mqtt.subscribeCommands = readBool("Subscribe to commands (y/n)", config.mqtt.subscribeCommands);
        }
        
        Serial.println(F("└────────────────────────────────────────────────────────┘"));
        Serial.println(F("✓ MQTT configuration updated"));
    }
    
    void configureLoRa() {
        Serial.println(F("\n┌── LoRa Configuration ──────────────────────────────────┐"));
        
        config.lora.frequency = readFloat("Frequency (MHz)", config.lora.frequency);
        config.lora.bandwidth = readFloat("Bandwidth (kHz)", config.lora.bandwidth);
        config.lora.spreadingFactor = readInt("Spreading Factor (7-12)", config.lora.spreadingFactor);
        config.lora.codingRate = readInt("Coding Rate (5-8)", config.lora.codingRate);
        config.lora.txPower = readInt("TX Power (2-20 dBm)", config.lora.txPower);
        config.lora.syncWord = readInt("Sync Word (hex)", config.lora.syncWord);
        config.lora.enableCRC = readBool("Enable CRC (y/n)", config.lora.enableCRC);
        
        Serial.println(F("└────────────────────────────────────────────────────────┘"));
        Serial.println(F("✓ LoRa configuration updated"));
        Serial.println(F("⚠ Restart required for LoRa changes to take effect"));
    }
    
    void configureRepeater() {
        Serial.println(F("\n┌── Repeater Configuration ──────────────────────────────┐"));
        
        String nodeName = readLine("Node Name", String(config.repeater.nodeName));
        strncpy(config.repeater.nodeName, nodeName.c_str(), sizeof(config.repeater.nodeName) - 1);
        
        config.repeater.maxHops = readInt("Max Hops (1-7)", config.repeater.maxHops);
        config.repeater.autoAck = readBool("Auto ACK (y/n)", config.repeater.autoAck);
        config.repeater.broadcastEnabled = readBool("Broadcast Enabled (y/n)", config.repeater.broadcastEnabled);
        config.repeater.routeTimeout = readInt("Route Timeout (seconds)", config.repeater.routeTimeout);
        
        Serial.println(F("└────────────────────────────────────────────────────────┘"));
        Serial.println(F("✓ Repeater configuration updated"));
    }
    
    void showConfiguration() {
        Serial.println(F("\n╔════════════════════════════════════════════════════════╗"));
        Serial.println(F("║ CURRENT CONFIGURATION                                  ║"));
        Serial.println(F("╠════════════════════════════════════════════════════════╣"));
        
        // WiFi
        Serial.println(F("║ WiFi Settings:                                         ║"));
        Serial.printf("║   Enabled: %-43s ║\n", config.wifi.enabled ? "Yes" : "No");
        Serial.printf("║   SSID: %-46s ║\n", config.wifi.ssid);
        Serial.printf("║   Password: %-42s ║\n", strlen(config.wifi.password) > 0 ? "********" : "(none)");
        
        // MQTT
        Serial.println(F("╠════════════════════════════════════════════════════════╣"));
        Serial.println(F("║ MQTT Settings:                                         ║"));
        Serial.printf("║   Enabled: %-43s ║\n", config.mqtt.enabled ? "Yes" : "No");
        Serial.printf("║   Server: %-46s ║\n", config.mqtt.server);
        Serial.printf("║   Port: %-48d║\n", config.mqtt.port);
        Serial.printf("║   Username: %-44s ║\n", config.mqtt.username);
        Serial.printf("║   Client ID: %-43s ║\n", config.mqtt.clientId);
        Serial.printf("║   Topic Prefix: %-40s ║\n", config.mqtt.topicPrefix);
        Serial.printf("║   Publish Raw: %-41s ║\n", config.mqtt.publishRaw ? "Yes" : "No");
        Serial.printf("║   Publish Decoded: %-37s ║\n", config.mqtt.publishDecoded ? "Yes" : "No");
        
        // LoRa
        Serial.println(F("╠════════════════════════════════════════════════════════╣"));
        Serial.println(F("║ LoRa Settings:                                         ║"));
        Serial.printf("║   Frequency: %.2f MHz                                 ║\n", config.lora.frequency);
        Serial.printf("║   Bandwidth: %.1f kHz                                 ║\n", config.lora.bandwidth);
        Serial.printf("║   Spreading Factor: %-35d║\n", config.lora.spreadingFactor);
        Serial.printf("║   Coding Rate: %-40d║\n", config.lora.codingRate);
        Serial.printf("║   TX Power: %-43d║\n", config.lora.txPower);
        Serial.printf("║   Sync Word: 0x%02X                                       ║\n", config.lora.syncWord);
        
        // Repeater
        Serial.println(F("╠════════════════════════════════════════════════════════╣"));
        Serial.println(F("║ Repeater Settings:                                     ║"));
        Serial.printf("║   Node Name: %-43s ║\n", config.repeater.nodeName);
        Serial.printf("║   Node ID: 0x%08X                                  ║\n", config.repeater.nodeId);
        Serial.printf("║   Max Hops: %-44d║\n", config.repeater.maxHops);
        Serial.printf("║   Auto ACK: %-44s║\n", config.repeater.autoAck ? "Yes" : "No");
        Serial.printf("║   Broadcast: %-43s║\n", config.repeater.broadcastEnabled ? "Yes" : "No");
        
        Serial.println(F("╚════════════════════════════════════════════════════════╝"));
    }
    
    void saveConfiguration() {
        Serial.print(F("\nSaving configuration... "));
        if (settingsManager.saveConfig(config)) {
            Serial.println(F("✓ Done!"));
        } else {
            Serial.println(F("✗ Failed!"));
        }
    }
    
    void resetToDefaults() {
        Serial.print(F("\n⚠ Reset to factory defaults? (y/n): "));
        while (!Serial.available()) delay(10);
        String input = Serial.readStringUntil('\n');
        input.trim();
        input.toLowerCase();
        
        if (input == "y" || input == "yes") {
            config = getDefaultConfig();
            settingsManager.clearConfig();
            Serial.println(F("✓ Configuration reset to defaults"));
            Serial.println(F("⚠ Don't forget to save!"));
        } else {
            Serial.println(F("Cancelled"));
        }
    }
    
    void restartDevice() {
        Serial.print(F("\n⚠ Restart device now? (y/n): "));
        while (!Serial.available()) delay(10);
        String input = Serial.readStringUntil('\n');
        input.trim();
        input.toLowerCase();
        
        if (input == "y" || input == "yes") {
            Serial.println(F("\nRestarting..."));
            delay(1000);
            ESP.restart();
        } else {
            Serial.println(F("Cancelled"));
        }
    }
    
    void exitConfig() {
        Serial.println(F("\nExiting configuration menu..."));
        Serial.println(F("Press 'c' to return to configuration\n"));
    }
};

#endif // SERIAL_CONFIG_H

