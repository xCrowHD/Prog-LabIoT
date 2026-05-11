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
        esp_id = data.get("id")
        current_status = data.get("status") # ONLINE o OFFLINE

        # 1. Inizializzazione se il nodo è nuovo
        if self.esp_list.get(esp_id) is None:
            self.esp_list[esp_id] = {}

        # Aggiorniamo lo stato nel dizionario
        self.esp_list[esp_id]["status"] = current_status

        # 2. LOGICA SE IL NODO TORNA ONLINE
        if current_status == "ONLINE":
            esp = self.esp_list[esp_id]
            
            # RIPRISTINO PARAMETRI: Se abbiamo dati salvati, inviamoli subito all'ESP
            if esp.get("plant-thr"):
                self.send_thresholds(esp.get("plant-thr"))
            if esp.get("start-stop"):
                self.send_start_stop(esp.get("start-stop"))
            if esp.get("settings"):
                self.send_set_mcu(esp.get("settings"))
            
            # GESTIONE BACKUP: Se questo nodo è il "principale", cerchiamo il suo backup e mettiamolo in standby
            # (Assumiamo che il backup abbia lo stesso nome ma il flag backup=True)
            self._manage_backup_for(esp_id, activate_backup=False)

        # 3. LOGICA SE IL NODO VA OFFLINE (Last Will)
        elif current_status == "OFFLINE":
            print(f"ATTENZIONE: Nodo {esp_id} disconnesso! Cerco un sostituto...")
            # Attiviamo il backup se esiste
            self._manage_backup_for(esp_id, activate_backup=True)

        print(f"Stato aggiornato: {esp_id} è ora {current_status}")

    def _manage_backup_for(self, target_id, activate_backup: bool):
        """
        Trova il backup del nodo 'target_id' e lo attiva o lo mette in standby.
        """
        target_node = self.esp_list.get(target_id)
        if not target_node or "settings" not in target_node:
            return

        target_name = target_node["settings"].get("name")

        # Cerchiamo tra tutti gli altri nodi
        for other_id, other_data in self.esp_list.items():
            if other_id == target_id:
                continue
                
            settings = other_data.get("settings")
            if settings and settings.get("name") == target_name and settings.get("backup") is True:
                # Abbiamo trovato il backup!
                status_msg = {"id": other_id, "standby": not activate_backup}
                
                # Se activate_backup è True, standby sarà False (sveglia!)
                # Se activate_backup è False, standby sarà True (nanna!)
                self.client.publish(f"{TOPIC_BACKUP}/{other_id}", json.dumps(status_msg), 1, retain=True)
                
                azione = "ATTIVATO" if activate_backup else "messo in STANDBY"
                print(f"Il backup {other_id} per {target_name} è stato {azione}")

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

    def _does_node_exist(self, payload: dict):
        esp_id_target = payload.get("id")
        new_name = payload.get("name")
        is_backup = payload.get("backup", False)
        for node_id, node_data in self.esp_list.items():
            if node_id == esp_id_target:
                continue

            existing_settings = node_data.get("settings")
            if existing_settings != None:
                existing_name = existing_settings.get("name")

                if existing_name == new_name:
                # Se quello che stiamo settando NON è un backup, blocchiamo tutto
                    if not is_backup:
                        return True
        return False
    
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
        esp_id = payload.get("id")
        is_backup = payload.get("backup", False)

        if self._does_node_exist(payload):
            print(f"ERRORE: Nome già in uso e non è un backup!")
            return 

        if is_backup:
            d = {"id": esp_id, "standby": True}
            self.client.publish(TOPIC_BACKUP, json.dumps(d), 1)
        else:
            d = {"id": esp_id, "standby": False}
            self.client.publish(TOPIC_BACKUP, json.dumps(d), 1)

        self.esp_list[esp_id]["settings"] = payload
        self.client.publish(TOPIC_SET_MCU, json.dumps(payload), 1)


    

mqtt_hub = MQTTManager()

