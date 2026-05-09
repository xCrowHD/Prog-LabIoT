import paho.mqtt.client as mqtt
from paho.mqtt import client as mqtt_client
import json

TOPIC_TEST = "lab_iot/mafogani/test"
TOPIC_SET_MCU = "lab_iot/mafogani/set-mcu"
TOPIC_CONNECTION = "lab_iot/mafogani/connection/+"
TOPIC_SET_ESP_THRESHOLD = "lab_iot/mafogani/threshold"
TOPIC_SET_ESP_START_STOP = "lab_iot/mafogani/start-stop"
MQTT_IP = "broker.emqx.io"
MQTT_PORT = 1883

class MQTTManager:
    def __init__(self):
        self.is_esp_online = False
        self.esp_list = set()
        self.client = mqtt_client.Client(mqtt_client.CallbackAPIVersion.VERSION2)
        self.client.connect(MQTT_IP, MQTT_PORT, 60)
        self.client.message_callback_add(TOPIC_CONNECTION, self._esp_status_check)
        self.client.publish(TOPIC_TEST, "MQTT Client ON")
        self.client.loop_start()

    def _esp_status_check(self, client, userdata, msg):
        payload = msg.payload.decode()
        data = json.loads(payload)
        print(data)
        esp_id = data.get("id")
        self.esp_list.add(esp_id)

    
    def send_thresholds(self, payload: str):
        self.client.publish(TOPIC_SET_ESP_THRESHOLD, payload)
    def send_start_stop(self, payload: str):
        self.client.publish(TOPIC_SET_ESP_START_STOP, payload)
    def send_set_mcu(self, payload: str):
        self.client.publish(TOPIC_SET_MCU, payload)


    

mqtt_hub = MQTTManager()

