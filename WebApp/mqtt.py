import paho.mqtt.client as mqtt
from paho.mqtt import client as mqtt_client

# Monstera Deliciosa, Nepenthes Rajah, Dendrophylax lindenii

TOPIC_TEST = "lab_iot/mafogani/test"
TOPIC_IS_ESP_ONLINE = "lab_iot/mafogani/esp_online"
TOPIC_SET_ESP_THRESHOLD = "lab_iot/mafogani/threshold"
MQTT_IP = "broker.emqx.io"
MQTT_PORT = 1883

class MQTTManager:
    def __init__(self):
        self.is_esp_online = False

        self.client = mqtt_client.Client(mqtt_client.CallbackAPIVersion.VERSION2)
        self.client.connect(MQTT_IP, MQTT_PORT, 60)
        self.client.message_callback_add(TOPIC_IS_ESP_ONLINE, self.esp_status_check)
        self.client.publish(TOPIC_TEST, "MQTT Client ON")
        self.client.loop_start()

    def esp_status_check(self, client, userdata, msg):
        payload = msg.payload.decode()
        # Supponiamo che l'ESP mandi "online" o "offline"
        self.is_esp_online = (payload.lower() == "online")
        print(f"[MQTT] Stato ESP aggiornato: {self.is_esp_online}")

    

mqtt_hub = MQTTManager()

