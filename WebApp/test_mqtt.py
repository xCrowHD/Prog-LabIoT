import json
import time
import paho.mqtt.client as mqtt

# --- CONFIGURAZIONE ---
BROKER = "broker.emqx.io"
PORT = 1883
FALSO_MAC_ESP = "485519E3BFDC" 

# I due topic usati dall'architettura
TOPIC_CONNESSIONE = "lab_iot/mafogani/connection/485519E3BFDC" # Il topic dove l'ESP dice chi è e cosa fa
TOPIC_SICUREZZA = "lab_iot/mafogani/security"    # Il topic dell'allarme

def on_connect(client, userdata, flags, rc, properties=None):
    if rc == 0:
        print("✅ Simu-ESP connesso al Broker EMQX!")
    else:
        print(f"❌ Connessione fallita: {rc}")

client = mqtt.Client(callback_api_version=mqtt.CallbackAPIVersion.VERSION2)
client.on_connect = on_connect

print(f"Connessione a {BROKER}...")
client.connect(BROKER, PORT, 60)
client.loop_start()
time.sleep(1) 

try:
    # ── STEP 1: SIMULIAMO L'AVVIO DELL'ESP (Connection/Settings payload) ──
    # Inviamo il payload esatto che dice al backend che questo dispositivo esiste
    payload_boot = {
        "id": FALSO_MAC_ESP,
        "status": "ONLINE",
        "type": "SECURITY_SENSOR",
        "actions": ["SECURITY", "SETTINGS"]
    }
    
    print(f"\n📡 [STEP 1] Fingo l'avvio dell'ESP sul topic: {TOPIC_CONNESSIONE}")
    client.publish(TOPIC_CONNESSIONE, json.dumps(payload_boot), qos=1).wait_for_publish()
    print("✅ ESP registrato nel sistema del backend!")

    # Attendiamo 3 secondi (simuliamo il tempo in cui l'ESP gira normalmente)
    print("\n⏳ Attendo 3 secondi prima di scatenare l'inferno...")
    time.sleep(3)

    # ── STEP 2: SIMULIAMO IL CAMBIO DI STATO (Allarme Fiamma/Collisione) ──
    payload_allarme_door = {
        "id": FALSO_MAC_ESP,
        "timestamp": "22/06/2026",
        "alarmType": "DOOR",
    }
    
    print(f"🚀 [STEP 2] Spedisco l'allarme sul topic: {TOPIC_SICUREZZA}")
    client.publish(TOPIC_SICUREZZA, json.dumps(payload_allarme_door), qos=1).wait_for_publish()
    print("✅ Allarme inviato!")

except Exception as e:
    print(f"❌ Errore durante la simulazione: {e}")

# Chiudiamo la connessione
time.sleep(1)
client.loop_stop()
client.disconnect()
print("\n🔌 Simulazione terminata. Sconnesso.")