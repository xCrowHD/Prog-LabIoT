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
        self.esp_list = set()
        # self.esp_list.add("TESTID0")
        # self.esp_list.add("TESTID1")
        # self.esp_list.add("TESTID2")
        # self.esp_list.add("TESTID3")
        self.current_esp_index = 0
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

    def get_current_esp(self):
        if len(self.esp_list) == 0:
            return None

        return sorted(list(self.esp_list))[self.current_esp_index]
    
    def get_next_esp(self):
        if len(self.esp_list) == 0:
            return None

        self.current_esp_index += 1
        if self.current_esp_index >= len(self.esp_list):
            self.current_esp_index = 0

        return sorted(list(self.esp_list))[self.current_esp_index]

    
    def send_thresholds(self, payload: str):
        self.client.publish(TOPIC_SET_ESP_THRESHOLD, payload, 1)
    def send_start_stop(self, payload: str):
        self.client.publish(TOPIC_SET_ESP_START_STOP, payload, 1)
    def send_set_mcu(self, payload: str):
        self.client.publish(TOPIC_SET_MCU, payload, 1)


    

mqtt_hub = MQTTManager()

