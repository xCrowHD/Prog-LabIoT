from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS
import time
import random

INFLUXDB_TOKEN = "***REMOVED***"
INFLUXDB_ORG = "***REMOVED***"
INFLUXDB_BUCKET = "***REMOVED***"
INFLUXDB_URL = "***REMOVED***"

client = InfluxDBClient(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG)
write_api = client.write_api(write_options=SYNCHRONOUS)

try:
    while True:
        # Generiamo dati casuali realistici per il test
        t = round(random.uniform(22.0, 26.0), 1)
        h = round(random.uniform(55.0, 65.0), 1)
        l = random.randint(300, 800)

        # Creazione del punto
        point = Point("Serra") \
            .tag("pianta", "ghost_orchid") \
            .field("temp", t) \
            .field("hum", h) \
            .field("lux", l)

        # Scrittura
        write_api.write(bucket=INFLUXDB_BUCKET, org=INFLUXDB_ORG, record=point)
        
        print(f"[{time.strftime('%H:%M:%S')}] Inviato: Temp={t}°, Hum={h}%, Lux={l}")

        # Aspetta 5 minuti (300 secondi)
        # Per testare subito, puoi mettere 5 o 10 secondi!
        time.sleep(10) 

except KeyboardInterrupt:
    print("\nMonitoraggio interrotto dall'utente.")
finally:
    client.close()