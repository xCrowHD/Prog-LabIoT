"""
mqtt.py
MQTT client manager.
Handles node discovery, backup failover, and message dispatch.
"""

import json
from paho.mqtt import client as mqtt_client

from config import (
    MQTT_IP, MQTT_PORT,
    TOPIC_TEST, TOPIC_SET_MCU, TOPIC_CONNECTION,
    TOPIC_SET_THRESHOLD, TOPIC_SET_START_STOP, TOPIC_BACKUP,
)


class MQTTManager:
    def __init__(self):
        self.esp_list:dict[str, dict] = {}
        self.current_esp_index: int = 0

        self.client = mqtt_client.Client(mqtt_client.CallbackAPIVersion.VERSION2)
        self.client.connect(MQTT_IP, MQTT_PORT, keepalive=60)
        self.client.subscribe(TOPIC_CONNECTION)
        self.client.message_callback_add(TOPIC_CONNECTION, self._on_node_status)
        self.client.publish(TOPIC_TEST, "MQTT Client ON")
        self.client.loop_start()

    # ── Internal callbacks ────────────────────────────────────────────────────

    def _on_node_status(self, client, userdata, msg):
        """Handle connection/disconnection announcements from ESP nodes."""
        data = json.loads(msg.payload.decode())
        esp_id = data.get("id")
        status = data.get("status")  # "ONLINE" | "OFFLINE"

        # Initialise node entry if new
        if esp_id not in self.esp_list:
            self.esp_list[esp_id] = {}

        self.esp_list[esp_id]["status"] = status

        if status == "ONLINE":
            self._restore_node_state(esp_id)
            self._set_backup_standby(esp_id, in_standby=True)
        elif status == "OFFLINE":
            print(f"[MQTT] Node {esp_id} went offline — activating backup if present")
            self._set_backup_standby(esp_id, in_standby=False)

        print(f"[MQTT] {esp_id} → {status}")

    def _restore_node_state(self, esp_id: str):
        """Re-send stored configuration to a node that just came back online."""
        node = self.esp_list[esp_id]
        if node.get("plant-thr"):
            self.send_thresholds(node["plant-thr"])
        if node.get("start-stop"):
            self.send_start_stop(node["start-stop"])
        if node.get("settings"):
            self.send_set_mcu(node["settings"])

    def _set_backup_standby(self, primary_id: str, *, in_standby: bool):
        """
        Find the backup node for *primary_id* and toggle its standby state.
        in_standby=True  → backup goes to sleep (primary is back online)
        in_standby=False → backup wakes up   (primary went offline)
        """
        primary = self.esp_list.get(primary_id)
        if not primary or "settings" not in primary:
            return

        primary_name = primary["settings"].get("name")

        for node_id, node_data in self.esp_list.items():
            if node_id == primary_id:
                continue
            settings = node_data.get("settings", {})
            if settings.get("name") == primary_name and settings.get("backup") is True:
                msg    = {"id": node_id, "standby": in_standby}
                self.client.publish(TOPIC_BACKUP, json.dumps(msg), qos=1, retain=True)
                state  = "standby" if in_standby else "active"
                print(f"[MQTT] Backup {node_id} for '{primary_name}' → {state}")

    def _node_name_already_taken(self, payload: dict) -> bool:
        """Return True if another (non-backup) node already uses the same name."""
        target_id = payload.get("id")
        new_name  = payload.get("name")
        is_backup = payload.get("backup", False)

        for node_id, node_data in self.esp_list.items():
            if node_id == target_id:
                continue
            existing_name = (node_data.get("settings") or {}).get("name")
            if existing_name == new_name and not is_backup:
                return True
        return False

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

    def send_thresholds(self, payload: dict):
        esp_id = payload["id"]
        self.esp_list[esp_id]["plant-thr"] = payload
        self.client.publish(TOPIC_SET_THRESHOLD, json.dumps(payload), qos=1)

    def send_start_stop(self, payload: dict):
        esp_id = payload["id"]
        self.esp_list[esp_id]["start-stop"] = payload
        self.client.publish(TOPIC_SET_START_STOP, json.dumps(payload), qos=1)

    def send_set_mcu(self, payload: dict):
        esp_id = payload["id"]
        is_backup = payload.get("backup", False)

        if self._node_name_already_taken(payload):
            print(f"[MQTT] ERROR: name '{payload.get('name')}' already in use by a non-backup node")
            return

        standby_msg = {"id": esp_id, "standby": is_backup}
        self.client.publish(TOPIC_BACKUP, json.dumps(standby_msg), qos=1)

        self.esp_list[esp_id]["settings"] = payload
        self.client.publish(TOPIC_SET_MCU, json.dumps(payload), qos=1)


mqtt_hub = MQTTManager()