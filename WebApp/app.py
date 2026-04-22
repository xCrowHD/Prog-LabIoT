from fastapi import FastAPI
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
import os
from mqtt import mqtt_hub
import json
from influxdb_client import InfluxDBClient
from fastapi import FastAPI, HTTPException
from dotenv import load_dotenv

# 1. Carica il file .env
load_dotenv()

#COMANDO PER AVVIARE IL SERVER: python -m uvicorn app:app --reload --host 127.0.0.1 --port 8000

#Req web app
# App per lab Iot per microcontrollore esp2286
# comandi di Start and stop (sleep) per esp2286 con anche quanto tempo esp2286 deve andare in sleep
# un bel grafico linebar che mostri i dati di influxdb
# poter impostare delle trashhold a seconda della piante e poterle madnare a un serve MQTT e poi esp2286 se le pulla
# nella web app voglio selezionare la pianta e vedere le soglie 
# i valori pullati dall'influx db sono temperatura umidita quanta luce c'è i miei grafiici e le treshhold sono su questi valori

INFLUXDB_TOKEN = os.getenv("INFLUXDB_TOKEN")
INFLUXDB_ORG = os.getenv("INFLUXDB_ORG")
INFLUXDB_BUCKET = os.getenv("INFLUXDB_BUCKET")
INFLUXDB_URL = os.getenv("INFLUXDB_URL")

PLANT_DATABASE = {
    "monstera_albo": {
        "name": "Monstera Deliciosa",
        "img": "./static/P1.jpeg",
        "thresholds": {
            "temp": {"min": 18.0, "max": 27.0},
            "hum": {"min": 60.0, "max": 75.0},
            "light": {"min": 300, "max": 500} 
        }
    },
    "nepenthes_rajah": {
        "name": "Nepenthes Rajah",
        "img": "./static/P2.jpeg",
        "thresholds": {
            "temp": {"min": 13.0, "max": 25.0},
            "hum": {"min": 80.0, "max": 95.0},
            "light": {"min": 200, "max": 400}
        }
    },
    "ghost_orchid": {
        "name": "Dendrophylax lindenii",
        "img": "./static/P3.jpeg",
        "thresholds": {
            "temp": {"min": 22.0, "max": 32.0},
            "hum": {"min": 70.0, "max": 90.0},
            "light": {"min": 600, "max": 850}
        }
    }
}

app = FastAPI()
app.mount("/static", StaticFiles(directory="static"), name="static")

@app.get("/")
async def home():
    return FileResponse('index.html')


@app.get("/api/piante/soglie/{nome_pianta}")
async def get_soglie_pianta(nome_pianta: str):
    pianta = PLANT_DATABASE.get(nome_pianta)
    
    if pianta:
        return pianta
    

@app.get("/api/piante/syncmqtt/{nome_pianta}")
async def sync_mqtt(nome_pianta: str):
    pianta = PLANT_DATABASE.get(nome_pianta)
    
    if pianta:
        payload = {
            "name": nome_pianta,
            "thresholds": pianta.get("thresholds")
        }
        json_string = json.dumps(payload)
        mqtt_hub.send_thresholds(json_string)

@app.get("/api/piante/startstop/{esp_status}")
async def sync_mqtt(esp_status: str):
    payload = str(esp_status)
    mqtt_hub.send_start_stop(payload)


@app.get("/api/piante/data/{nome_pianta}/{last_time}")
async def get_data_pianta(nome_pianta: str, last_time:str):
    client = InfluxDBClient(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG)
    query_api = client.query_api()
    query = f'''
    from(bucket: "{INFLUXDB_BUCKET}")
    |> range(start: -{last_time})
    |> filter(fn: (r) => r["_measurement"] == "Serra")
    |> filter(fn: (r) => r["pianta"] == "{nome_pianta}")
    |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")
    |> group()
    |> sort(columns: ["_time"], desc: false)
    '''

    try:
        
        results = query_api.query(org=INFLUXDB_ORG, query=query)
        lista_punti = []
        # Influx restituisce i dati organizzati in 'tabelle' (una per ogni campo/serie)

        for table in results:
            for record in table.records:
                punto = {
                    "timestamp": record.get_time().strftime('%d/%m/%Y %H:%M'),
                    "pianta": record["pianta"],
                    "temp": record.values.get("temp"),
                    "hum": record.values.get("hum"),
                    "lux": record.values.get("lux"),
                    # Se vuoi già i kLux convertiti, usa la tua funzione qui:
                    "klux": _adc_to_klux(float(record.values.get("lux"))) if record.values.get("lux") is not None else 0
                }
                lista_punti.append(punto)

        return lista_punti

    except Exception as e:
        print(f"Errore durante la query: {e}")
    finally:
        client.close()


@app.get("/api/piante/latestdata/{nome_pianta}")
async def get_latest_data_pianta(nome_pianta: str):
    client = InfluxDBClient(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG)
    query_api = client.query_api()
    query = f'''
    from(bucket: "{INFLUXDB_BUCKET}")
    |> range(start: 0)
    |> filter(fn: (r) => r["_measurement"] == "Serra")
    |> filter(fn: (r) => r["pianta"] == "{nome_pianta}")
    |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")
    |> group() 
    |> sort(columns: ["_time"], desc: true)
    |> limit(n: 1)
    '''

    try:
        results = query_api.query(org=INFLUXDB_ORG, query=query)
        if not results or len(results) == 0:
            return {"status": "error", "message": "Nessun dato trovato per questa pianta"}
        record = results[0].records[0]
        
        # Costruiamo il dizionario da restituire come JSON
        data = {
            "timestamp": record.get_time().strftime('%d/%m/%Y %H:%M'),
            "pianta": record.values.get("pianta"),
            "temp": record.values.get("temp"),
            "hum": record.values.get("hum"),
            "lux": record.values.get("lux"),
            # Se vuoi già i kLux convertiti, usa la tua funzione qui:
            "klux": _adc_to_klux(float(record.values.get("lux"))) if record.values.get("lux") is not None else 0
        }
        
        return data

    except Exception as e:
        print(f"Errore durante la query: {e}")
    finally:
        client.close()

def _adc_to_klux(adc_value):
    if adc_value <= 0: return 0
    if adc_value >= 1023: return 1000 # Evitiamo errori di calcolo al limite
    
    # 1. Calcoliamo la tensione (assumendo 3.3V se usi ESP8266 o 5V se usi Arduino)
    # Cambia 3.3 in 5.0 se alimenti il sensore a 5V
    v_out = (adc_value * 3.3) / 1024.0
    
    # 2. Calcoliamo la resistenza della fotoresistenza (R_LDR)
    # Il modulo KY-018 ha una resistenza fissa R_fixed = 10k ohm
    # Se con molta luce il valore ADC è ALTO:
    r_ldr = (10000.0 * (3.3 - v_out)) / v_out
    
    # 3. Formula empirica per trasformare la Resistenza in Lux
    # Molte LDR seguono la curva Lux = 500 / (R_LDR in kOhm)
    # Nota: il valore 1000 serve per passare da Ohm a kOhm
    lux = 500 / (r_ldr / 1000.0)
    
    return round(lux / 1000, 2)