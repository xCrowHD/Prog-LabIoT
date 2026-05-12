/* =============================================================
   chart.js
   Bar-chart rendering and axis helpers.
   Depends on: api.js
   ============================================================= */

import { fetchPlantData } from "./api.js";

/** Maps a sensor field to its Tailwind colour token. */
const FIELD_COLOR = {
  temp: "bg-primary",
  hum: "bg-secondary",
  lux: "bg-tertiary",
};

/**
 * Fetch data for *plantId* and render the bar chart.
 * @param {string} plantId
 * @param {"temp"|"hum"|"lux"} field
 * @param {string} lastTime  e.g. "24h", "7d"
 */
export async function renderPlantChart(plantId, field, lastTime) {
  const container = document.getElementById("chart-bars");
  if (!container) return;

  try {
    const data = await fetchPlantData(plantId, lastTime);
    const fieldData = data.map((r) => r[field]);
    const colorClass = FIELD_COLOR[field] ?? "bg-primary";

    if (fieldData.length === 0) {
      container.innerHTML = "";
      document.getElementById("y-max").textContent = "No Data";
      document.getElementById("y-mid").textContent = "No Data";
      _updateXAxis(data);
      return;
    }

    const max = Math.max(...fieldData);
    _updateYAxis(fieldData);
    _updateXAxis(data);

    container.innerHTML = "";
    fieldData.forEach((value) => {
      const bar = document.createElement("div");
      bar.className = [
        "flex-1",
        `${colorClass}/20`,
        "rounded-t-sm",
        `hover:${colorClass}`,
        "transition-all",
        "duration-300",
      ].join(" ");
      // Inline style required — Tailwind JIT cannot resolve dynamic h-[] at runtime
      bar.style.height = `${(value / max) * 100}%`;
      container.appendChild(bar);
    });
  } catch (err) {
    console.error("[chart] Failed to render:", err);
    container.innerHTML = "";
  }
}

// ── Axis helpers (module-private) ─────────────────────────────────────────────

function _updateYAxis(data) {
  const max = Math.max(...data);
  document.getElementById("y-max").textContent = max;
  document.getElementById("y-mid").textContent = Math.round(max / 2);
}

function _updateXAxis(data) {
  const container = document.getElementById("x-axe");
  container.innerHTML = "";
  if (!data?.length) return;

  const last = data.length - 1;
  const points =
    data.length <= 5
      ? data.map((d) => d.timestamp)
      : [
          data[0].timestamp,
          data[Math.floor(last * 0.25)].timestamp,
          data[Math.floor(last * 0.5)].timestamp,
          data[Math.floor(last * 0.75)].timestamp,
          data[last].timestamp,
        ];

  points.forEach((time) => {
    const span = document.createElement("span");
    span.textContent = time;
    container.appendChild(span);
  });
}
