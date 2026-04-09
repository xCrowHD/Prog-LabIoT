import paho.mqtt.client as mqtt
from paho.mqtt import client as mqtt_client
TOPIC = "lab_iot/mafogani/#"

def on_message(client, userdata, msg):
    print(f"MESSAGGIO RICEVUTO! Topic: {msg.topic} - Contenuto: {msg.payload.decode()}")

client = mqtt.Client(mqtt_client.CallbackAPIVersion.VERSION2)
client.on_message = on_message
client.connect("broker.emqx.io", 1883, 60)
client.subscribe(TOPIC)

print(f"In ascolto sul topic: {TOPIC}...")
client.loop_forever()