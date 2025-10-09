# MeshCore MQTT Gateway

A powerful gateway firmware that bridges MeshCore LoRa mesh networks with MQTT brokers, enabling bidirectional message flow between LoRa devices and MQTT-based applications.

## 🌟 Features

- **Full MeshCore Repeater Functionality** - Acts as a mesh network repeater, extending network range
- **MQTT Bridging** - Forward LoRa mesh messages to MQTT broker and vice versa
- **Serial Configuration** - Easy-to-use serial menu for configuring all settings
- **WiFi Connectivity** - Connect to your local network
- **Persistent Configuration** - Settings stored in ESP32 NVS (non-volatile storage)
- **Real-time Statistics** - Monitor packets, uptime, and connection status
- **Multiple Publishing Modes** - Publish raw hex packets and/or decoded messages
- **Remote Commands** - Send commands from MQTT to the gateway
- **Gateway Status Reporting** - Automatic online/offline status with last will

## 📋 Requirements

### Hardware
- ESP32-based LoRa board (Heltec v3, generic ESP32+SX127x, etc.)
- LoRa radio module (SX1276/SX1278 or compatible)
- USB cable for programming and serial configuration

### Software
- [PlatformIO](https://platformio.org/) (recommended) or Arduino IDE
- [MeshCore Library](https://github.com/meshcore-dev/MeshCore)

## 🚀 Quick Start

### 1. Clone and Open Project

```bash
git clone <your-repo-url>
cd MeshCore-MQTT-Gateway
```

Open in VS Code with PlatformIO extension installed.

### 2. Configure Hardware

Edit `platformio.ini` to select your target board:

```ini
[platformio]
default_envs = esp32_mqtt_gateway  ; or heltec_v3_mqtt
```

If using a custom board, adjust pin definitions in `src/config.h`:

```cpp
#define LORA_SCK 5
#define LORA_MISO 19
#define LORA_MOSI 27
// ... etc
```

### 3. Build and Upload

```bash
pio run --target upload
```

Or use the PlatformIO upload button in VS Code.

### 4. Initial Configuration

1. Open serial monitor (115200 baud)
2. Press `c` to enter configuration menu
3. Configure WiFi settings (option 1)
4. Configure MQTT settings (option 2)
5. Configure LoRa settings (option 3)
6. Configure Repeater settings (option 4)
7. Save configuration (option 6)
8. Restart device (option 8)

## ⚙️ Configuration

### Serial Configuration Menu

The gateway provides an interactive serial configuration menu accessible via USB:

**Main Menu Options:**
- `1` - WiFi Settings (SSID, password, enable/disable)
- `2` - MQTT Settings (broker, port, credentials, topics)
- `3` - LoRa Settings (frequency, bandwidth, spreading factor, etc.)
- `4` - Repeater Settings (node name, max hops, timeouts)
- `5` - Show Current Configuration
- `6` - Save Configuration
- `7` - Reset to Defaults
- `8` - Restart Device
- `0` - Exit Configuration

**Runtime Commands:**
- `c` - Enter configuration menu
- `s` - Show statistics
- `r` - Restart device

### Configuration Structure

#### WiFi Configuration
```
SSID: your-wifi-ssid
Password: your-wifi-password
Enabled: yes
```

#### MQTT Configuration
```
Server: mqtt.example.com (or IP address)
Port: 1883 (or 8883 for TLS)
Username: (optional)
Password: (optional)
Client ID: meshcore_gateway_001
Topic Prefix: meshcore
Publish Raw: yes
Publish Decoded: yes
Subscribe Commands: yes
```

#### LoRa Configuration
```
Frequency: 915.0 MHz (US) or 868.0 MHz (EU) or 433.0 MHz
Bandwidth: 125.0 kHz
Spreading Factor: 7-12 (7=fastest, 12=longest range)
Coding Rate: 5-8
TX Power: 2-20 dBm
Sync Word: 0x12
Enable CRC: yes
```

#### Repeater Configuration
```
Node Name: MQTT-Gateway
Node ID: 0x12345678 (auto-generated from chip ID)
Max Hops: 3
Auto ACK: yes
Broadcast Enabled: yes
Route Timeout: 300 seconds
```

## 📡 MQTT Topics

### Published Topics

#### Raw Packets
Topic: `{prefix}/raw`

```json
{
  "timestamp": 12345678,
  "rssi": -85,
  "snr": 8.5,
  "data": "0102030405...",
  "length": 32
}
```

#### Decoded Messages
Topic: `{prefix}/messages`

```json
{
  "timestamp": 12345678,
  "from": 305419896,
  "to": 305419897,
  "message": "Hello from LoRa!",
  "type": 1,
  "rssi": -85,
  "snr": 8.5,
  "hops": 2,
  "gateway": "meshcore_gateway_001"
}
```

#### Node Information
Topic: `{prefix}/nodes/{nodeId}`

```json
{
  "nodeId": 305419896,
  "name": "Node-001",
  "online": true,
  "timestamp": 12345678,
  "gateway": "meshcore_gateway_001"
}
```

#### Gateway Statistics
Topic: `{prefix}/gateway/{clientId}/stats`

```json
{
  "timestamp": 12345678,
  "uptime": 3600,
  "packetsReceived": 1250,
  "packetsSent": 45,
  "packetsForwarded": 1180,
  "packetsFailed": 5,
  "rssi": -65,
  "freeHeap": 156000
}
```

#### Gateway Status (Retained)
Topic: `{prefix}/gateway/{clientId}/status`

```json
{
  "online": true,
  "timestamp": 12345678,
  "ip": "192.168.1.100",
  "rssi": -65
}
```

### Subscribed Topics

#### Send Command
Topic: `{prefix}/commands/send`

Payload: Raw bytes or JSON message to send via LoRa

#### Restart Command
Topic: `{prefix}/commands/restart`

Payload: (any) - Triggers gateway restart

## 🔧 Integration with MeshCore

The current implementation includes placeholder comments where MeshCore integration should be added. Follow these steps to complete the integration:

### 1. Include MeshCore Headers

In `src/main.cpp`, uncomment and adjust:
```cpp
#include <MeshCore.h>
```

### 2. Initialize MeshCore

In the `setupLoRa()` function, add MeshCore initialization based on the actual library API:

```cpp
void setupLoRa() {
    SPI.begin(LORA_SCK, LORA_MISO, LORA_MOSI, LORA_CS);
    
    // Initialize MeshCore with your specific API calls
    MeshCore.begin();
    MeshCore.setNodeId(config.repeater.nodeId);
    MeshCore.setNodeName(config.repeater.nodeName);
    // ... etc
}
```

### 3. Handle Incoming Packets

In the `handleLoRaReceive()` function:

```cpp
void handleLoRaReceive() {
    if (MeshCore.available()) {
        uint8_t buffer[256];
        size_t length = MeshCore.receive(buffer, sizeof(buffer));
        int rssi = MeshCore.getLastRssi();
        float snr = MeshCore.getLastSnr();
        
        packetsReceived++;
        handleLoRaPacket(buffer, length, rssi, snr);
    }
}
```

### 4. Send Packets

In the `sendLoRaPacket()` function:

```cpp
void sendLoRaPacket(const uint8_t* data, size_t length) {
    if (MeshCore.send(data, length)) {
        packetsSent++;
    } else {
        packetsFailed++;
    }
}
```

## 📊 Monitoring

### Serial Monitor
- Real-time packet logging
- Connection status
- Error messages
- Statistics display

### MQTT Dashboard
Subscribe to topics to monitor:
- `meshcore/gateway/+/status` - All gateway statuses
- `meshcore/messages` - All mesh messages
- `meshcore/gateway/+/stats` - Gateway statistics

Example using `mosquitto_sub`:
```bash
mosquitto_sub -h mqtt.example.com -t "meshcore/#" -v
```

## 🛠️ Troubleshooting

### WiFi Not Connecting
- Check SSID and password in configuration
- Ensure 2.4GHz WiFi (ESP32 doesn't support 5GHz)
- Check signal strength
- Try restarting the gateway

### MQTT Not Connecting
- Verify broker address and port
- Check username/password if required
- Ensure broker allows connections from your network
- Check firewall rules

### No LoRa Packets Received
- Verify LoRa frequency matches your region and other nodes
- Check antenna connection
- Verify spreading factor and bandwidth match mesh network
- Ensure sync word matches (default: 0x12)

### Configuration Not Saving
- Check serial output for error messages
- Try "Reset to Defaults" then reconfigure
- Ensure adequate power supply

## 🔐 Security Considerations

- **MQTT Credentials**: Store credentials securely, consider using TLS/SSL
- **Network Security**: Use VPN or secure network for MQTT traffic
- **Access Control**: Implement MQTT ACLs to restrict topic access
- **Firmware Updates**: Keep firmware updated for security patches

## 📝 Example MQTT Publishers

### Python Example

```python
import paho.mqtt.client as mqtt
import json

client = mqtt.Client("controller")
client.username_pw_set("user", "password")
client.connect("mqtt.example.com", 1883)

# Send message to LoRa mesh
message = b"\x01\x02\x03\x04"  # Your packet data
client.publish("meshcore/commands/send", message)

# Subscribe to messages
def on_message(client, userdata, msg):
    data = json.loads(msg.payload)
    print(f"Message from node {data['from']}: {data['message']}")

client.subscribe("meshcore/messages")
client.on_message = on_message
client.loop_forever()
```

### Node-RED Flow

Import this flow to visualize mesh traffic:

```json
[
    {
        "id": "mqtt-in",
        "type": "mqtt in",
        "topic": "meshcore/messages",
        "broker": "mqtt-broker"
    },
    {
        "id": "debug",
        "type": "debug",
        "name": "Mesh Messages"
    }
]
```

## 🤝 Contributing

Contributions are welcome! Please:
1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Test thoroughly
5. Submit a pull request

## 📄 License

This project is licensed under the MIT License - see LICENSE file for details.

## 🔗 Links

- [MeshCore Project](https://github.com/meshcore-dev/MeshCore)
- [MeshCore Discord](https://discord.gg/meshcore)
- [PlatformIO Documentation](https://docs.platformio.org/)
- [MQTT Documentation](https://mqtt.org/)

## 📮 Support

- Open an issue for bugs or feature requests
- Join MeshCore Discord for community support
- Check existing issues before creating new ones

## 🎯 Roadmap

- [ ] TLS/SSL support for MQTT
- [ ] Web configuration interface
- [ ] OTA (Over-The-Air) firmware updates
- [ ] Message encryption
- [ ] Multiple MQTT broker support
- [ ] Local message logging to SD card
- [ ] REST API for local control
- [ ] Prometheus metrics endpoint

---

**Made with ❤️ for the MeshCore community**

