#pip install fastapi uvicorn influxdb-client paho-mqtt
from fastapi import FastAPI
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
import os
from mqtt import mqtt_hub
import json
from fastapi import FastAPI, HTTPException

#COMANDO PER AVVIARE IL SERVER: uvicorn app:app --reload --host 127.0.0.1 --port 8000

#Req web app
# App per lab Iot per microcontrollore esp2286
# comandi di Start and stop (sleep) per esp2286 con anche quanto tempo esp2286 deve andare in sleep
# un bel grafico linebar che mostri i dati di influxdb
# poter impostare delle trashhold a seconda della piante e poterle madnare a un serve MQTT e poi esp2286 se le pulla
# nella web app voglio selezionare la pianta e vedere le soglie 
# i valori pullati dall'influx db sono temperatura umidita quanta luce c'è i miei grafiici e le treshhold sono su questi valori

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
    # 2. Invia il file index.html come risposta alla richiesta base
    return FileResponse('index.html')

@app.get("/api/piante/{nome_pianta}")
async def get_soglie_pianta(nome_pianta: str):
    pianta = PLANT_DATABASE.get(nome_pianta)
    
    if pianta:
        # Restituiamo solo i limiti (thresholds) come richiesto
        return pianta
    
    # Se la pianta non esiste, restituiamo un errore 404
    raise HTTPException(status_code=404, detail="Pianta non trovata nel database")