"""
mqtt.py
MQTT client manager.
Handles node discovery, backup failover, and message dispatch.
"""

import json
from paho.mqtt import client as mqtt_client
from db.plants_db import plant_db_manager
from db.node_settings_db import settings_db_manager
import asyncio

from config import (
    MQTT_IP, MQTT_PORT,
    TOPIC_TEST, TOPIC_SET_MCU, TOPIC_CONNECTION,
    TOPIC_SET_THRESHOLD, TOPIC_SET_START_STOP,
    TOPIC_BACKUP, TOPIC_TOPICS, TOPIC_SECURITY
)


class MQTTManager:
    def __init__(self):
        self.esp_list:dict[str, dict] = {}
        self.current_esp_index: int = 0
        self.callback_sicurezza = None
        self.loop_principale = None
        self.client = mqtt_client.Client(mqtt_client.CallbackAPIVersion.VERSION2)

    def start(self):
        """Fa partire il client MQTT solo quando il server è pronto"""
        self.client.connect(MQTT_IP, MQTT_PORT, keepalive=60)
        self.client.subscribe(TOPIC_CONNECTION)
        self.client.subscribe(TOPIC_SECURITY)
        self.client.message_callback_add(TOPIC_CONNECTION, self._on_node_status)
        self.client.message_callback_add(TOPIC_SECURITY, self._on_security_message)
        self.client.publish(TOPIC_TEST, "MQTT Client ON")
        self.client.loop_start()
        print("[MQTT] Client connesso e loop avviato correttamente!")

    # ── Internal callbacks ────────────────────────────────────────────────────

    def _on_node_status(self, client, userdata, msg):
        data = json.loads(msg.payload.decode())
        esp_id = data.get("id")
        status = data.get("status")
        esp_type = data.get("type")

        if esp_id not in self.esp_list:
            self.esp_list[esp_id] = {}
        self.esp_list[esp_id]["status"] = status

        settings_db_manager.ensure_node_exists(esp_id)
        node_db = settings_db_manager.get_node_settings_by_id(esp_id)

        if status == "CONNECTING":
            self._send_dynamic_topics_list(esp_id, esp_type)
        

        if status == "ONLINE":
            if esp_type == "SECURITY_SENSOR":
                return
            
            self._restore_node_state(esp_id) # Ripristina soglie/timer

            if node_db and node_db.name:
                if not node_db.is_backup:
                    # Sono un MAIN: metto il mio backup in standby
                    self._set_backup_standby(esp_id, in_standby=True)
                else:
                    # Sono un BACKUP: devo attivarmi o stare in standby?
                    main_id = self._get_partner_node(node_db.name, node_db.id)
                    
                    # Se il main è ONLINE, io sto in standby. Altrimenti mi attivo.
                    should_standby = (main_id is not None and self.esp_list.get(main_id, {}).get("status") == "ONLINE")
                    
                    msg = {"id": esp_id, "standby": should_standby}
                    self.client.publish(TOPIC_BACKUP, json.dumps(msg), qos=1)
                    print(f"[MQTT] Backup {esp_id} checked partner: standby={should_standby}")

        elif status == "OFFLINE":
            if esp_type == "SECURITY_SENSOR":
                return
            
            if node_db and not node_db.is_backup:
                # Se un Main muore, svegliamo il backup
                self._set_backup_standby(esp_id, in_standby=False)
        
        print(self.esp_list)

    def _on_security_message(self, client, userdata, msg):
        """Scatta quando il nuovo sensore invia dati su incendio o collisione"""
        try:
            payload = json.loads(msg.payload.decode())
            print(f"[MQTT] Ricevuto messaggio sicurezza: {payload}")
            
            # Se la callback è attiva e abbiamo il riferimento al loop di Uvicorn
            if self.callback_sicurezza and self.loop_principale:
                # Spediamo la coroutine direttamente al loop di Uvicorn in totale sicurezza
                asyncio.run_coroutine_threadsafe(
                    self.callback_sicurezza(payload), 
                    self.loop_principale
                )
        except Exception as e:
            print(f"[MQTT] Errore nel parsing del messaggio di sicurezza: {e}")

    # ── Internal helper functions ────────────────────────────────────────────────────
    def _send_dynamic_topics_list(self, esp_id: str, esp_type: str):

        topics_list = {"id": esp_id}

        if esp_type == "PLANT_SENSOR":
            topics_list["backup"] = TOPIC_BACKUP
            topics_list["settings"] = TOPIC_SET_MCU
            topics_list["startstop"] = TOPIC_SET_START_STOP
            topics_list["syncplant"] = TOPIC_SET_THRESHOLD
        if esp_type == "SECURITY_SENSOR":
            topics_list["security"] = TOPIC_SECURITY
            topics_list["settings"] = TOPIC_SET_MCU

        self.client.publish(TOPIC_TOPICS, json.dumps(topics_list), qos=1)

    def _restore_node_state(self, esp_id: str):
        """
        Recupera le impostazioni dal DB e le re-invia al nodo 
        che è appena tornato online.
        """
        print(f"[MQTT] Ripristino stato per il nodo: {esp_id}")
        
        # Recupera i dati dal Database
        node_db = settings_db_manager.get_node_settings_by_id(esp_id)
        
        if not node_db:
            print(f"[MQTT] Nessuna configurazione salvata per {esp_id}. Salto il ripristino.")
            return

        # Ripristino START/STOP (is_running)
        if node_db.is_running is not None:
            self.send_start_stop(esp_id, node_db.is_running)

        # Ripristino SOGLIE PIANTA (plant_id)
        if node_db.plant_id:
            self.send_thresholds(esp_id, node_db.plant_id)

        # Ripristino CONFIGURAZIONE MCU (name, timer, backup)
        # Inviato se il nome è stato configurato
        if node_db.name is not None and node_db.timer is not None:
            self.send_set_mcu(esp_id, node_db.name, node_db.is_backup, node_db.timer)
            
        print(f"[MQTT] Ripristino completato per {esp_id}")

    def _set_backup_standby(self, primary_id: str, *, in_standby: bool):
        """
        Trova il backup nel DB basandosi sul nome del primario (primary_id)
        e invia il comando di standby/wake-up.
        """
        # Recuperiamo i dati del primario dal DB per conoscerne il nome
        primary_db = settings_db_manager.get_node_settings_by_id(primary_id)
        
        if not primary_db or not primary_db.name:
            # Se il primario non ha un nome, non possiamo trovare il suo backup
            return

        primary_name = primary_db.name

        # Cerchiamo se esiste un backup con lo stesso nome nel DB
        backup_node = settings_db_manager.get_backup_node_by_name(primary_name)

        if backup_node:
            # Prepariamo il comando MQTT
            # Nota: backup_node.id è il MAC address del backup
            msg = {"id": backup_node.id, "standby": in_standby}
            
            self.client.publish(TOPIC_BACKUP, json.dumps(msg), qos=1)
            
            state = "STANDBY (Sleep)" if in_standby else "ACTIVE (Wake Up)"
            print(f"[MQTT] Failover: Backup {backup_node.id} per '{primary_name}' → {state}")
        else:
            # Opzionale: log se non esiste un backup per questo nodo
            print(f"[MQTT] Nessun backup configurato per il nodo '{primary_name}'")
            pass

    def _node_name_already_taken(self, target_id: str, new_name: str, is_backup: bool) -> bool:
        """
        Verifica nel DB se esiste un conflitto di nomi tra nodi Main.
        """

        if is_backup:
            return False

        # 2. Cerchiamo nel DB tutti i nodi con lo stesso nome
        existing_nodes = settings_db_manager.get_nodes_by_name(new_name)

        for node in existing_nodes:
            # Se il nodo trovato è lo stesso che stiamo configurando, non è un conflitto
            if node.id == target_id:
                continue
            
            # Se troviamo un ALTRO nodo con lo stesso nome che NON è un backup,
            # allora abbiamo un conflitto tra due Main.
            if not node.is_backup:
                print(f"[MQTT] Conflitto: Il nome '{new_name}' è già usato dal Main {node.id}")
                return True

        # Nessun conflitto trovato
        return False

    def _get_partner_node(self, name: str, current_id: str) -> str | None:
        """
        Ritorna l'ID del nodo Main associato al nome fornito, 
        escludendo l'ID del nodo che effettua la richiesta.
        """
        # Cerchiamo nel DB il nodo Main con quel nome
        main_id = settings_db_manager.get_main_id_by_name(name)
        
        # Restituiamo l'ID trovato solo se non è quello attuale
        if main_id and main_id != current_id:
            return main_id
            
        return None
    
    # ── Public API ────────────────────────────────────────────────────────────

    def get_current_esp(self) -> dict | None:
        if not self.esp_list:
            return None
        esp_id = sorted(self.esp_list)[self.current_esp_index]
        return {"id": esp_id, "status": self.esp_list[esp_id]["status"]}

    def get_next_esp(self) -> dict | None:
        if not self.esp_list:
            return None
        self.current_esp_index = (self.current_esp_index + 1) % len(self.esp_list)
        esp_id = sorted(self.esp_list)[self.current_esp_index]
        return {"id": esp_id, "status": self.esp_list[esp_id]["status"]}

    def send_thresholds(self, node_id: str, plant_id: str):
        """
        Recupera le soglie dal DB e le invia al nodo specifico.
        """
        plant = plant_db_manager.get_plant_by_id(plant_id)

        if not plant:
            print(f"[MQTT] Errore: Pianta '{plant_id}' non trovata nel DB.")
            return

        payload = {
            "id": node_id,
            "name": plant_id,
            "thresholds": {
                "temp": {"min": plant.temp_min, "max": plant.temp_max},
                "hum": {"min": plant.hum_min, "max": plant.hum_max},
                "light": {"min": plant.light_min, "max": plant.light_max}
            }
        }
        
        settings_db_manager.update_node_plant(node_id, plant_id)

        self.client.publish(TOPIC_SET_THRESHOLD, json.dumps(payload), qos=1)
        print(f"[MQTT] Soglie inviate a {node_id} per la pianta {plant.name}")

    def send_start_stop(self, node_id: str, is_running: bool):

        payload = {
            "id": node_id,
            "status": is_running
        }
        settings_db_manager.update_node_running_state(node_id, is_running)
        self.client.publish(TOPIC_SET_START_STOP, json.dumps(payload), qos=1)

    def send_set_mcu(self, node_id: str, name: str, is_backup: bool, timer: float):
        if self._node_name_already_taken(node_id, name, is_backup):
            print(f"[MQTT] ERROR: name '{name}' already in use by a non-backup node")
            return

        settings_db_manager.update_node_settings(node_id, name, is_backup, timer)

        if is_backup:
            # Se sto diventando un backup, controllo se il mio (nuovo) main è online
            main_id = self._get_partner_node(name, node_id)
            main_status = self.esp_list.get(main_id, {}).get("status") if main_id else None
            should_standby = (main_status == "ONLINE")
        else:
            # SE DIVENTO UN MAIN, DEVO SVEGLIARMI SEMPRE
            # (Risolve il tuo problema del nodo che rimane in standby)
            should_standby = False

        standby_msg = {"id": node_id, "standby": should_standby}
        self.client.publish(TOPIC_BACKUP, json.dumps(standby_msg), qos=1)
        print(f"[MQTT] Standby impostato per {node_id} -> {should_standby}")

        payload = {
            "id": node_id,
            "name": name,
            "backup": is_backup,
            "timer": timer
        }
        self.client.publish(TOPIC_SET_MCU, json.dumps(payload), qos=1)

    # ── Callback set for websocket ──────────────────────────────────
    def set_security_callback(self, callback_func):
        """Permette a app.py di registrare la propria funzione WebSocket"""
        self.callback_sicurezza = callback_func
        # CATTURA IL LOOP QUI: Essendo chiamata da app.py (Lifespan/Main), 
        # siamo al 100% nel thread e nel loop di Uvicorn!
        self.loop_principale = asyncio.get_running_loop()
        print("[MQTT] Callback e Loop principale registrati con successo!")

mqtt_hub = MQTTManager()