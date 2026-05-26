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


def _record_to_dict(record) -> dict:
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
        return [_record_to_dict(r) for table in results for r in table.records]
    finally:
        client.close()


def get_latest_plant_data(plant_name: str) -> dict | None:
    """Return the single most-recent record for *plant_name*, or None."""
    query = f"""
    from(bucket: "{INFLUXDB_BUCKET}")
    |> range(start: 0)
    |> filter(fn: (r) => r["_measurement"] == "Serra")
    |> filter(fn: (r) => r["pianta"] == "{plant_name}")
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
        return _record_to_dict(results[0].records[0])
    finally:
        client.close()