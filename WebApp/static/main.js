/* =============================================================
   main.js – Chlorophyll Logic Dashboard
   ============================================================= */

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
let activeDataset = 'temp';

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

/* ── Tab switching ───────────────────────────────────────────
   Highlights the active filter pill and re-renders the chart.
   ─────────────────────────────────────────────────────────── */
function initChartTabs() {
    const tabs = document.querySelectorAll('[data-chart-tab]');

    tabs.forEach((tab) => {
        tab.addEventListener('click', () => {
            const dataset = tab.dataset.chartTab;
            if (dataset === activeDataset) return;

            activeDataset = dataset;

            /* Update pill styles */
            tabs.forEach((t) => {
                const isActive = t.dataset.chartTab === dataset;
                t.classList.toggle('bg-surface-container-highest', isActive);
                t.classList.toggle('text-primary',                  isActive);
                t.classList.toggle('border',                        isActive);
                t.classList.toggle('border-primary/20',             isActive);
                t.classList.toggle('bg-surface-container-lowest',  !isActive);
                t.classList.toggle('text-on-surface-variant',      !isActive);
            });

            renderChart(dataset);
        });
    });
}

/* ── Boot ────────────────────────────────────────────────── */
document.addEventListener('DOMContentLoaded', () => {
    renderChart(activeDataset);
    initChartTabs();
});
