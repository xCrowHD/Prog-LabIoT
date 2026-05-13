"""
config.py
Centralised configuration: environment variables and MQTT constants.
All other modules import from here — never from os.getenv directly.
"""

import os
from dotenv import load_dotenv

load_dotenv()

# ── InfluxDB ──────────────────────────────────────────────────────────────────
INFLUXDB_TOKEN  = os.getenv("INFLUXDB_TOKEN")
INFLUXDB_ORG    = os.getenv("INFLUXDB_ORG")
INFLUXDB_BUCKET = os.getenv("INFLUXDB_BUCKET")
INFLUXDB_URL    = os.getenv("INFLUXDB_URL")

# ── MQTT ──────────────────────────────────────────────────────────────────────
MQTT_IP   = "broker.emqx.io"
MQTT_PORT = 1883

TOPIC_TEST            = "lab_iot/mafogani/test"
TOPIC_TOPICS          = "lab_iot/mafogani/topics"
TOPIC_SET_MCU         = "lab_iot/mafogani/set-mcu"
TOPIC_CONNECTION      = "lab_iot/mafogani/connection/+"
TOPIC_SET_THRESHOLD   = "lab_iot/mafogani/threshold"
TOPIC_SET_START_STOP  = "lab_iot/mafogani/start-stop"
TOPIC_BACKUP          = "lab_iot/mafogani/backup"

# ── Static files ──────────────────────────────────────────────────────────────
UPLOAD_DIR = "./static/uploads"