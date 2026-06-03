from fastapi import APIRouter, WebSocket, WebSocketDisconnect

router = APIRouter()

connessioni_attive: list[WebSocket] = []

@router.websocket("/ws/sicurezza")
async def websocket_endpoint(websocket: WebSocket):
    await websocket.accept()
    connessioni_attive.append(websocket)
    print(f"[WebSocket] Browser connesso alla dashboard. Totale: {len(connessioni_attive)}")
    try:
        while True:
            # Mantiene attiva la connessione
            await websocket.receive_text()
    except WebSocketDisconnect:
        connessioni_attive.remove(websocket)
        print(f"[WebSocket] Browser disconnesso. Totale: {len(connessioni_attive)}")

async def notifica_frontend_allarme(payload: dict):
    """Funzione che riceve i dati da MQTT e li spara ai browser"""
    print(f"[WebSocket] Invio allarme in tempo reale a {len(connessioni_attive)} client.")
    for ws in connessioni_attive:
        try:
            await ws.send_json(payload)
        except Exception:
            pass