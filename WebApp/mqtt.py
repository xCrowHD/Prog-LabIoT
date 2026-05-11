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
        # print(data)
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
        
        current_id = sorted(list(self.esp_list))[self.current_esp_index]
        d = {
            "id": current_id,
            "status": self.esp_list[current_id]["status"]
        }

        return d
    
    def get_next_esp(self):
        if len(self.esp_list) == 0:
            return None

        self.current_esp_index += 1
        if self.current_esp_index >= len(self.esp_list):
            self.current_esp_index = 0

        current_id = sorted(list(self.esp_list))[self.current_esp_index]
        d = {
            "id": current_id,
            "status": self.esp_list[current_id]["status"]
        }
        return d
    
    def send_thresholds(self, payload: dict):
        esp_id = payload.get("id")
        print(payload)
        self.esp_list[esp_id]["plant-thr"] = payload
        # print(self.esp_list.get(esp_id))
        self.client.publish(TOPIC_SET_ESP_THRESHOLD, json.dumps(payload), 1)
    def send_start_stop(self, payload: dict):
        esp_id = payload.get("id")
        print(payload)
        self.esp_list[esp_id]["start-stop"] = payload
        # print(self.esp_list.get(esp_id))
        self.client.publish(TOPIC_SET_ESP_START_STOP, json.dumps(payload), 1)
    def send_set_mcu(self, payload: dict):
        esp_id_target = payload.get("id")
        new_name = payload.get("name")
        is_backup = payload.get("backup", False)
        print(payload)

        for node_id, node_data in self.esp_list.items():
            if node_id == esp_id_target:
                continue

            existing_settings = node_data.get("settings")
            if existing_settings != None:
                existing_name = existing_settings.get("name")

                if existing_name == new_name:
                # Se quello che stiamo settando NON è un backup, blocchiamo tutto
                    if not is_backup:
                        print(f"ERRORE: Il nome '{new_name}' è già in uso da un nodo attivo!")
                        return

        print("Tutto Apposto nel settare MCU")
        self.esp_list[esp_id_target]["settings"] = payload
        # print(self.esp_list.get(esp_id))
        self.client.publish(TOPIC_SET_MCU, json.dumps(payload), 1)


    

mqtt_hub = MQTTManager()

