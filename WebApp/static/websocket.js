/* =============================================================
   websocket.js
   Manages the real-time WebSocket connection to the backend.
   No DOM access here — fires a callback when a message arrives.
   ============================================================= */

let socket = null;

/**
 * Inizializza la connessione WebSocket.
 * @param {Function} onMessageCallback - La funzione UI da eseguire quando arriva un allarme
 */
export function initWebSocket(onMessageCallback) {
  const protocol = window.location.protocol === "https:" ? "wss:" : "ws:";
  const host = window.location.host; // Es: "127.0.0.1:8000" o "192.168.1.50:8000"

  socket = new WebSocket(`${protocol}//${host}/ws/sicurezza`);

  socket.onopen = function (e) {
    console.log("[WS] Connessione stabilita con il backend!");
  };

  socket.onmessage = function (event) {
    try {
      const data = JSON.parse(event.data);
      console.log("[WS] Allarme ricevuto dal backend:", data);

      // Eseguiamo la funzione di callback passandogli i dati ricevuti
      if (onMessageCallback) {
        onMessageCallback(data);
      }
    } catch (err) {
      console.error("[WS] Errore nel parsing dei dati:", err);
    }
  };

  socket.onclose = function (event) {
    console.warn("[WS] Connessione chiusa. Tento il riavvio tra 5 secondi...");
    setTimeout(() => initWebSocket(onMessageCallback), 5000); // Riconnessione automatica
  };

  socket.onerror = function (error) {
    console.error("[WS] Errore socket:", error);
  };
}
