"""
app.py
FastAPI application entry point.
Only wires together the app, static files, and routers.

Start with:
    python -m uvicorn app:app --reload --host 127.0.0.1 --port 8000
"""

import os
from fastapi import FastAPI
from contextlib import asynccontextmanager
from fastapi.responses import FileResponse
from fastapi.staticfiles import StaticFiles
from fastapi import WebSocket, WebSocketDisconnect
from mqtt import mqtt_hub
from config import UPLOAD_DIR
from db.plants_db import plant_db_manager
from routers import plants, nodes, web_sockets, security
from BotSerra.bot import start_bot

# ── Initialisation ────────────────────────────────────────────────────────────
os.makedirs(UPLOAD_DIR, exist_ok=True)
# plant_db_manager.delete_plant_by_id("test")

@asynccontextmanager
async def lifespan(app: FastAPI):
    # 1. Configura la callback e cattura il loop principale di Uvicorn
    mqtt_hub.set_security_callback(web_sockets.notifica_frontend_allarme)
    
    # 2. Avvia la connessione MQTT adesso che tutto è pronto e asincrono
    mqtt_hub.start()
    bot_app = await start_bot()
    
    yield # Qui l'applicazione gira normalmente...
    await bot_app.stop()
    await bot_app.shutdown()
    mqtt_hub.client.loop_stop()
    mqtt_hub.client.disconnect()

# ── App ───────────────────────────────────────────────────────────────────────
app = FastAPI(title="Chlorophyll IoT Dashboard", lifespan=lifespan)

app.mount("/static", StaticFiles(directory="static"), name="static")

app.include_router(plants.router)
app.include_router(nodes.router)
app.include_router(web_sockets.router)
app.include_router(security.router)

@app.get("/")
async def home():
    return FileResponse("index.html")


if __name__ == "__main__":
    import uvicorn
    uvicorn.run("app:app", host="127.0.0.1", port=8000, reload=True)