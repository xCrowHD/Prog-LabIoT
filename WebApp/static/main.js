/* =============================================================
   main.js – Chlorophyll Logic Dashboard
   ============================================================= */
console.log("DEBUG: File main.js caricato!");
/* ── Chart data ──────────────────────────────────────────────
   Each value represents a bar height as a percentage (0–100).
   Replace with real telemetry data when available.
   ─────────────────────────────────────────────────────────── */

/* Active Plant */
let activePlantIndex = 0;

let activeField = "temp";
let activeTime = "24h";

/* Color map per dataset */
const COLOR_MAP = {
  temp: "bg-primary",
  hum: "bg-secondary",
  lux: "bg-tertiary",
};

/* ── renderChart ─────────────────────────────────────────────
   Clears and re-renders the bar chart for the given dataset.
   ─────────────────────────────────────────────────────────── */
async function _renderPlantChart(plantid, field, lastTime) {
  const container = document.getElementById("chart-bars");
  if (!container) return;

  try {
    let response = await fetch(`/api/piante/data/${plantid}/${lastTime}`);

    if (!response.ok) throw new Error("Pianta non trovata");
    let data = await response.json();
    const colorClass = COLOR_MAP[field];
    //console.log(data);
    let field_data = data.map((record) => record[field]);
    //console.log(field_data);

    if (field_data.length == 0) {
      container.innerHTML = "";
      document.getElementById("y-max").textContent = "No Data";
      document.getElementById("y-mid").textContent = "No Data";
      _updateXAxis(data);
      return;
    }

    _updateYAxis(field_data);
    _updateXAxis(data);
    const max = Math.max(...field_data);
    container.innerHTML = "";

    field_data.forEach((value) => {
      const bar = document.createElement("div");
      bar.className = [
        "flex-1",
        `${colorClass}/20`,
        "rounded-t-sm",
        `h-[${value}%]`,
        `hover:${colorClass}`,
        "transition-all",
        "duration-300",
      ].join(" ");

      // Inline style fallback (Tailwind JIT won't see dynamic h-[] at runtime)
      bar.style.height = `${(value / max) * 100}%`;

      container.appendChild(bar);
    });
  } catch (error) {
    console.error("Errore nel caricamento:", error);
    container.innerHTML = "";
  }
}

async function _caricaSogliePianta(nomeId) {
  try {
    // Effettua la chiamata alla tua REST API
    let response = await fetch(`/api/piante/soglie/${nomeId}`);

    if (!response.ok) throw new Error("Pianta non trovata");

    let data = await response.json();
    console.log(data);

    // Aggiorna l'HTML con i dati ricevuti
    document.getElementById("plant-name").innerText = `${data.name}`;
    document.getElementById("plant-img").src = `${data.img_path}`;

    document.getElementById("temp-range").innerText =
      `${data.temp_min}° - ${data.temp_max}°`;

    document.getElementById("hum-range").innerText =
      `${data.hum_min}% - ${data.hum_max}%`;

    document.getElementById("light-range").innerText =
      `${data.light_min} - ${data.light_max} (LDR)`;
  } catch (error) {
    console.error("Errore nel caricamento:", error);
    document.getElementById("plant-name").innerText = "Errore Caricamento";
  }
}

async function loopPlants() {
  let plant = null;
  let len = 0;
  activePlantIndex++;
  try {
    plant = await getCurrentIndexPlant();

    let response = await fetch("/api/piante/count");
    if (!response.ok) throw new Error("Errore nel db delle piante");

    let data = await response.json();
    len = data.count;
  } catch (error) {
    console.error("Errore nel caricamento:", error);
  }

  // Se l'indice è uguale alla lunghezza dell'array, mostriamo il form
  if (activePlantIndex === len) {
    _showAddPlantForm();
  }
  // Se superiamo anche il form, resettiamo a 0 (prima pianta)
  else if (activePlantIndex > len) {
    activePlantIndex = 0;
    plant = await getCurrentIndexPlant();
    _showPlantData(plant);
  }
  // Altrimenti, mostriamo la pianta corrente
  else {
    _showPlantData(plant);
  }
}

async function _caricaLatestDatoPianta(nomeId) {
  try {
    let response = await fetch(`/api/piante/latestdata/${nomeId}`);

    if (!response.ok) throw new Error("Pianta non trovata");

    let data = await response.json();
    console.log(data);
    // console.log(data);
    document.getElementById("plant-temp").innerText = data.temp;
    document.getElementById("plant-hum").innerText = data.hum;
    document.getElementById("plant-lux").innerText = data.klux;
  } catch (error) {
    console.error("Errore nel caricamento:", error);
  }
}

async function _showAddPlantForm() {
  document.getElementById("plant-display-section").classList.add("hidden");
  document.getElementById("add-plant-form").classList.remove("hidden");
}

async function _showPlantData(plant) {
  document.getElementById("plant-display-section").classList.remove("hidden");
  document.getElementById("add-plant-form").classList.add("hidden");

  _caricaLatestDatoPianta(plant.id);
  _caricaSogliePianta(plant.id);
  _renderPlantChart(plant.id, activeField, activeTime);
}

async function selectTabPlantField() {
  const tabs = document.querySelectorAll("#chart-field-tabs span");
  tabs.forEach((t) => {
    t.classList.remove("border", "border-primary/20");
    t.classList.replace(
      "bg-surface-container-highest",
      "bg-surface-container-lowest",
    );
    t.classList.replace("text-primary", "text-on-surface-variant");
  });

  this.classList.add("border", "border-primary/20");
  this.classList.replace(
    "bg-surface-container-lowest",
    "bg-surface-container-highest",
  );
  this.classList.replace("text-on-surface-variant", "text-primary");
  activeField = this.getAttribute("data-field");
  let plant = await getCurrentIndexPlant();
  if (plant != null) {
    _renderPlantChart(plant.id, activeField, activeTime);
  }
}

async function selectTabPlantTime() {
  const tabs = document.querySelectorAll("#chart-time-tabs span");
  tabs.forEach((t) => {
    t.classList.remove("border", "border-primary/20");
    t.classList.replace(
      "bg-surface-container-highest",
      "bg-surface-container-lowest",
    );
    t.classList.replace("text-primary", "text-on-surface-variant");
  });

  this.classList.add("border", "border-primary/20");
  this.classList.replace(
    "bg-surface-container-lowest",
    "bg-surface-container-highest",
  );
  this.classList.replace("text-on-surface-variant", "text-primary");
  activeTime = this.getAttribute("data-field");
  let plant = await getCurrentIndexPlant();
  if (plant != null) {
    _renderPlantChart(plant.id, activeField, activeTime);
  }
}

async function syncMQTTSoglie() {
  try {
    let plant = await getCurrentIndexPlant();
    // Effettua la chiamata alla tua REST API
    let response = await fetch(`/api/piante/syncmqtt/${plant.id}`);

    if (!response.ok) throw new Error("Pianta non trovata");
  } catch (error) {
    console.error("Errore:", error);
  }
}

async function startStopEsp8266() {
  try {
    let status = this.getAttribute("data-field");
    // Effettua la chiamata alla tua REST API
    let response = await fetch(`/api/piante/startstop/${status}`);
    if (status == "START") {
      this.setAttribute("data-field", "STOP");
      document.getElementById("start-stop-text").innerHTML = "STOP ESP8266";
    } else if (status == "STOP") {
      this.setAttribute("data-field", "START");
      document.getElementById("start-stop-text").innerHTML = "START ESP8266";
    } else {
      document.getElementById("start-stop-text").innerHTML = "ERRORE";
    }
    //console.log("Called It");
  } catch (error) {
    console.error("Errore:", error);
  }
}

async function _updateYAxis(data) {
  const max = Math.max(...data);
  document.getElementById("y-max").textContent = max;
  document.getElementById("y-mid").textContent = Math.round(max / 2);
}

async function _updateXAxis(data) {
  let xPoints = [];
  const container = document.getElementById("x-axe");
  container.innerHTML = "";
  if (!data || data.length === 0) return;
  const lastIdx = data.length - 1;
  if (data.length <= 5) {
    xPoints = data.map((d) => d.timestamp);
  } else {
    xPoints = [
      data[0].timestamp,
      data[Math.floor(lastIdx * 0.25)].timestamp,
      data[Math.floor(lastIdx * 0.5)].timestamp,
      data[Math.floor(lastIdx * 0.75)].timestamp,
      data[lastIdx].timestamp,
    ];
  }

  xPoints.forEach((time) => {
    const span = document.createElement("span");
    span.textContent = time;
    container.appendChild(span);
  });
}

async function handleSavePlant() {
  const nameInput = document.getElementById("new-name").value;
  const imgInput = document.getElementById("new-img").files[0]; // Prende il file

  // Creiamo il contenitore per i dati "multipart"
  const formData = new FormData();
  formData.append("name", nameInput);
  formData.append("temp_min", document.getElementById("temp-min-input").value);
  formData.append("temp_max", document.getElementById("temp-max-input").value);
  formData.append("hum_min", document.getElementById("hum-min-input").value);
  formData.append("hum_max", document.getElementById("hum-max-input").value);
  formData.append("light_min", document.getElementById("lux-min-input").value);
  formData.append("light_max", document.getElementById("lux-max-input").value);

  // Aggiungiamo l'immagine solo se l'utente l'ha selezionata
  if (imgInput) {
    formData.append("image", imgInput);
  }

  const response = await fetch("/api/plants/save", {
    method: "POST",
    body: formData,
  });

  const result = await response.json();
  if (response.ok) {
    console.log("Salvataggio riuscito:", result);

    const formContainer = document.getElementById("add-plant-form");
    const inputs = formContainer.querySelectorAll("input");
    inputs.forEach((input) => {
      input.value = "";
    });
  } else {
    alert("Errore nel salvataggio: " + result.detail);
  }
}

async function getCurrentIndexPlant() {
  const response = await fetch(
    `/api/piante/soglie/position/${activePlantIndex}`,
  );

  if (!response.ok) {
    return null;
  }

  const plant = await response.json();
  return plant;
}

async function loadAtStart() {
  activePlantIndex = 0;
  const plant = await getCurrentIndexPlant();
  if (plant == null) {
    _showAddPlantForm();
  } else {
    _showPlantData(plant);
  }
}

/* ── Boot ────────────────────────────────────────────────── */
document.addEventListener("DOMContentLoaded", () => {
  console.log("Boot avviato...");
  loadAtStart();

  document.getElementById("plant-loop").addEventListener("click", loopPlants);

  document
    .getElementById("sync-mqtt")
    .addEventListener("click", syncMQTTSoglie);

  const ftabs = document.querySelectorAll("#chart-field-tabs span");
  ftabs.forEach((tab) => {
    tab.addEventListener("click", selectTabPlantField);
  });
  const ttabs = document.querySelectorAll("#chart-time-tabs span");
  ttabs.forEach((tab) => {
    tab.addEventListener("click", selectTabPlantTime);
  });

  document
    .getElementById("save-new-plant")
    .addEventListener("click", handleSavePlant);

  document
    .getElementById("start-esp")
    .addEventListener("click", startStopEsp8266);
});
