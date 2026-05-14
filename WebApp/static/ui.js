/* =============================================================
   ui.js
   All DOM manipulation and view-toggling helpers.
   Pure display logic: receives data objects, writes to the DOM.
   No fetch calls here — use api.js for those.
   ============================================================= */

import {
  fetchPlantThresholds,
  fetchLatestPlantData,
  fetchCurrentNode,
  fecthNodeSettings,
} from "./api.js";
import { renderPlantChart } from "./chart.js";

// ── Plant display ─────────────────────────────────────────────────────────────

/**
 * Show the plant data section and hide the add-plant form.
 * Loads thresholds, latest sensor reading, and chart.
 * @param {{ id: string }} plant
 * @param {string} activeField
 * @param {string} activeTime
 */
export async function showPlantData(plant, activeField, activeTime) {
  _toggle("plant-display-section", true);
  _toggle("add-plant-form", false);

  await Promise.all([
    loadPlantThresholds(plant.id),
    loadLatestSensorData(plant.id),
    renderPlantChart(plant.id, activeField, activeTime),
  ]);
}

export function showAddPlantForm() {
  _toggle("plant-display-section", false);
  _toggle("add-plant-form", true);
}

/** Populate threshold labels from the API response. */
export async function loadPlantThresholds(plantId) {
  try {
    const data = await fetchPlantThresholds(plantId);
    document.getElementById("plant-name").innerText = data.name;
    document.getElementById("plant-img").src = data.img_path;
    document.getElementById("temp-range").innerText =
      `${data.temp_min}° - ${data.temp_max}°`;
    document.getElementById("hum-range").innerText =
      `${data.hum_min}% - ${data.hum_max}%`;
    document.getElementById("light-range").innerText =
      `${data.light_min} - ${data.light_max} (LDR)`;
  } catch (err) {
    console.error("[ui] loadPlantThresholds:", err);
    document.getElementById("plant-name").innerText = "Errore Caricamento";
  }
}

/** Populate the live sensor reading badges. */
export async function loadLatestSensorData(plantId) {
  try {
    const data = await fetchLatestPlantData(plantId);
    document.getElementById("plant-temp").innerText = data.temp;
    document.getElementById("plant-hum").innerText = data.hum;
    document.getElementById("plant-lux").innerText = data.klux;
  } catch (err) {
    console.error("[ui] loadLatestSensorData:", err);
  }
}

// ── MCU panel ─────────────────────────────────────────────────────────────────

export function showMcuInfo() {
  loadMcuInfo();
  _toggle("set-mcu-form", false);
  _toggle("mcu-info", true);
}

export async function loadMcuInfo() {
  const node = await fetchCurrentNode();
  //console.log(nodeId);
  const data = await fecthNodeSettings(node.id);
  document.getElementById("mcu-name-info").innerText = data.name;
  document.getElementById("mcu-backup-info").innerText = data.is_backup;
  document.getElementById("mcu-timer-info").innerText = data.timer;
  document.getElementById("mcu-running-info").innerText = data.is_running;
  document.getElementById("mcu-plantsync-info").innerText = data.plant_id;
}

export function showSetMcuForm() {
  _toggle("mcu-info", false);
  _toggle("set-mcu-form", true);
}

/** Reflect node data (id + online/offline) in the MCU status widget. */
export function renderNodeStatus(data) {
  const isOnline = data.status !== "OFFLINE";

  _swapClass("esp-status", isOnline, "text-red", "text-primary");
  _swapClass("eps-status-icon", isOnline, "bg-red", "bg-primary");

  document.getElementById("node-id").innerText =
    `Monitoring Node: MCU-${data.id}`;
  document.getElementById("esp-status").innerText = `System ${data.status}`;
}

// ── Tab helpers ───────────────────────────────────────────────────────────────

/**
 * Activate the clicked tab and deactivate the others.
 * Works for both field-tabs and time-tabs.
 * @param {HTMLElement} clickedTab
 * @param {NodeList}    allTabs
 */
export function activateTab(clickedTab, allTabs) {
  allTabs.forEach((t) => {
    t.classList.remove("border", "border-primary/20");
    t.classList.replace(
      "bg-surface-container-highest",
      "bg-surface-container-lowest",
    );
    t.classList.replace("text-primary", "text-on-surface-variant");
  });
  clickedTab.classList.add("border", "border-primary/20");
  clickedTab.classList.replace(
    "bg-surface-container-lowest",
    "bg-surface-container-highest",
  );
  clickedTab.classList.replace("text-on-surface-variant", "text-primary");
}

// ── Form helpers ──────────────────────────────────────────────────────────────

/** Clear all inputs (and uncheck checkboxes) inside a container. */
export function clearFormInputs(containerId) {
  document.querySelectorAll(`#${containerId} input`).forEach((input) => {
    if (input.type === "checkbox") input.checked = false;
    else input.value = "";
  });
}

// ── Private ───────────────────────────────────────────────────────────────────

function _toggle(id, visible) {
  const el = document.getElementById(id);
  if (!el) return;
  el.classList.toggle("hidden", !visible);
}

/** Replace *offClass* with *onClass* when *condition* is true, and vice-versa. */
function _swapClass(id, condition, offClass, onClass) {
  const el = document.getElementById(id);
  if (!el) return;
  if (condition) {
    el.classList.replace(offClass, onClass);
  } else {
    el.classList.replace(onClass, offClass);
  }
}
