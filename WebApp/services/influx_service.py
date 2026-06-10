"""
services/influx_service.py
All InfluxDB interactions in one place.
The FastAPI routes call these functions and never touch the InfluxDB client directly.
"""

from influxdb_client import InfluxDBClient
from config import INFLUXDB_URL, INFLUXDB_TOKEN, INFLUXDB_ORG, INFLUXDB_BUCKET
from utils.sensors import adc_to_klux


def _build_client() -> InfluxDBClient:
    return InfluxDBClient(url=INFLUXDB_URL, token=INFLUXDB_TOKEN, org=INFLUXDB_ORG)


def _plant_record_to_dict(record) -> dict:
    """Shared serialiser: converts a single Flux record into a plain dict."""
    lux_raw = record.values.get("lux")
    return {
        "timestamp": record.get_time().strftime("%d/%m/%Y %H:%M"),
        "pianta":    record.values.get("pianta"),
        "temp":      record.values.get("temp"),
        "hum":       record.values.get("hum"),
        "lux":       lux_raw,
        "klux":      adc_to_klux(float(lux_raw)) if lux_raw is not None else 0.0,
    }

def _alarm_record_to_dict(record) -> dict:
    """Serializza i record unificati di DoorAlarm e FlameAlarm."""
    measurement = record.values.get("_measurement")
    
    # Struttura base condivisa
    data = {
        "timestamp": record.get_time().strftime("%d/%m/%Y %H:%M:%S"),
        "alarmType":      measurement,
        "device":    record.values.get("device"),
    }
    
    # Logica specifica in base al tipo di allarme
    if measurement == "DoorAlarm":
        data["doorClose"] = record.values.get("doorClose")
        
    elif measurement == "FlameAlarm":
        temp = record.values.get("temp")
        is_flame = record.values.get("isOnFlame")
        data["temp"] = temp
        data["isOnFlame"] = is_flame
        
    return data

def get_plant_data(plant_id: str, last_time: str) -> list[dict]:
    """Return time-series records for *plant_id* over the *last_time* window."""
    query = f"""
    from(bucket: "{INFLUXDB_BUCKET}")
    |> range(start: -{last_time})
    |> filter(fn: (r) => r["_measurement"] == "Serra")
    |> filter(fn: (r) => r["pianta"] == "{plant_id}")
    |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")
    |> group()
    |> sort(columns: ["_time"], desc: false)
    """
    client = _build_client()
    try:
        results = client.query_api().query(org=INFLUXDB_ORG, query=query)
        return [_plant_record_to_dict(r) for table in results for r in table.records]
    finally:
        client.close()


def get_latest_plant_data(plant_id: str) -> dict | None:
    """Return the single most-recent record for *plant_id*, or None."""
    query = f"""
    from(bucket: "{INFLUXDB_BUCKET}")
    |> range(start: 0)
    |> filter(fn: (r) => r["_measurement"] == "Serra")
    |> filter(fn: (r) => r["pianta"] == "{plant_id}")
    |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")
    |> group()
    |> sort(columns: ["_time"], desc: true)
    |> limit(n: 1)
    """
    client = _build_client()
    try:
        results = client.query_api().query(org=INFLUXDB_ORG, query=query)
        if not results or not results[0].records:
            return None
        return _plant_record_to_dict(results[0].records[0])
    finally:
        client.close()

def get_combined_event_log(device_id: str, limit: int = 5) -> list[dict]:
    query = f"""
    door = from(bucket: "{INFLUXDB_BUCKET}")
        |> range(start: 0)
        |> filter(fn: (r) => r["_measurement"] == "DoorAlarm")
        |> filter(fn: (r) => r["device"] == "{device_id}")
        |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")

    flame = from(bucket: "{INFLUXDB_BUCKET}")
        |> range(start: 0)
        |> filter(fn: (r) => r["_measurement"] == "FlameAlarm")
        |> filter(fn: (r) => r["device"] == "{device_id}")
        |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")

    union(tables: [door, flame])
        |> group()
        |> sort(columns: ["_time"], desc: true)
        |> limit(n: {limit})
    """
    client = _build_client()
    try:
        results = client.query_api().query(org=INFLUXDB_ORG, query=query)
        return [_alarm_record_to_dict(r) for table in results for r in table.records]
    finally:
        client.close()

def count_door_opens_today(device_id: str) -> int:
    """Conta quante volte la porta è stata aperta (doorClose = false) oggi."""

    query = f"""
    from(bucket: "{INFLUXDB_BUCKET}")
      |> range(start: today())
      |> filter(fn: (r) => r["_measurement"] == "DoorAlarm")
      |> filter(fn: (r) => r["device"] == "{device_id}")
      |> filter(fn: (r) => r["_field"] == "doorClose")
      |> filter(fn: (r) => r["_value"] == false)
      |> count()
    """
    
    client = _build_client()
    try:
        results = client.query_api().query(org=INFLUXDB_ORG, query=query)
        
        # Se non ci sono aperture, la query non ritorna righe, quindi controlliamo
        if not results or not results[0].records:
            return 0
            
        # Il valore del count è nel campo _value
        return results[0].records[0].get_value()
    finally:
        client.close()

def get_latest_door_event(device_id: str) -> dict | None:
    """Ritorna l'ultimo record in assoluto (più recente) per il DoorAlarm di un dispositivo."""

    query = f"""
    from(bucket: "{INFLUXDB_BUCKET}")
    |> range(start: 0)
    |> filter(fn: (r) => r["_measurement"] == "DoorAlarm")
    |> filter(fn: (r) => r["device"] == "{device_id}")
    |> filter(fn: (r) => r["_field"] == "doorClose")
    |> last()
    |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")
    """
    
    client = _build_client()
    try:
        results = client.query_api().query(org=INFLUXDB_ORG, query=query)
        
        # Se il dispositivo non ha mai inviato dati nel range temporale, ritorniamo None
        if not results or not results[0].records:
            return None
            
        # Prendiamo il record e lo passiamo al serializzatore degli allarmi
        record = results[0].records[0]
        return _alarm_record_to_dict(record)
        
    finally:
        client.close()

def get_latest_flame_event(device_id: str) -> dict | None:
    """Ritorna l'ultimo record in assoluto per il FlameAlarm (con isOnFlame e temp)."""
    
    query = f"""
    from(bucket: "{INFLUXDB_BUCKET}")
      |> range(start: 0)
      |> filter(fn: (r) => r["_measurement"] == "FlameAlarm")
      |> filter(fn: (r) => r["device"] == "{device_id}")
      |> pivot(rowKey:["_time"], columnKey: ["_field"], valueColumn: "_value")
      |> group()
      |> sort(columns: ["_time"], desc: true)
      |> limit(n: 1)
    """
    
    client = _build_client()
    try:
        results = client.query_api().query(org=INFLUXDB_ORG, query=query)
        
        if not results or not results[0].records:
            return None
            
        record = results[0].records[0]
        return _alarm_record_to_dict(record)
    finally:
        client.close()