import paho.mqtt.client as mqtt
from paho.mqtt import client as mqtt_client
import json

TOPIC_TEST = "lab_iot/mafogani/test"
TOPIC_SET_MCU = "lab_iot/mafogani/set-mcu"
TOPIC_CONNECTION = "lab_iot/mafogani/connection/+"
TOPIC_SET_ESP_THRESHOLD = "lab_iot/mafogani/threshold"
TOPIC_SET_ESP_START_STOP = "lab_iot/mafogani/start-stop"
TOPIC_BACKUP = "lab_iot/mafogani/backup"
MQTT_IP = "broker.emqx.io"
MQTT_PORT = 1883

class MQTTManager:
    def __init__(self):
        self.esp_list = dict()
        # self.esp_list.add("TESTID0")
        # self.esp_list.add("TESTID1")
        # self.esp_list.add("TESTID2")
        # self.esp_list.add("TESTID3")
        self.current_esp_index = 0
        self.client = mqtt_client.Client(mqtt_client.CallbackAPIVersion.VERSION2)
        self.client.connect(MQTT_IP, MQTT_PORT, 60)
        self.client.subscribe(TOPIC_CONNECTION)
        self.client.message_callback_add(TOPIC_CONNECTION, self._esp_status_check)
        self.client.publish(TOPIC_TEST, "MQTT Client ON")
        self.client.loop_start()

    def _esp_status_check(self, client, userdata, msg):
        payload = msg.payload.decode()
        data = json.loads(payload)
        print(data)
        esp_id = data.get("id")

        if self.esp_list.get(esp_id) == None:
            self.esp_list[esp_id] = {}   
        else:
            esp = self.esp_list[esp_id]
            if esp.get("plant-thr") != None:
                self.send_thresholds(esp.get("plant-thr"))
                print(esp.get("plant-thr"))
            if esp.get("start-stop") != None:
                self.send_start_stop(esp.get("start-stop"))
            if esp.get("settings") != None:
                self.send_set_mcu(esp.get("settings"))

        self.esp_list[esp_id]["status"] = data.get("status")
        print(self.esp_list)

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

    
    def send_thresholds(self, payload: dict):
        esp_id = payload.get("id")
        self.esp_list[esp_id]["plant-thr"] = payload
        # print(self.esp_list.get(esp_id))
        self.client.publish(TOPIC_SET_ESP_THRESHOLD, json.dumps(payload), 1)
    def send_start_stop(self, payload: dict):
        esp_id = payload.get("id")
        self.esp_list[esp_id]["start-stop"] = payload
        # print(self.esp_list.get(esp_id))
        self.client.publish(TOPIC_SET_ESP_START_STOP, json.dumps(payload), 1)
    def send_set_mcu(self, payload: dict):
        esp_id = payload.get("id")
        self.esp_list[esp_id]["settings"] = payload
        # print(self.esp_list.get(esp_id))
        self.client.publish(TOPIC_SET_MCU, json.dumps(payload), 1)


    

mqtt_hub = MQTTManager()

