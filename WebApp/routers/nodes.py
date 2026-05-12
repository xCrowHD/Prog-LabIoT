"""
routers/nodes.py
Routes for managing ESP8266 nodes: start/stop, MCU configuration, and node navigation.
"""

from fastapi import APIRouter, HTTPException
from mqtt import mqtt_hub

router = APIRouter(prefix="/api/nodes", tags=["nodes"])


@router.get("/currentnode")
async def get_current_node():
    node = mqtt_hub.get_current_esp()
    if node is None:
        raise HTTPException(status_code=503, detail="Nessun nodo disponibile")
    return node


@router.get("/nextnode")
async def get_next_node():
    node = mqtt_hub.get_next_esp()
    if node is None:
        raise HTTPException(status_code=503, detail="Nessun nodo disponibile")
    return node


@router.post("/startstop")
async def start_stop_esp(data: dict):
    current_esp = mqtt_hub.get_current_esp()
    if current_esp is None:
        raise HTTPException(status_code=503, detail="Nessun nodo disponibile")

    payload = {
        "id":     current_esp["id"],
        "status": data.get("status", False),
    }
    mqtt_hub.send_start_stop(payload)
    return {"message": "Comando inviato", "target": current_esp, "status": payload["status"]}


@router.post("/set-mcu")
async def set_mcu(payload: dict):
    current_esp = mqtt_hub.get_current_esp()
    if current_esp is None:
        raise HTTPException(status_code=503, detail="Nessun nodo disponibile")

    payload["id"] = current_esp["id"]
    mqtt_hub.send_set_mcu(payload)
    return {"status": "ok", "target": current_esp["id"]}