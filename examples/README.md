# MeshCore MQTT Gateway Examples

This directory contains example scripts and code for interacting with the MeshCore MQTT Gateway.

## mqtt_test_client.py

A Python-based MQTT client for testing and monitoring the gateway.

### Installation

```bash
pip install paho-mqtt
```

### Usage

**Monitor all messages:**
```bash
python mqtt_test_client.py --broker mqtt.example.com --topic-prefix meshcore
```

**With authentication:**
```bash
python mqtt_test_client.py \
    --broker mqtt.example.com \
    --username myuser \
    --password mypass \
    --topic-prefix meshcore
```

**Send a test message:**
```bash
# Broadcast message
python mqtt_test_client.py \
    --broker mqtt.example.com \
    --send "Hello MeshCore!" \
    --dest 0xFFFFFFFF

# Send to specific node
python mqtt_test_client.py \
    --broker mqtt.example.com \
    --send "Direct message" \
    --dest 0x12345678
```

## MQTT Topic Examples

### Subscribe to all messages
```bash
mosquitto_sub -h mqtt.example.com -t "meshcore/#" -v
```

### Subscribe to specific topics
```bash
# Only mesh messages
mosquitto_sub -h mqtt.example.com -t "meshcore/messages"

# Only gateway status
mosquitto_sub -h mqtt.example.com -t "meshcore/gateway/+/status"

# Only node information
mosquitto_sub -h mqtt.example.com -t "meshcore/nodes/#"
```

### Publish commands
```bash
# Send a message
mosquitto_pub -h mqtt.example.com \
    -t "meshcore/commands/send" \
    -m '{"to": 4294967295, "message": "Test"}'

# Restart gateway
mosquitto_pub -h mqtt.example.com \
    -t "meshcore/commands/restart" \
    -m "restart"
```

## Node-RED Example

Import this flow into Node-RED to create a simple dashboard:

```json
[
    {
        "id": "mqtt-in-messages",
        "type": "mqtt in",
        "topic": "meshcore/messages",
        "broker": "mqtt-broker",
        "name": "Mesh Messages"
    },
    {
        "id": "format-message",
        "type": "function",
        "func": "msg.payload = `From: ${msg.payload.from}\\nMessage: ${msg.payload.message}\\nRSSI: ${msg.payload.rssi}`;\\nreturn msg;"
    },
    {
        "id": "display",
        "type": "debug",
        "name": "Display Messages"
    }
]
```

## Home Assistant Integration

Add to your `configuration.yaml`:

```yaml
sensor:
  - platform: mqtt
    name: "MeshCore Gateway Status"
    state_topic: "meshcore/gateway/meshcore_gateway_001/status"
    value_template: "{{ 'Online' if value_json.online else 'Offline' }}"
    json_attributes_topic: "meshcore/gateway/meshcore_gateway_001/status"
    
  - platform: mqtt
    name: "MeshCore Packets Received"
    state_topic: "meshcore/gateway/meshcore_gateway_001/stats"
    value_template: "{{ value_json.packetsReceived }}"
    unit_of_measurement: "packets"

automation:
  - alias: "Alert on MeshCore Gateway Offline"
    trigger:
      platform: mqtt
      topic: "meshcore/gateway/meshcore_gateway_001/status"
    condition:
      condition: template
      value_template: "{{ trigger.payload_json.online == false }}"
    action:
      service: notify.mobile_app
      data:
        message: "MeshCore Gateway is offline!"
```

## Python Integration Example

```python
import paho.mqtt.client as mqtt
import json

def on_message(client, userdata, msg):
    data = json.loads(msg.payload)
    print(f"Node {data['from']}: {data['message']}")

client = mqtt.Client()
client.on_message = on_message
client.connect("mqtt.example.com", 1883)
client.subscribe("meshcore/messages")
client.loop_start()

# Send a message after 2 seconds
import time
time.sleep(2)

message = {
    "to": 0xFFFFFFFF,  # Broadcast
    "message": "Hello from Python!"
}
client.publish("meshcore/commands/send", json.dumps(message))

# Keep running
try:
    while True:
        time.sleep(1)
except KeyboardInterrupt:
    client.disconnect()
```

## JavaScript/Node.js Example

```javascript
const mqtt = require('mqtt');

const client = mqtt.connect('mqtt://mqtt.example.com:1883');

client.on('connect', () => {
    console.log('Connected to MQTT broker');
    client.subscribe('meshcore/messages');
    client.subscribe('meshcore/gateway/+/status');
});

client.on('message', (topic, message) => {
    const data = JSON.parse(message.toString());
    
    if (topic.includes('/messages')) {
        console.log(`Message from ${data.from}: ${data.message}`);
        console.log(`RSSI: ${data.rssi} dBm, SNR: ${data.snr} dB`);
    } else if (topic.includes('/status')) {
        console.log(`Gateway ${data.online ? 'online' : 'offline'}`);
    }
});

// Send a test message
setTimeout(() => {
    const message = {
        to: 0xFFFFFFFF,
        message: "Hello from Node.js!"
    };
    client.publish('meshcore/commands/send', JSON.stringify(message));
}, 2000);
```

## More Examples

Check the [MeshCore repository](https://github.com/meshcore-dev/MeshCore) for more examples and documentation.

