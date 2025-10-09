# MeshCore Integration Guide

This guide explains how to integrate this MQTT Gateway firmware with the actual MeshCore library.

## Understanding MeshCore Library Structure

First, you need to understand the MeshCore library API. Based on the simple_repeater example from the MeshCore repository, you'll need to:

1. Include the appropriate MeshCore headers
2. Initialize the radio and mesh protocol
3. Set up packet handlers
4. Configure repeater mode

## Step-by-Step Integration

### Step 1: Examine the simple_repeater Example

Go to the MeshCore repository and examine:
```
examples/simple_repeater/simple_repeater.ino
```

Look for:
- Include statements
- Radio initialization code
- Packet receive handlers
- Packet send functions
- Repeater configuration

### Step 2: Identify Key Functions

From the simple_repeater example, identify these key functions:

```cpp
// Common patterns in MeshCore (adjust based on actual API)
void setup() {
    // Radio initialization
    radio.begin(frequency);
    radio.setSpreadingFactor(sf);
    radio.setBandwidth(bw);
    // ... etc
    
    // Mesh protocol initialization
    mesh.begin(nodeId);
    mesh.setRepeater(true);
    mesh.onReceive(onReceiveCallback);
}

void loop() {
    mesh.update();  // Process mesh protocol
}

void onReceiveCallback(uint32_t from, uint32_t to, uint8_t* payload, size_t len) {
    // Handle received packet
}
```

### Step 3: Update src/main.cpp

Based on what you find in the simple_repeater example, update the following sections in `src/main.cpp`:

#### A. Add Includes

Replace the commented include at the top:
```cpp
// Replace this:
// #include <MeshCore.h>

// With actual includes from simple_repeater:
#include <RadioLib.h>  // If MeshCore uses RadioLib
#include <MeshCore.h>  // Or whatever the actual header is
```

#### B. Add Global Objects

Add the radio and mesh objects:
```cpp
// Add after global object declarations
SX1276 radio = new Module(LORA_CS, LORA_DIO0, LORA_RST, LORA_DIO1);
MeshCore mesh(&radio);  // Adjust constructor based on actual API
```

#### C. Complete setupLoRa()

Update the `setupLoRa()` function:
```cpp
void setupLoRa() {
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
    
    // Initialize radio (example - adjust to actual API)
    int state = radio.begin(
        config.lora.frequency,
        config.lora.bandwidth,
        config.lora.spreadingFactor,
        config.lora.codingRate,
        config.lora.syncWord,
        config.lora.txPower
    );
    
    if (state != RADIOLIB_ERR_NONE) {
        Serial.printf("Radio initialization failed: %d\n", state);
        while (true);
    }
    
    // Initialize mesh protocol
    mesh.begin(config.repeater.nodeId);
    mesh.setNodeName(config.repeater.nodeName);
    mesh.setMaxHops(config.repeater.maxHops);
    mesh.setRepeaterMode(true);
    
    // Set receive callback
    mesh.onReceive([](uint32_t from, uint32_t to, uint8_t* payload, size_t len, int16_t rssi, float snr) {
        handleMeshPacket(from, to, payload, len, rssi, snr);
    });
    
    Serial.println(F("✓ MeshCore initialized"));
}
```

#### D. Complete handleLoRaReceive()

Update the packet receive function:
```cpp
void handleLoRaReceive() {
    // Call mesh update to process any pending packets
    mesh.update();
    
    // The actual packet handling is done in the callback
    // set up in setupLoRa()
}
```

#### E. Add Mesh Packet Handler

Add a new function to handle mesh packets:
```cpp
void handleMeshPacket(uint32_t fromId, uint32_t toId, uint8_t* payload, 
                     size_t length, int16_t rssi, float snr) {
    packetsReceived++;
    
    Serial.printf("Mesh packet: from=0x%08X to=0x%08X len=%d rssi=%d snr=%.1f\n",
                 fromId, toId, length, rssi, snr);
    
    // Forward to MQTT if connected
    if (mqttHandler && mqttHandler->isConnected()) {
        // Publish raw packet data
        if (config.mqtt.publishRaw) {
            mqttHandler->publishRawPacket(payload, length, rssi, snr);
        }
        
        // Publish decoded message
        if (config.mqtt.publishDecoded) {
            // Convert payload to string (if it's a text message)
            char message[256] = {0};
            size_t msgLen = min(length, sizeof(message) - 1);
            memcpy(message, payload, msgLen);
            
            mqttHandler->publishDecodedMessage(
                fromId, toId, message, 
                0,  // message type - you may need to extract this from packet
                rssi, snr,
                0   // hop count - extract from packet if available
            );
        }
        
        packetsForwarded++;
    }
    
    // MeshCore should automatically handle repeating if in repeater mode
}
```

#### F. Complete sendLoRaPacket()

Update the send function:
```cpp
void sendLoRaPacket(const uint8_t* data, size_t length) {
    Serial.printf("Sending LoRa packet: %d bytes\n", length);
    
    // Parse destination from MQTT message if in JSON format
    // For simple case, broadcast:
    uint32_t destId = 0xFFFFFFFF;  // Broadcast
    
    if (mesh.send(destId, data, length)) {
        packetsSent++;
        Serial.println(F("✓ Packet sent"));
    } else {
        packetsFailed++;
        Serial.println(F("✗ Packet send failed"));
    }
}
```

### Step 4: Update MQTT Message Format

Based on the actual MeshCore packet structure, you may need to update the MQTT message format in `mqtt_handler.h` to include additional fields like:

- Packet type
- Hop count  
- Time-to-live (TTL)
- Route information
- Sequence numbers

### Step 5: Handle Different Packet Types

MeshCore likely supports different packet types (data, ack, route discovery, etc.). Add handlers for each:

```cpp
void handleMeshPacket(uint32_t fromId, uint32_t toId, uint8_t* payload, 
                     size_t length, int16_t rssi, float snr, uint8_t packetType) {
    
    switch (packetType) {
        case PACKET_TYPE_DATA:
            handleDataPacket(fromId, toId, payload, length, rssi, snr);
            break;
            
        case PACKET_TYPE_ACK:
            handleAckPacket(fromId, toId, payload, length);
            break;
            
        case PACKET_TYPE_ROUTE_DISCOVERY:
            handleRouteDiscovery(fromId, toId, payload, length);
            break;
            
        default:
            Serial.printf("Unknown packet type: %d\n", packetType);
            break;
    }
}
```

### Step 6: Implement Node Tracking

Track active nodes in the mesh and publish their status:

```cpp
#include <map>

struct NodeInfo {
    char name[32];
    unsigned long lastSeen;
    int16_t lastRssi;
    float lastSnr;
};

std::map<uint32_t, NodeInfo> activeNodes;

void updateNodeInfo(uint32_t nodeId, const char* name, int16_t rssi, float snr) {
    NodeInfo& node = activeNodes[nodeId];
    strncpy(node.name, name, sizeof(node.name) - 1);
    node.lastSeen = millis();
    node.lastRssi = rssi;
    node.lastSnr = snr;
    
    // Publish to MQTT
    if (mqttHandler && mqttHandler->isConnected()) {
        mqttHandler->publishNodeInfo(nodeId, name, true);
    }
}

void checkNodeTimeout() {
    unsigned long now = millis();
    for (auto& pair : activeNodes) {
        if (now - pair.second.lastSeen > 300000) {  // 5 minutes
            if (mqttHandler && mqttHandler->isConnected()) {
                mqttHandler->publishNodeInfo(pair.first, pair.second.name, false);
            }
        }
    }
}
```

### Step 7: Add Route Information

If MeshCore provides route information, publish it to MQTT:

```cpp
void publishRouteInfo(uint32_t destId, uint32_t nextHop, uint8_t hopCount) {
    if (!mqttHandler || !mqttHandler->isConnected()) return;
    
    char topic[128];
    snprintf(topic, sizeof(topic), "%s/routes/%08X", 
             config.mqtt.topicPrefix, destId);
    
    StaticJsonDocument<256> doc;
    doc["destination"] = destId;
    doc["nextHop"] = nextHop;
    doc["hopCount"] = hopCount;
    doc["timestamp"] = millis();
    doc["gateway"] = config.mqtt.clientId;
    
    String output;
    serializeJson(doc, output);
    
    mqttHandler->publish(topic, output.c_str());
}
```

## Testing the Integration

### 1. Basic Connectivity Test
```cpp
// In setup(), after initialization:
Serial.println("\n=== Testing MeshCore ===");
Serial.printf("Node ID: 0x%08X\n", mesh.getNodeId());
Serial.printf("Is Repeater: %s\n", mesh.isRepeater() ? "Yes" : "No");
```

### 2. Send Test Packet
```cpp
// Add a test command in checkSerialInput()
case 't':
case 'T':
    {
        const char* testMsg = "Test from MQTT Gateway";
        sendLoRaPacket((uint8_t*)testMsg, strlen(testMsg));
    }
    break;
```

### 3. Monitor MQTT
```bash
# Subscribe to all topics
mosquitto_sub -h your-broker -t "meshcore/#" -v

# Or use MQTT Explorer GUI tool
```

### 4. Test Repeater Function
- Set up multiple MeshCore nodes
- Send messages between distant nodes
- Verify gateway forwards packets
- Check hop count increases

## Common Issues and Solutions

### Issue: Radio Won't Initialize
**Solution**: 
- Double-check pin definitions
- Verify SPI wiring
- Check radio power supply
- Try lowering SPI clock speed

### Issue: Packets Not Being Repeated
**Solution**:
- Verify repeater mode is enabled
- Check max hop count setting
- Ensure sync word matches other nodes
- Verify frequency and bandwidth match

### Issue: High Memory Usage
**Solution**:
- Reduce MQTT buffer size
- Limit number of tracked nodes
- Decrease JSON document sizes
- Enable PSRAM if available

### Issue: Missed Packets
**Solution**:
- Increase processing speed in loop()
- Remove delays in packet handlers
- Use interrupts for radio events
- Optimize MQTT publishing

## Advanced Features

### Packet Filtering
```cpp
bool shouldForwardPacket(uint32_t fromId, uint32_t toId, uint8_t packetType) {
    // Don't forward packets from ourselves
    if (fromId == config.repeater.nodeId) {
        return false;
    }
    
    // Only forward data packets to MQTT
    if (packetType != PACKET_TYPE_DATA) {
        return false;
    }
    
    // Add custom filtering logic here
    return true;
}
```

### Packet Queuing
```cpp
#include <queue>

struct QueuedPacket {
    uint32_t destId;
    uint8_t data[256];
    size_t length;
    unsigned long queueTime;
};

std::queue<QueuedPacket> sendQueue;

void queuePacket(uint32_t destId, const uint8_t* data, size_t length) {
    if (sendQueue.size() >= 10) {
        Serial.println("Send queue full!");
        return;
    }
    
    QueuedPacket packet;
    packet.destId = destId;
    packet.length = min(length, sizeof(packet.data));
    memcpy(packet.data, data, packet.length);
    packet.queueTime = millis();
    
    sendQueue.push(packet);
}

void processSendQueue() {
    if (sendQueue.empty()) return;
    
    QueuedPacket& packet = sendQueue.front();
    if (mesh.canSend()) {
        sendLoRaPacket(packet.data, packet.length);
        sendQueue.pop();
    }
}
```

## Reference Documentation

Make sure to read:
- MeshCore library documentation
- Simple_repeater example code  
- RadioLib documentation (if used)
- ESP32 Arduino core docs

## Support

If you encounter issues:
1. Check MeshCore Discord for help
2. Review MeshCore GitHub issues
3. Compare with working simple_repeater example
4. Enable debug logging to trace packet flow

---

Good luck with your integration! The MeshCore community is friendly and helpful if you need assistance.

