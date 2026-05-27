"""
app.py
FastAPI application entry point.
Only wires together the app, static files, and routers.

Start with:
    python -m uvicorn app:app --reload --host 127.0.0.1 --port 8000
"""

import os
from fastapi import FastAPI
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from fastapi import WebSocket, WebSocketDisconnect
from mqtt import mqtt_hub
from config import UPLOAD_DIR
from db.plants_db import plant_db_manager
from routers import plants, nodes, web_sockets

# ── Initialisation ────────────────────────────────────────────────────────────
os.makedirs(UPLOAD_DIR, exist_ok=True)
# plant_db_manager.delete_plant_by_id("test")

mqtt_hub.set_security_callback(web_sockets.notifica_frontend_allarme)
# ── App ───────────────────────────────────────────────────────────────────────
app = FastAPI(title="Chlorophyll IoT Dashboard")
app.mount("/static", StaticFiles(directory="static"), name="static")

app.include_router(plants.router)
app.include_router(nodes.router)
app.include_router(web_sockets.router)

@app.get("/")
async def home():
    return FileResponse("index.html")