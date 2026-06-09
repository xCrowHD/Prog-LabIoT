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
  fecthTodayDoorStats,
  fecthLatestSecurityEvents,
  fetchCurrentDoorStatus,
  fetchCurrentFlameStatus,
} from "./api.js";
import { renderPlantChart } from "./chart.js";

// ── Dashboard display ─────────────────────────────────────────────────────────────

export async function switchDashboardView() {
  const node = await fetchCurrentNode();
  const dashplant = document.getElementById("dashboard-plant");
  const dashsec = document.getElementById("dashboard-security");
  console.log(node);
  if (node.type == "PLANT_SENSOR") {
    dashplant.classList.remove("hidden");
    dashsec.classList.add("hidden");
    console.log("[UI] show plant dashboard");
  }
  if (node.type == "SECURITY_SENSOR") {
    dashsec.classList.remove("hidden");
    dashplant.classList.add("hidden");
    console.log("[UI] show security dashboard");
    bootSecurity();
  }
}

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
  document.getElementById("mcu-location-info").innerText = data.location;
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

// ── Security ─────────────────────────────────────────────────────────────
/**
 * Aggiorna l'interfaccia grafica in tempo reale con i dati di sicurezza
 * ricevuti dal WebSocket (Sensore Fiamma, Collisione/Porta, Temperatura).
 * * @param data
 */
export async function updateSecurityDashboard(data) {
  console.log("[ui] Got Security Data");
  console.log(data);
  const dateObj = new Date(data.timestamp * 1000);

  // Formattazione locale (es: "02/06/2026, 19:17:41")
  const formattedDate = dateObj.toLocaleString("it-IT", {
    day: "2-digit",
    month: "2-digit",
    year: "numeric",
    hour: "2-digit",
    minute: "2-digit",
    second: "2-digit",
  });
  if (data.type == "FLAME_ALARM") {
    setFlameAlarm(data, formattedDate);
    addFlameEventToLog(data.temp, formattedDate, data.isOnFlame);
  }
  if (data.type == "DOOR_ALARM") {
    if (!data.doorClose) {
      incrementDoorCounter(data.timestamp);
    }
    doorStatus(data.doorClose);
    addDoorEventToLog(data.doorClose, formattedDate);
  }
}

async function doorStatus(doorClose) {
  const statusLabel = document.getElementById("door-status-label");
  const statusDot = document.getElementById("door-status-dot");

  // La porta è aperta se doorClose è false
  const isOpen = !doorClose;

  // 1. Gestione del Testo (Label)
  if (statusLabel) {
    if (isOpen) {
      statusLabel.innerText = "Door Opened";
      statusLabel.classList.remove("text-on-surface-variant/50");
      statusLabel.classList.add("text-red", "font-bold");
    } else {
      statusLabel.innerText = "Door Closed";
      statusLabel.classList.remove("text-red", "font-bold");
      statusLabel.classList.add("text-on-surface-variant/50");
    }
  }

  // 2. Gestione del Pallino (Dot)
  if (statusDot) {
    if (isOpen) {
      // Porta aperta: togliamo il verde di default (bg-primary) e mettiamo il rosso
      statusDot.classList.remove("bg-primary");
      statusDot.classList.add("bg-red");
    } else {
      // Porta chiusa: ripristiniamo il verde di default
      statusDot.classList.remove("bg-red");
      statusDot.classList.add("bg-primary");
    }
  }
}

async function incrementDoorCounter(timestampRaw) {
  const countElement = document.getElementById("door-count");

  const eventDate = new Date(timestampRaw * 1000);
  const today = new Date();

  // Verifica se è oggi
  const isToday =
    eventDate.getDate() === today.getDate() &&
    eventDate.getMonth() === today.getMonth() &&
    eventDate.getFullYear() === today.getFullYear();

  if (isToday) {
    let currentCount = parseInt(countElement.innerText) || 0;
    countElement.innerText = currentCount + 1;
  }
}

async function setFlameAlarm(data, formattedDate) {
  document.getElementById("flame-last-event").innerText =
    formattedDate + "(UTC)";
  document.getElementById("analog-temp").innerText = data.temp;
  _swapClass("flame-indicator", data.isOnFlame, "bg-primary", "bg-red");
  document.getElementById("flame-status").innerText = data.isOnFlame
    ? "FIRE DETECTED!"
    : "CLEAR";
}

async function addFlameEventToLog(temp, time, isOnFlame) {
  const logContainer = document.getElementById("event-log-list");

  // Definiamo i testi e le classi in base allo stato
  const isAlert = isOnFlame;
  const statusLabel = isAlert ? "ALERT" : "RESTORED";
  const statusColor = isAlert ? "text-red" : "text-primary-500";
  const bgClass = isAlert ? "bg-red/10" : "bg-primary-500/10";
  const message = isAlert ? "Flame detected" : "Flame cleared";

  const newEventHTML = `
    <div class="flex items-center gap-3 px-3 py-2 bg-surface-container-lowest rounded-lg mb-2 border-l-4 ${isAlert ? "border-red" : "border-emerald-500"}">
      <span class="material-symbols-outlined text-[16px] ${statusColor}">
        ${isAlert ? "local_fire_department" : "check_circle"}
      </span>
      <span class="text-[10px] font-mono text-on-surface-variant/50 shrink-0">${time} (UTC)</span>
      <span class="text-xs text-on-surface">${message} — temp ${temp}°C</span>
      <span class="ml-auto px-2 py-0.5 ${bgClass} ${statusColor} text-[9px] font-bold rounded uppercase">${statusLabel}</span>
    </div>
  `;

  logContainer.insertAdjacentHTML("afterbegin", newEventHTML);

  while (logContainer.children.length > 20) {
    logContainer.removeChild(logContainer.lastElementChild);
  }
}

async function addDoorEventToLog(doorClose, time) {
  const logContainer = document.getElementById("event-log-list");

  // La porta è aperta quando doorClose è false
  const isOpen = !doorClose;

  // Definiamo i testi e le classi in base allo stato
  // Se è aperta (isOpen) è un avviso, se è chiusa è un ripristino
  const statusLabel = isOpen ? "OPENED" : "CLOSED";
  const statusColor = isOpen ? "text-red" : "text-primary"; // text-primary usa il verde del tuo config
  const bgClass = isOpen ? "bg-red/10" : "bg-primary/10";
  const message = isOpen ? "Door opened" : "Door closed";
  const icon = isOpen ? "door_open" : "door_back";

  const newEventHTML = `
    <div class="flex items-center gap-3 px-3 py-2 bg-surface-container-lowest rounded-lg mb-2 border-l-4 ${isOpen ? "border-red" : "border-primary"}">
      <span class="material-symbols-outlined text-[16px] ${statusColor}">
        ${icon}
      </span>
      <span class="text-[10px] font-mono text-on-surface-variant/50 shrink-0">${time} (UTC)</span>
      <span class="text-xs text-on-surface">${message}</span>
      <span class="ml-auto px-2 py-0.5 ${bgClass} ${statusColor} text-[9px] font-bold rounded uppercase">${statusLabel}</span>
    </div>
  `;

  logContainer.insertAdjacentHTML("afterbegin", newEventHTML);

  // Manteniamo il limite di 20 elementi
  while (logContainer.children.length > 20) {
    logContainer.removeChild(logContainer.lastElementChild);
  }
}

async function bootSecurity() {
  const logContainer = document.getElementById("event-log-list");
  logContainer.innerHTML = "";

  const node = await fetchCurrentNode();
  const eventsData = await fecthLatestSecurityEvents(node.id, 5);

  for (const event of [...eventsData].reverse()) {
    if (event.alarmType == "FlameAlarm") {
      addFlameEventToLog(event.temp, event.timestamp, event.isOnFlame);
    }
    if (event.alarmType == "DoorAlarm") {
      addDoorEventToLog(event.doorClose, event.timestamp);
    }
  }

  const doorCounter = await fecthTodayDoorStats(node.id);
  const countElement = document.getElementById("door-count");
  countElement.innerText = doorCounter.count;
  const currentDoorStatus = await fetchCurrentDoorStatus(node.id);
  doorStatus(currentDoorStatus.doorClose);
  const currentFlameStatus = await fetchCurrentFlameStatus(node.id);
  setFlameAlarm(currentFlameStatus, currentFlameStatus.timestamp);
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
