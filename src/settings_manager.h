#ifndef SETTINGS_MANAGER_H
#define SETTINGS_MANAGER_H

#include <Arduino.h>
#include <Preferences.h>
#include "config.h"

class SettingsManager {
public:
    SettingsManager() : prefs() {}
    
    bool begin() {
        return prefs.begin(CONFIG_NAMESPACE, false);
    }
    
    void end() {
        prefs.end();
    }
    
    bool saveConfig(const GatewayConfig& config) {
        prefs.begin(CONFIG_NAMESPACE, false);
        
        // Save magic number to indicate valid config
        prefs.putUInt("magic", config.magic);
        
        // WiFi settings
        prefs.putString("wifi_ssid", config.wifi.ssid);
        prefs.putString("wifi_pass", config.wifi.password);
        prefs.putBool("wifi_en", config.wifi.enabled);
        
        // MQTT settings
        prefs.putString("mqtt_srv", config.mqtt.server);
        prefs.putUShort("mqtt_port", config.mqtt.port);
        prefs.putString("mqtt_user", config.mqtt.username);
        prefs.putString("mqtt_pass", config.mqtt.password);
        prefs.putString("mqtt_id", config.mqtt.clientId);
        prefs.putString("mqtt_pfx", config.mqtt.topicPrefix);
        prefs.putBool("mqtt_en", config.mqtt.enabled);
        prefs.putBool("mqtt_raw", config.mqtt.publishRaw);
        prefs.putBool("mqtt_dec", config.mqtt.publishDecoded);
        prefs.putBool("mqtt_cmd", config.mqtt.subscribeCommands);
        
        // LoRa settings
        prefs.putFloat("lora_freq", config.lora.frequency);
        prefs.putFloat("lora_bw", config.lora.bandwidth);
        prefs.putUChar("lora_sf", config.lora.spreadingFactor);
        prefs.putUChar("lora_cr", config.lora.codingRate);
        prefs.putUChar("lora_pwr", config.lora.txPower);
        prefs.putUChar("lora_sw", config.lora.syncWord);
        prefs.putBool("lora_crc", config.lora.enableCRC);
        
        // Repeater settings
        prefs.putString("rep_name", config.repeater.nodeName);
        prefs.putUInt("rep_id", config.repeater.nodeId);
        prefs.putUChar("rep_hops", config.repeater.maxHops);
        prefs.putBool("rep_ack", config.repeater.autoAck);
        prefs.putBool("rep_bc", config.repeater.broadcastEnabled);
        prefs.putUShort("rep_tout", config.repeater.routeTimeout);
        
        prefs.end();
        return true;
    }
    
    bool loadConfig(GatewayConfig& config) {
        prefs.begin(CONFIG_NAMESPACE, true);
        
        // Check if config exists
        uint32_t magic = prefs.getUInt("magic", 0);
        if (magic != CONFIG_MAGIC) {
            prefs.end();
            return false;
        }
        
        config.magic = magic;
        
        // WiFi settings
        strncpy(config.wifi.ssid, prefs.getString("wifi_ssid", "").c_str(), sizeof(config.wifi.ssid));
        strncpy(config.wifi.password, prefs.getString("wifi_pass", "").c_str(), sizeof(config.wifi.password));
        config.wifi.enabled = prefs.getBool("wifi_en", false);
        
        // MQTT settings
        strncpy(config.mqtt.server, prefs.getString("mqtt_srv", DEFAULT_MQTT_SERVER).c_str(), sizeof(config.mqtt.server));
        config.mqtt.port = prefs.getUShort("mqtt_port", DEFAULT_MQTT_PORT);
        strncpy(config.mqtt.username, prefs.getString("mqtt_user", "").c_str(), sizeof(config.mqtt.username));
        strncpy(config.mqtt.password, prefs.getString("mqtt_pass", "").c_str(), sizeof(config.mqtt.password));
        strncpy(config.mqtt.clientId, prefs.getString("mqtt_id", DEFAULT_MQTT_CLIENT_ID).c_str(), sizeof(config.mqtt.clientId));
        strncpy(config.mqtt.topicPrefix, prefs.getString("mqtt_pfx", DEFAULT_MQTT_TOPIC_PREFIX).c_str(), sizeof(config.mqtt.topicPrefix));
        config.mqtt.enabled = prefs.getBool("mqtt_en", false);
        config.mqtt.publishRaw = prefs.getBool("mqtt_raw", true);
        config.mqtt.publishDecoded = prefs.getBool("mqtt_dec", true);
        config.mqtt.subscribeCommands = prefs.getBool("mqtt_cmd", true);
        
        // LoRa settings
        config.lora.frequency = prefs.getFloat("lora_freq", DEFAULT_LORA_FREQ);
        config.lora.bandwidth = prefs.getFloat("lora_bw", DEFAULT_LORA_BW);
        config.lora.spreadingFactor = prefs.getUChar("lora_sf", DEFAULT_LORA_SF);
        config.lora.codingRate = prefs.getUChar("lora_cr", DEFAULT_LORA_CR);
        config.lora.txPower = prefs.getUChar("lora_pwr", DEFAULT_LORA_TX_POWER);
        config.lora.syncWord = prefs.getUChar("lora_sw", 0x12);
        config.lora.enableCRC = prefs.getBool("lora_crc", true);
        
        // Repeater settings
        strncpy(config.repeater.nodeName, prefs.getString("rep_name", "MQTT-Gateway").c_str(), sizeof(config.repeater.nodeName));
        config.repeater.nodeId = prefs.getUInt("rep_id", 0);
        config.repeater.maxHops = prefs.getUChar("rep_hops", 3);
        config.repeater.autoAck = prefs.getBool("rep_ack", true);
        config.repeater.broadcastEnabled = prefs.getBool("rep_bc", true);
        config.repeater.routeTimeout = prefs.getUShort("rep_tout", 300);
        
        prefs.end();
        return true;
    }
    
    void clearConfig() {
        prefs.begin(CONFIG_NAMESPACE, false);
        prefs.clear();
        prefs.end();
    }
    
private:
    Preferences prefs;
};

#endif // SETTINGS_MANAGER_H

