/* =============================================================
   main.js
   Application state, event wiring, and boot sequence.
   Imports: api.js, ui.js, chart.js
   ============================================================= */

import {
  fetchPlantCount,
  fetchPlantByPosition,
  fetchNextNode,
  fetchCurrentNode,
  syncMqttThresholds,
  sendStartStop,
  sendSetMcu,
  savePlant,
} from "./api.js";

import {
  showPlantData,
  showAddPlantForm,
  showMcuInfo,
  showSetMcuForm,
  renderNodeStatus,
  activateTab,
  clearFormInputs,
} from "./ui.js";

import { renderPlantChart } from "./chart.js";

// ── Application state ─────────────────────────────────────────────────────────
let activePlantIndex = 0;
let activeMcuFormStep = 0; // 0 = info panel, 1 = set-mcu form
let activeField = "temp";
let activeTime = "24h";

// ── Helpers ───────────────────────────────────────────────────────────────────

async function getCurrentPlant() {
  return fetchPlantByPosition(activePlantIndex);
}

// ── Event handlers ────────────────────────────────────────────────────────────

async function onLoopPlants() {
  activePlantIndex++;
  const [plant, { count }] = await Promise.all([
    getCurrentPlant(),
    fetchPlantCount(),
  ]);

  if (activePlantIndex === count) {
    showAddPlantForm();
  } else if (activePlantIndex > count) {
    activePlantIndex = 0;
    const first = await getCurrentPlant();
    showPlantData(first, activeField, activeTime);
  } else {
    showPlantData(plant, activeField, activeTime);
  }
}

async function onLoopMcuInfo() {
  activeMcuFormStep = (activeMcuFormStep + 1) % 2;
  activeMcuFormStep === 0 ? showMcuInfo() : showSetMcuForm();
}

async function onSelectFieldTab() {
  activateTab(this, document.querySelectorAll("#chart-field-tabs span"));
  activeField = this.getAttribute("data-field");
  const plant = await getCurrentPlant();
  if (plant) renderPlantChart(plant.id, activeField, activeTime);
}

async function onSelectTimeTab() {
  activateTab(this, document.querySelectorAll("#chart-time-tabs span"));
  activeTime = this.getAttribute("data-field");
  const plant = await getCurrentPlant();
  if (plant) renderPlantChart(plant.id, activeField, activeTime);
}

async function onSyncMqtt() {
  const plant = await getCurrentPlant();
  if (!plant) return;
  try {
    await syncMqttThresholds(plant.id);
  } catch (err) {
    console.error("[main] syncMqtt:", err);
  }
}

async function onStartStop() {
  const btn = this;
  const shouldStart = btn.getAttribute("data-field") === "START";
  const statusTextEl = document.getElementById("start-stop-text");

  try {
    await sendStartStop(shouldStart);
    btn.setAttribute("data-field", shouldStart ? "STOP" : "START");
    statusTextEl.innerHTML = shouldStart ? "STOP ESP8266" : "START ESP8266";
  } catch (err) {
    console.error("[main] startStop:", err);
    statusTextEl.innerHTML = "ERRORE CONNESSIONE";
  }
}

async function onSetMcu() {
  const payload = {
    name: document.getElementById("mcu-name").value,
    backup: document.getElementById("mcu-backup").checked,
    timer: document.getElementById("mcu-timer").value,
  };

  try {
    await sendSetMcu(payload);
    clearFormInputs("set-mcu-form");
  } catch (err) {
    console.error("[main] setMcu:", err);
    alert("Errore durante il salvataggio: " + err.message);
  }
}

async function onSavePlant() {
  try {
    await savePlant({
      name: document.getElementById("new-name").value,
      image: document.getElementById("new-img").files[0],
      temp_min: document.getElementById("temp-min-input").value,
      temp_max: document.getElementById("temp-max-input").value,
      hum_min: document.getElementById("hum-min-input").value,
      hum_max: document.getElementById("hum-max-input").value,
      light_min: document.getElementById("lux-min-input").value,
      light_max: document.getElementById("lux-max-input").value,
    });
    clearFormInputs("add-plant-form");
  } catch (err) {
    alert("Errore nel salvataggio: " + err.message);
  }
}

async function onNextNode() {
  const data = await fetchNextNode();
  renderNodeStatus(data);
}

// ── Boot ──────────────────────────────────────────────────────────────────────

async function boot() {
  activePlantIndex = 0;
  const plant = await getCurrentPlant();
  if (plant) {
    showPlantData(plant, activeField, activeTime);
  } else {
    showAddPlantForm();
  }

  const nodeData = await fetchCurrentNode();
  renderNodeStatus(nodeData);
}

document.addEventListener("DOMContentLoaded", () => {
  boot();

  document.getElementById("plant-loop").addEventListener("click", onLoopPlants);
  document.getElementById("sync-mqtt").addEventListener("click", onSyncMqtt);
  document
    .getElementById("save-new-plant")
    .addEventListener("click", onSavePlant);
  document.getElementById("start-esp").addEventListener("click", onStartStop);
  document
    .getElementById("mcu-set-loop")
    .addEventListener("click", onLoopMcuInfo);
  document.getElementById("set-mcu").addEventListener("click", onSetMcu);
  document
    .getElementById("next-node-btn")
    .addEventListener("click", onNextNode);

  document
    .querySelectorAll("#chart-field-tabs span")
    .forEach((tab) => tab.addEventListener("click", onSelectFieldTab));
  document
    .querySelectorAll("#chart-time-tabs span")
    .forEach((tab) => tab.addEventListener("click", onSelectTimeTab));
});
