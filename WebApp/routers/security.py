from fastapi import APIRouter, HTTPException
from services.influx_service import get_combined_event_log, count_door_opens_today, get_latest_door_event, get_latest_flame_event

router = APIRouter(prefix="/api/security", tags=["security"])

@router.get("/events/{device_id}/{limit}")
async def get_security_events(device_id: str, limit: int = 5):
    try:
        return get_combined_event_log(device_id, limit)
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@router.get("/stats/door-opens/{device_id}")
async def get_door_stats(device_id: str):
    try:
        return {"count": count_door_opens_today(device_id)}
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))
    
@router.get("/status/door/{device_id}")
async def get_current_door_status(device_id: str):
    """Ritorna l'ultimo stato della porta (True/False) registrato nel DB."""
    try:
        status = get_latest_door_event(device_id)
        if status is None:
            # Se il sensore non ha mai inviato dati, puoi decidere se dare 404 
            # o ritornare uno stato di default sicuro (es. porta chiusa)
            return {"doorClose": True, "msg": "Nessun dato registrato"}
        return status
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))

@router.get("/status/flame/{device_id}")
async def get_current_flame_status(device_id: str):
    """Ritorna l'ultimo stato dell'allarme incendio e l'ultima temperatura."""
    try:
        status = get_latest_flame_event(device_id)
        if status is None:
            return {"isOnFlame": False, "temp": None, "msg": "Nessun dato registrato"}
        return status
    except Exception as e:
        raise HTTPException(status_code=500, detail=str(e))