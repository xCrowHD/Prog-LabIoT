from fastapi import APIRouter, HTTPException
from services.influx_service import get_combined_event_log, count_door_opens_today

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