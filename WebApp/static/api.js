/* =============================================================
   api.js
   All network calls to the backend REST API.
   Every function returns a parsed JSON value or throws on error.
   No DOM access here — callers handle display logic.
   ============================================================= */

/** @param {Response} res */
async function _checkResponse(res) {
  if (!res.ok) {
    const body = await res.json().catch(() => ({}));
    throw new Error(body.detail ?? `HTTP ${res.status}`);
  }
  return res.json();
}

// ── Plants ────────────────────────────────────────────────────────────────────

export async function fetchPlantCount() {
  const res = await fetch("/api/plants/count");
  return _checkResponse(res); // { count: number }
}

export async function fetchPlantByPosition(pos) {
  const res = await fetch(`/api/plants/soglie/position/${pos}`);
  if (res.status === 404) return null;
  return _checkResponse(res);
}

export async function fetchPlantThresholds(plantId) {
  const res = await fetch(`/api/plants/soglie/${plantId}`);
  return _checkResponse(res);
}

export async function fetchPlantData(plantId, lastTime) {
  const res = await fetch(`/api/plants/data/${plantId}/${lastTime}`);
  return _checkResponse(res); // Array of records
}

export async function fetchLatestPlantData(plantId) {
  const res = await fetch(`/api/plants/latestdata/${plantId}`);
  return _checkResponse(res);
}

export async function syncMqttThresholds(plantId) {
  const res = await fetch(`/api/nodes/syncplant/${plantId}`);
  return _checkResponse(res);
}

/**
 * @param {{ name: string, temp_min: number, temp_max: number,
 *            hum_min: number, hum_max: number,
 *            light_min: number, light_max: number,
 *            image?: File }} plantData
 */
export async function savePlant(plantData) {
  const form = new FormData();
  form.append("name", plantData.name);
  form.append("temp_min", plantData.temp_min);
  form.append("temp_max", plantData.temp_max);
  form.append("hum_min", plantData.hum_min);
  form.append("hum_max", plantData.hum_max);
  form.append("light_min", plantData.light_min);
  form.append("light_max", plantData.light_max);
  if (plantData.image) form.append("image", plantData.image);

  const res = await fetch("/api/plants/save", { method: "POST", body: form });
  return _checkResponse(res);
}

// ── Nodes ─────────────────────────────────────────────────────────────────────

export async function fetchCurrentNode() {
  const res = await fetch("/api/nodes/currentnode");
  return _checkResponse(res);
}

export async function fetchNextNode() {
  const res = await fetch("/api/nodes/nextnode");
  return _checkResponse(res);
}

/** @param {boolean} start */
export async function sendStartStop(start) {
  const res = await fetch("/api/nodes/startstop", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify({ status: start }),
  });
  return _checkResponse(res);
}

/** @param {{ name: string, backup: boolean, timer: number }} config */
export async function sendSetMcu(config) {
  const res = await fetch("/api/nodes/set-mcu", {
    method: "POST",
    headers: { "Content-Type": "application/json" },
    body: JSON.stringify(config),
  });
  return _checkResponse(res);
}
