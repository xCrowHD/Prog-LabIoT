#pip install fastapi uvicorn influxdb-client paho-mqtt
from fastapi import FastAPI
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
import os
from mqtt import mqtt_hub
import json

#COMANDO PER AVVIARE IL SERVER: uvicorn app:app --reload --host 127.0.0.1 --port 8000

#Req web app
# App per lab Iot per microcontrollore esp2286
# comandi di Start and stop (sleep) per esp2286 con anche quanto tempo esp2286 deve andare in sleep
# un bel grafico linebar che mostri i dati di influxdb
# poter impostare delle trashhold a seconda della piante e poterle madnare a un serve MQTT e poi esp2286 se le pulla
# nella web app voglio selezionare la pianta e vedere le soglie 
# i valori pullati dall'influx db sono temperatura umidita quanta luce c'è i miei grafiici e le treshhold sono su questi valori

app = FastAPI()
app.mount("/static", StaticFiles(directory="static"), name="static")


@app.get("/")
async def home():
    # 2. Invia il file index.html come risposta alla richiesta base
    return FileResponse('index.html')

