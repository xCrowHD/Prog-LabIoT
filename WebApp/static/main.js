/* =============================================================
   main.js – Chlorophyll Logic Dashboard
   ============================================================= */
console.log("DEBUG: File main.js caricato!");
/* ── Chart data ──────────────────────────────────────────────
   Each value represents a bar height as a percentage (0–100).
   Replace with real telemetry data when available.
   ─────────────────────────────────────────────────────────── */

/* Active Plant */
let activePlantIndex = 0
let plantArray = [
    "monstera_albo",
    "nepenthes_rajah",
    "ghost_orchid"
]

/* Color map per dataset */
const COLOR_MAP = {
    "temp": 'bg-primary',
    "hum":  'bg-secondary',
    "lux":  'bg-tertiary',
};

/* ── renderChart ─────────────────────────────────────────────
   Clears and re-renders the bar chart for the given dataset.
   ─────────────────────────────────────────────────────────── */
async function renderPlantChart(plantid, field, lastTime) {
    const container = document.getElementById('chart-bars');
    if (!container) return;
    
    try{
        let response = await fetch(`/api/piante/data/${plantid}/${lastTime}`);
        
        if (!response.ok) throw new Error("Pianta non trovata");
        let data = await response.json();
        const colorClass = COLOR_MAP[field];

        let field_data = data.map(record => record[field]);
        updateYAxis(field_data);
        const max = Math.max(...field_data);
        container.innerHTML = '';

        field_data.forEach((value) => {
            const bar = document.createElement('div');
            bar.className = [
                'flex-1',
                `${colorClass}/20`,
                'rounded-t-sm',
                `h-[${value}%]`,
                `hover:${colorClass}`,
                'transition-all',
                'duration-300',
            ].join(' ');

            // Inline style fallback (Tailwind JIT won't see dynamic h-[] at runtime)
            bar.style.height = `${value/max*100}%`;

            container.appendChild(bar);
        });

    }catch(error){
        console.error("Errore nel caricamento:", error);
        container.innerHTML = '';
    }

}

async function caricaSogliePianta(nomeId) {
    try {
        // Effettua la chiamata alla tua REST API
        let response = await fetch(`/api/piante/soglie/${nomeId}`);
        
        if (!response.ok) throw new Error("Pianta non trovata");

        let data = await response.json();
        console.log(data);
        let soglie = data.thresholds;

        // Aggiorna l'HTML con i dati ricevuti
        document.getElementById('plant-name').innerText = `${data.name}`;
        document.getElementById('plant-img').src = `${data.img}`;
        
        document.getElementById('temp-range').innerText = 
            `${soglie.temp.min}° - ${soglie.temp.max}°`;
            
        document.getElementById('hum-range').innerText = 
            `${soglie.hum.min}% - ${soglie.hum.max}%`;
            
        document.getElementById('light-range').innerText = 
            `${soglie.light.min} - ${soglie.light.max} (LDR)`;

    } catch (error) {
        console.error("Errore nel caricamento:", error);
        document.getElementById('plant-name').innerText = "Errore Caricamento";
    }
}

async function loopPlants() {
    if (activePlantIndex >= plantArray.length - 1){
        activePlantIndex = 0;
    }
    else{
        activePlantIndex++;
    }
    caricaLatestDatoPianta(plantArray[activePlantIndex]);
    caricaSogliePianta(plantArray[activePlantIndex]);
    renderPlantChart(plantArray[activePlantIndex], "temp", "30d")

}


async function caricaLatestDatoPianta(nomeId) {
    try {
        // Effettua la chiamata alla tua REST API
        let response = await fetch(`/api/piante/latestdata/${nomeId}`);
        
        if (!response.ok) throw new Error("Pianta non trovata");

        let data = await response.json();
        // console.log(data);
        document.getElementById('plant-temp').innerText = data.temp;
        document.getElementById('plant-hum').innerText = data.hum;
        document.getElementById('plant-lux').innerText = data.klux;
    }
    catch (error) {
        console.error("Errore nel caricamento:", error);
    }
}

async function selectTabPlantField() {
    const tabs = document.querySelectorAll('#chart-field-tabs span');
    tabs.forEach( t => {
        t.classList.remove("border", "border-primary/20");
        t.classList.replace("bg-surface-container-highest", "bg-surface-container-lowest");
        t.classList.replace("text-primary", "text-on-surface-variant");
    });

    this.classList.add("border", "border-primary/20");
    this.classList.replace("bg-surface-container-lowest", "bg-surface-container-highest");
    this.classList.replace("text-on-surface-variant", "text-primary");

}

async function syncMQTTSoglie() {
    try {
        // Effettua la chiamata alla tua REST API
        let response = await fetch(`/api/piante/syncmqtt/${plantArray[activePlantIndex]}`);
        
        if (!response.ok) throw new Error("Pianta non trovata");
    }
    catch (error) {
        console.error("Errore:", error);
    }
}

function updateYAxis(data) {
  const max = Math.max(...data);
  document.getElementById('y-max').textContent = max;
  document.getElementById('y-mid').textContent = Math.round(max / 2);
}

/* ── Boot ────────────────────────────────────────────────── */
document.addEventListener('DOMContentLoaded', () => {
    console.log("Boot avviato...");
    caricaSogliePianta(plantArray[0]);
    caricaLatestDatoPianta(plantArray[0])
    renderPlantChart(plantArray[0], "temp", "30d");
    document.getElementById("plant-loop").addEventListener("click", loopPlants);
    document.getElementById("sync-mqtt").addEventListener("click", syncMQTTSoglie);
    const tabs = document.querySelectorAll('#chart-field-tabs span');
    tabs.forEach(tab => {
        tab.addEventListener("click", selectTabPlantField);
    });
});
