/**
 * MeshCore MQTT Gateway
 * 
 * This firmware is a LoRa MQTT gateway with serial configuration.
 * It bridges LoRa mesh messages to MQTT brokers.
 * 
 * Features:
 * - LoRa packet repeater functionality
 * - MQTT message bridging (LoRa <-> MQTT)
 * - Serial configuration interface
 * - WiFi connectivity
 * - Persistent configuration storage
 * 
 * Version: 1.0.0
 */

#include <Arduino.h>
#include <SPI.h>
#include <RadioLib.h>

// Configuration and handlers
#include "config.h"
#include "settings_manager.h"
#include "mqtt_handler.h"
#include "serial_config.h"

// LoRa radio object
SX1276 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, LORA_DIO1);

// Global objects
GatewayConfig config;
SettingsManager settingsManager;
MQTTHandler* mqttHandler = nullptr;
ConfigMenu* serialConfig = nullptr;

// Statistics
uint32_t packetsReceived = 0;
uint32_t packetsSent = 0;
uint32_t packetsForwarded = 0;
uint32_t packetsFailed = 0;

// Timing
unsigned long lastStatsPublish = 0;
unsigned long lastStatusBlink = 0;
unsigned long lastPacketCheck = 0;
bool configMode = false;

// Radio state
bool radioInitialized = false;
volatile bool packetReceived = false;

// Function declarations
void setupLoRa();
void handleLoRaReceive();
void handleLoRaPacket(uint8_t* data, size_t length, int rssi, float snr);
bool sendLoRaPacket(const uint8_t* data, size_t length);
void checkSerialInput();
void publishStats();
void blinkLED();
void setRadioFlag();

// Radio interrupt flag
void IRAM_ATTR setRadioFlag() {
    packetReceived = true;
}

void setup() {
    Serial.begin(115200);
    delay(1000);
    
    Serial.println(F("\n\n"));
    Serial.println(F("╔════════════════════════════════════════════════════════╗"));
    Serial.println(F("║                                                        ║"));
    Serial.println(F("║        MeshCore MQTT Gateway v" FIRMWARE_VERSION "                ║"));
    Serial.println(F("║                                                        ║"));
    Serial.println(F("╚════════════════════════════════════════════════════════╝"));
    Serial.println();
    
    // Initialize settings manager
    if (!settingsManager.begin()) {
        Serial.println(F("✗ Failed to initialize settings manager"));
    }
    
    // Load configuration or use defaults
    if (!settingsManager.loadConfig(config)) {
        Serial.println(F("⚠ No saved configuration found, using defaults"));
        config = getDefaultConfig();
        settingsManager.saveConfig(config);
    } else {
        Serial.println(F("✓ Configuration loaded"));
    }
    
    // Generate node ID if not set
    if (config.repeater.nodeId == 0) {
        uint64_t chipid = ESP.getEfuseMac();
        config.repeater.nodeId = (uint32_t)(chipid & 0xFFFFFFFF);
        settingsManager.saveConfig(config);
        Serial.printf("✓ Generated Node ID: 0x%08X\n", config.repeater.nodeId);
    }
    
    Serial.println();
    Serial.println(F("┌────────────────────────────────────────────────────────┐"));
    Serial.printf("│ Node Name: %-43s │\n", config.repeater.nodeName);
    Serial.printf("│ Node ID:   0x%08X                                  │\n", config.repeater.nodeId);
    Serial.printf("│ LoRa Freq: %.2f MHz                                │\n", config.lora.frequency);
    Serial.printf("│ WiFi:      %-43s │\n", config.wifi.enabled ? "Enabled" : "Disabled");
    Serial.printf("│ MQTT:      %-43s │\n", config.mqtt.enabled ? "Enabled" : "Disabled");
    Serial.println(F("└────────────────────────────────────────────────────────┘"));
    Serial.println();
    
    // Setup LoRa
    Serial.println(F("Initializing LoRa..."));
    setupLoRa();
    Serial.println(F("✓ LoRa initialized"));
    
    // Setup MQTT if enabled
    if (config.wifi.enabled && config.mqtt.enabled) {
        Serial.println(F("\nInitializing MQTT..."));
        mqttHandler = new MQTTHandler(config);
        
        // Set callback for MQTT -> LoRa messages
        mqttHandler->setMessageCallback([](const uint8_t* payload, size_t length) {
            Serial.printf("Forwarding MQTT message to LoRa (%d bytes)\n", length);
            sendLoRaPacket(payload, length);
        });
        
        if (mqttHandler->begin()) {
            Serial.println(F("✓ MQTT initialized"));
            mqttHandler->publishGatewayStatus(true);
        } else {
            Serial.println(F("✗ MQTT initialization failed"));
        }
    } else {
        Serial.println(F("⚠ MQTT disabled (WiFi or MQTT not enabled in config)"));
    }
    
    // Setup serial configuration interface
    serialConfig = new ConfigMenu(config, settingsManager);
    serialConfig->begin();
    
    Serial.println();
    Serial.println(F("════════════════════════════════════════════════════════"));
    Serial.println(F("✓ Gateway started successfully!"));
    Serial.println(F("════════════════════════════════════════════════════════"));
    Serial.println();
    Serial.println(F("Commands:"));
    Serial.println(F("  'c' - Enter configuration menu"));
    Serial.println(F("  's' - Show statistics"));
    Serial.println(F("  'r' - Restart device"));
    Serial.println();
}

void loop() {
    // Handle LoRa messages
    handleLoRaReceive();
    
    // Handle MQTT
    if (mqttHandler) {
        mqttHandler->loop();
    }
    
    // Check for serial commands
    if (!configMode) {
        checkSerialInput();
    } else {
        serialConfig->handleMenu();
    }
    
    // Publish statistics periodically
    unsigned long now = millis();
    if (mqttHandler && mqttHandler->isConnected() && now - lastStatsPublish > 60000) {
        publishStats();
        lastStatsPublish = now;
    }
    
    // Blink status LED
    if (now - lastStatusBlink > 1000) {
        blinkLED();
        lastStatusBlink = now;
    }
    
    yield();
}

void setupLoRa() {
    // Initialize SPI
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
    
    Serial.print(F("Initializing SX1276 radio... "));
    
    // Initialize radio with configuration
    int state = radio.begin(
        config.lora.frequency,
        config.lora.bandwidth,
        config.lora.spreadingFactor,
        config.lora.codingRate,
        config.lora.syncWord,
        config.lora.txPower,
        8,  // preamble length
        0   // gain (0 = auto)
    );
    
    if (state == RADIOLIB_ERR_NONE) {
        Serial.println(F("success!"));
        
        // Enable CRC if configured
        if (config.lora.enableCRC) {
            radio.setCRC(true);
        }
        
        // Set DIO0 to trigger on packet reception
        radio.setDio0Action(setRadioFlag, RISING);
        
        // Start listening
        state = radio.startReceive();
        if (state == RADIOLIB_ERR_NONE) {
            Serial.println(F("✓ Radio listening for packets"));
            radioInitialized = true;
        } else {
            Serial.print(F("✗ Failed to start receive, code: "));
            Serial.println(state);
        }
    } else {
        Serial.print(F("failed, code: "));
        Serial.println(state);
        Serial.println(F("✗ Check wiring and antenna!"));
    }
    
    // Print radio configuration
    Serial.println(F("\n┌── LoRa Configuration ──────────────────────────────────┐"));
    Serial.printf("│ Frequency:        %.2f MHz                            │\n", config.lora.frequency);
    Serial.printf("│ Bandwidth:        %.1f kHz                            │\n", config.lora.bandwidth);
    Serial.printf("│ Spreading Factor: %-36d│\n", config.lora.spreadingFactor);
    Serial.printf("│ Coding Rate:      4/%-34d│\n", config.lora.codingRate);
    Serial.printf("│ TX Power:         %d dBm                               │\n", config.lora.txPower);
    Serial.printf("│ Sync Word:        0x%02X                                  │\n", config.lora.syncWord);
    Serial.printf("│ CRC:              %-36s│\n", config.lora.enableCRC ? "Enabled" : "Disabled");
    Serial.println(F("└────────────────────────────────────────────────────────┘\n"));
}

void handleLoRaReceive() {
    if (!radioInitialized) return;
    
    // Check if packet was received
    if (packetReceived) {
        packetReceived = false;
        
        // Buffer for received data
        uint8_t buffer[256];
        
        // Read received data
        int state = radio.readData(buffer, sizeof(buffer));
        
        if (state == RADIOLIB_ERR_NONE) {
            // Get packet info
            size_t length = radio.getPacketLength();
            int rssi = radio.getRSSI();
            float snr = radio.getSNR();
            
            packetsReceived++;
            
            // Handle the packet
            handleLoRaPacket(buffer, length, rssi, snr);
            
        } else if (state == RADIOLIB_ERR_CRC_MISMATCH) {
            Serial.println(F("⚠ CRC error!"));
        }
        
        // Put radio back into receive mode
        radio.startReceive();
    }
}

void handleLoRaPacket(uint8_t* data, size_t length, int rssi, float snr) {
    // Log to serial
    Serial.printf("\n📡 LoRa RX: %d bytes | RSSI: %d dBm | SNR: %.1f dB\n", length, rssi, snr);
    
    // Print hex dump (first 32 bytes)
    Serial.print("   Data: ");
    for (size_t i = 0; i < min(length, (size_t)32); i++) {
        Serial.printf("%02X ", data[i]);
    }
    if (length > 32) Serial.print("...");
    Serial.println();
    
    // Try to interpret as text if printable
    bool isPrintable = true;
    for (size_t i = 0; i < length; i++) {
        if (data[i] < 32 || data[i] > 126) {
            isPrintable = false;
            break;
        }
    }
    
    if (isPrintable && length > 0) {
        Serial.print("   Text: \"");
        for (size_t i = 0; i < length; i++) {
            Serial.write(data[i]);
        }
        Serial.println("\"");
    }
    
    // Forward to MQTT if connected
    if (mqttHandler && mqttHandler->isConnected()) {
        // Publish raw packet
        if (config.mqtt.publishRaw) {
            mqttHandler->publishRawPacket(data, length, rssi, snr);
        }
        
        // Publish decoded message if it looks like text
        if (config.mqtt.publishDecoded && isPrintable) {
            char message[256] = {0};
            size_t msgLen = min(length, sizeof(message) - 1);
            memcpy(message, data, msgLen);
            
            mqttHandler->publishDecodedMessage(
                config.repeater.nodeId,  // from (us)
                0xFFFFFFFF,              // to (broadcast)
                message,
                0,                       // message type
                rssi,
                snr,
                0                        // hop count
            );
        }
        
        packetsForwarded++;
    }
    
    // Optional: Repeat packet if configured as repeater
    // This is a simple repeater - just retransmit what we receive
    // In a real mesh implementation, you'd check hop count, routing, etc.
    if (config.repeater.maxHops > 0 && length > 0) {
        // Simple delay to avoid collisions
        delay(random(100, 300));
        
        // Retransmit
        if (sendLoRaPacket(data, length)) {
            Serial.println("   ↻ Packet repeated");
        }
    }
}

bool sendLoRaPacket(const uint8_t* data, size_t length) {
    if (!radioInitialized || length == 0 || length > 255) {
        packetsFailed++;
        return false;
    }
    
    Serial.printf("\n📤 LoRa TX: %d bytes\n", length);
    
    // Transmit the packet
    int state = radio.transmit((uint8_t*)data, length);
    
    if (state == RADIOLIB_ERR_NONE) {
        packetsSent++;
        Serial.println("   ✓ Sent successfully");
        
        // Put radio back into receive mode
        radio.startReceive();
        return true;
    } else {
        packetsFailed++;
        Serial.print("   ✗ Failed, code: ");
        Serial.println(state);
        
        // Try to recover
        radio.startReceive();
        return false;
    }
}

void checkSerialInput() {
    if (Serial.available()) {
        char c = Serial.read();
        
        switch (c) {
            case 'c':
            case 'C':
                configMode = true;
                serialConfig->showMainMenu();
                break;
                
            case 's':
            case 'S':
                Serial.println(F("\n┌── Gateway Statistics ──────────────────────────────────┐"));
                Serial.printf("│ Uptime:           %-36lu │\n", millis() / 1000);
                Serial.printf("│ Packets Received: %-36lu │\n", packetsReceived);
                Serial.printf("│ Packets Sent:     %-36lu │\n", packetsSent);
                Serial.printf("│ Packets Forwarded:%-36lu │\n", packetsForwarded);
                Serial.printf("│ Packets Failed:   %-36lu │\n", packetsFailed);
                Serial.printf("│ Free Heap:        %-36lu │\n", ESP.getFreeHeap());
                
                if (WiFi.status() == WL_CONNECTED) {
                    Serial.printf("│ WiFi RSSI:        %-36d │\n", WiFi.RSSI());
                    Serial.printf("│ IP Address:       %-36s │\n", WiFi.localIP().toString().c_str());
                }
                
                if (mqttHandler && mqttHandler->isConnected()) {
                    Serial.println(F("│ MQTT:             Connected                            │"));
                } else {
                    Serial.println(F("│ MQTT:             Disconnected                         │"));
                }
                
                Serial.println(F("└────────────────────────────────────────────────────────┘\n"));
                break;
                
            case 'r':
            case 'R':
                Serial.println(F("\n⚠ Restarting device..."));
                delay(1000);
                ESP.restart();
                break;
        }
    }
}

void publishStats() {
    if (mqttHandler) {
        mqttHandler->publishStats(packetsReceived, packetsSent, packetsForwarded, packetsFailed);
    }
}

void blinkLED() {
    // TODO: Add LED blinking based on status
    // - Slow blink: Normal operation
    // - Fast blink: WiFi connecting
    // - Solid: MQTT connected
    // You can use the built-in LED or an external LED
}

// Exit configuration mode helper
void exitConfigMode() {
    configMode = false;
    Serial.println(F("\n✓ Exited configuration mode"));
    
    // Restart if configuration changed significantly
    Serial.println(F("⚠ Some changes may require a restart"));
}

