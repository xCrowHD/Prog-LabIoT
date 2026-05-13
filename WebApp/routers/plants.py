"""
routers/plants.py
All routes related to plant data: thresholds, sensor readings, and persistence.
"""

import os
import shutil

from fastapi import APIRouter, HTTPException, UploadFile, File, Form
from mqtt import mqtt_hub
from db.plants_db import plant_db_manager
from services.influx_service import get_plant_data, get_latest_plant_data
from config import UPLOAD_DIR

router = APIRouter(prefix="/api/plants", tags=["plants"])


# ── Read ──────────────────────────────────────────────────────────────────────

@router.get("/count")
async def get_plant_count():
    return {"count": plant_db_manager.get_plants_count()}


@router.get("/soglie/{plant_name}")
async def get_plant_thresholds(plant_name: str):
    plant = plant_db_manager.get_plant_by_id(plant_name)
    if plant is None:
        raise HTTPException(status_code=404, detail="Pianta non trovata")
    return plant


@router.get("/soglie/position/{pos}")
async def get_plant_thresholds_by_position(pos: int):
    plant = plant_db_manager.get_plant_by_position(pos)
    if plant is None:
        return None
    return plant


@router.get("/data/{plant_name}/{last_time}")
async def get_plant_data_range(plant_name: str, last_time: str):
    try:
        return get_plant_data(plant_name, last_time)
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))


@router.get("/latestdata/{plant_name}")
async def get_latest_plant_reading(plant_name: str):
    data = get_latest_plant_data(plant_name)
    if data is None:
        raise HTTPException(status_code=404, detail="Nessun dato trovato per questa pianta")
    return data


# ── MQTT sync ─────────────────────────────────────────────────────────────────

@router.get("/syncmqtt/{plant_name}")
async def sync_mqtt_thresholds(plant_name: str):
    current_esp = mqtt_hub.get_current_esp()

    if current_esp is None:
        raise HTTPException(status_code=503, detail="Nessun nodo ESP disponibile")

    mqtt_hub.send_thresholds(current_esp["id"], plant_name)
    return {"status": "ok"}


# ── Write ─────────────────────────────────────────────────────────────────────

@router.post("/save")
async def save_plant(
    name:       str   = Form(...),
    temp_min:   float = Form(...),
    temp_max:   float = Form(...),
    hum_min:    float = Form(...),
    hum_max:    float = Form(...),
    light_min:  float = Form(...),
    light_max:  float = Form(...),
    image: UploadFile = File(None),
):
    try:
        img_path = None
        if image:
            ext          = os.path.splitext(image.filename)[1]
            safe_name    = f"{plant_db_manager.generate_id(name)}{ext}"
            file_path    = os.path.join(UPLOAD_DIR, safe_name)
            with open(file_path, "wb") as buf:
                shutil.copyfileobj(image.file, buf)
            img_path = file_path

        plant_id = plant_db_manager.add_plant(
            name=name,
            img_path=img_path,
            t_min=temp_min, t_max=temp_max,
            h_min=hum_min,  h_max=hum_max,
            l_min=light_min, l_max=light_max,
        )
        return {"status": "success", "message": f"Pianta {name} salvata", "id": plant_id}

    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))