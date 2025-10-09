#!/usr/bin/env python3
"""
MeshCore MQTT Gateway - Test Client

This script demonstrates how to interact with the MeshCore MQTT Gateway.
It can subscribe to messages, send commands, and display statistics.

Requirements:
    pip install paho-mqtt

Usage:
    python mqtt_test_client.py --broker mqtt.example.com --topic-prefix meshcore
"""

import argparse
import json
import time
import paho.mqtt.client as mqtt
from datetime import datetime

class MeshCoreClient:
    def __init__(self, broker, port=1883, username=None, password=None, topic_prefix="meshcore"):
        self.broker = broker
        self.port = port
        self.username = username
        self.password = password
        self.topic_prefix = topic_prefix
        self.client = None
        
    def on_connect(self, client, userdata, flags, rc):
        if rc == 0:
            print(f"✓ Connected to MQTT broker: {self.broker}")
            
            # Subscribe to all relevant topics
            topics = [
                f"{self.topic_prefix}/messages",
                f"{self.topic_prefix}/raw",
                f"{self.topic_prefix}/gateway/+/status",
                f"{self.topic_prefix}/gateway/+/stats",
                f"{self.topic_prefix}/nodes/#",
            ]
            
            for topic in topics:
                client.subscribe(topic)
                print(f"  Subscribed to: {topic}")
        else:
            print(f"✗ Connection failed with code: {rc}")
    
    def on_message(self, client, userdata, msg):
        timestamp = datetime.now().strftime("%H:%M:%S")
        
        try:
            payload = json.loads(msg.payload.decode())
            
            if "/messages" in msg.topic:
                self.handle_message(payload, timestamp)
            elif "/raw" in msg.topic:
                self.handle_raw_packet(payload, timestamp)
            elif "/status" in msg.topic:
                self.handle_gateway_status(payload, timestamp)
            elif "/stats" in msg.topic:
                self.handle_gateway_stats(payload, timestamp)
            elif "/nodes/" in msg.topic:
                self.handle_node_info(payload, timestamp)
                
        except json.JSONDecodeError:
            print(f"[{timestamp}] Non-JSON message on {msg.topic}: {msg.payload}")
        except Exception as e:
            print(f"[{timestamp}] Error processing message: {e}")
    
    def handle_message(self, payload, timestamp):
        from_id = payload.get('from', 'Unknown')
        to_id = payload.get('to', 'Unknown')
        message = payload.get('message', '')
        rssi = payload.get('rssi', 0)
        snr = payload.get('snr', 0)
        hops = payload.get('hops', 0)
        
        print(f"\n[{timestamp}] 📨 MESH MESSAGE")
        print(f"  From:    0x{from_id:08X}")
        print(f"  To:      0x{to_id:08X}")
        print(f"  Message: {message}")
        print(f"  RSSI:    {rssi} dBm")
        print(f"  SNR:     {snr:.1f} dB")
        print(f"  Hops:    {hops}")
    
    def handle_raw_packet(self, payload, timestamp):
        data = payload.get('data', '')
        length = payload.get('length', 0)
        rssi = payload.get('rssi', 0)
        snr = payload.get('snr', 0)
        
        print(f"\n[{timestamp}] 📡 RAW PACKET")
        print(f"  Data:   {data[:64]}{'...' if len(data) > 64 else ''}")
        print(f"  Length: {length} bytes")
        print(f"  RSSI:   {rssi} dBm")
        print(f"  SNR:    {snr:.1f} dB")
    
    def handle_gateway_status(self, payload, timestamp):
        online = payload.get('online', False)
        ip = payload.get('ip', 'Unknown')
        gateway_rssi = payload.get('rssi', 0)
        
        status = "🟢 ONLINE" if online else "🔴 OFFLINE"
        print(f"\n[{timestamp}] {status}")
        print(f"  IP:   {ip}")
        print(f"  RSSI: {gateway_rssi} dBm")
    
    def handle_gateway_stats(self, payload, timestamp):
        uptime = payload.get('uptime', 0)
        received = payload.get('packetsReceived', 0)
        sent = payload.get('packetsSent', 0)
        forwarded = payload.get('packetsForwarded', 0)
        failed = payload.get('packetsFailed', 0)
        
        hours = uptime // 3600
        minutes = (uptime % 3600) // 60
        
        print(f"\n[{timestamp}] 📊 GATEWAY STATS")
        print(f"  Uptime:    {hours}h {minutes}m")
        print(f"  Received:  {received}")
        print(f"  Sent:      {sent}")
        print(f"  Forwarded: {forwarded}")
        print(f"  Failed:    {failed}")
    
    def handle_node_info(self, payload, timestamp):
        node_id = payload.get('nodeId', 'Unknown')
        name = payload.get('name', 'Unknown')
        online = payload.get('online', False)
        
        status = "🟢" if online else "🔴"
        print(f"\n[{timestamp}] {status} NODE: {name} (0x{node_id:08X})")
    
    def send_message(self, message, dest_id=0xFFFFFFFF):
        """Send a message to the mesh network"""
        topic = f"{self.topic_prefix}/commands/send"
        
        # Create a simple packet (you may need to adjust format)
        payload = {
            "to": dest_id,
            "message": message
        }
        
        self.client.publish(topic, json.dumps(payload))
        print(f"✓ Sent message to 0x{dest_id:08X}: {message}")
    
    def restart_gateway(self, gateway_id=""):
        """Restart a gateway"""
        topic = f"{self.topic_prefix}/commands/restart"
        self.client.publish(topic, "restart")
        print("✓ Restart command sent")
    
    def run(self):
        """Start the MQTT client"""
        self.client = mqtt.Client()
        self.client.on_connect = self.on_connect
        self.client.on_message = self.on_message
        
        if self.username:
            self.client.username_pw_set(self.username, self.password)
        
        try:
            self.client.connect(self.broker, self.port, 60)
            print(f"\nConnecting to {self.broker}:{self.port}...")
            print("Press Ctrl+C to exit\n")
            self.client.loop_forever()
        except KeyboardInterrupt:
            print("\n\nDisconnecting...")
            self.client.disconnect()
        except Exception as e:
            print(f"Error: {e}")

def main():
    parser = argparse.ArgumentParser(description="MeshCore MQTT Gateway Test Client")
    parser.add_argument("--broker", required=True, help="MQTT broker address")
    parser.add_argument("--port", type=int, default=1883, help="MQTT broker port")
    parser.add_argument("--username", help="MQTT username")
    parser.add_argument("--password", help="MQTT password")
    parser.add_argument("--topic-prefix", default="meshcore", help="MQTT topic prefix")
    parser.add_argument("--send", help="Send a test message")
    parser.add_argument("--dest", default="0xFFFFFFFF", help="Destination node ID (hex)")
    
    args = parser.parse_args()
    
    client = MeshCoreClient(
        broker=args.broker,
        port=args.port,
        username=args.username,
        password=args.password,
        topic_prefix=args.topic_prefix
    )
    
    if args.send:
        # Send mode: connect, send message, and exit
        client.client = mqtt.Client()
        if args.username:
            client.client.username_pw_set(args.username, args.password)
        client.client.connect(args.broker, args.port, 60)
        client.client.loop_start()
        time.sleep(1)  # Wait for connection
        
        dest_id = int(args.dest, 16)
        client.send_message(args.send, dest_id)
        time.sleep(1)  # Wait for publish
        client.client.disconnect()
    else:
        # Monitor mode: subscribe and display messages
        client.run()

if __name__ == "__main__":
    main()

