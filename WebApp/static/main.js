/* =============================================================
   main.js – Chlorophyll Logic Dashboard
   ============================================================= */
console.log("DEBUG: File main.js caricato!");
/* ── Chart data ──────────────────────────────────────────────
   Each value represents a bar height as a percentage (0–100).
   Replace with real telemetry data when available.
   ─────────────────────────────────────────────────────────── */
const CHART_DATA = {
    temp: [40, 45, 55, 65, 60, 75, 85, 80, 70, 60, 50, 55, 65, 45, 40, 35, 30, 45, 55, 65],
    hum:  [60, 62, 58, 55, 57, 63, 70, 72, 68, 65, 60, 58, 55, 60, 62, 65, 66, 64, 60, 58],
    lux:  [ 0,  0,  0,  5, 20, 55, 80, 90, 88, 75, 60, 65, 70, 50, 30, 10,  2,  0,  0,  0],
};

/* Active dataset key */
let activeDataset = 'lux';

/* Active Plant */
let activePlantIndex = 0
let plantArray = [
    "monstera_albo",
    "nepenthes_rajah",
    "ghost_orchid"
]

/* Color map per dataset */
const COLOR_MAP = {
    temp: 'bg-primary',
    hum:  'bg-secondary',
    lux:  'bg-tertiary',
};

/* ── renderChart ─────────────────────────────────────────────
   Clears and re-renders the bar chart for the given dataset.
   ─────────────────────────────────────────────────────────── */
function renderChart(dataset) {
    const container = document.getElementById('chart-bars');
    if (!container) return;

    const data      = CHART_DATA[dataset];
    const colorClass = COLOR_MAP[dataset];

    container.innerHTML = '';

    data.forEach((value) => {
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
        bar.style.height = `${value}%`;

        container.appendChild(bar);
    });
}

async function caricaSogliePianta(nomeId) {
    try {
        // Effettua la chiamata alla tua REST API
        let response = await fetch(`/api/piante/soglie/${nomeId}`);
        
        if (!response.ok) throw new Error("Pianta non trovata");

        let data = await response.json();
        console.log(data)
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

}


async function caricaDatiPianta(nomeId) {
    try {
        // Effettua la chiamata alla tua REST API
        let response = await fetch(`/api/piante/data/${nomeId}`);
        
        if (!response.ok) throw new Error("Pianta non trovata");

        let data = await response.json();
        console.log(data);
    }
    catch (error) {
        console.error("Errore nel caricamento:", error);
        document.getElementById('plant-name').innerText = "Errore Caricamento";
    }
}

async function caricaLatestDatoPianta(nomeId) {
    try {
        // Effettua la chiamata alla tua REST API
        let response = await fetch(`/api/piante/latestdata/${nomeId}`);
        
        if (!response.ok) throw new Error("Pianta non trovata");

        let data = await response.json();
        console.log(data);
        document.getElementById('plant-temp').innerText = data.temp;
        document.getElementById('plant-hum').innerText = data.hum;
        document.getElementById('plant-lux').innerText = data.klux;
    }
    catch (error) {
        console.error("Errore nel caricamento:", error);
    }
}

/* ── Boot ────────────────────────────────────────────────── */
document.addEventListener('DOMContentLoaded', () => {
    console.log("Boot avviato...");
    caricaDatiPianta(plantArray[0]);
    caricaSogliePianta(plantArray[0]);
    caricaLatestDatoPianta(plantArray[0])
    renderChart(activeDataset);
    document.getElementById("plant-loop").addEventListener("click", loopPlants)
});
