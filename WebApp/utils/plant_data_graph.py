import io
import matplotlib
matplotlib.use('Agg') 
import matplotlib.pyplot as plt

def genera_immagine_grafico(array_valori: list, array_orari: list, nome_campo: str) -> io.BytesIO:
    """
    Genera il grafico accoppiando i valori del sensore con i timestamp del frontend.
    """
    colori = {
        "temp": "#ff5733", 
        "hum": "#3399ff",     
        "lux": "#ffcc00"         
    }
    colore_scelto = colori.get(nome_campo.lower(), "#2ecc71")

    plt.figure(figsize=(8, 4))
    
    # Disegniamo la linea usando gli indici numerici per l'asse X se gli orari sono stringhe,
    # evitando che matplotlib si confonda con troppe scritte
    x_indici = list(range(len(array_orari)))
    
    plt.plot(x_indici, array_valori, color=colore_scelto, linewidth=2, marker='o', markersize=3)
    plt.fill_between(x_indici, array_valori, color=colore_scelto, alpha=0.15)
    
    # --- LOGICA ASSI SPECULARE AL FRONTEND ---
    # Invece di calcolare a mano il 25%, 50% ecc., decidiamo quanti "timestamp" mostrare sotto
    if len(array_orari) > 0:
        last = len(array_orari) - 1
        # Se sono meno di 5, mostragli tutti, altrimenti prendi i 5 punti cardine come facevi in JS
        if len(array_orari) <= 5:
            indici_da_mostrare = x_indici
        else:
            indici_da_mostrare = [0, int(last * 0.25), int(last * 0.5), int(last * 0.75), last]
        
        # Applichiamo le etichette (i tuoi d.timestamp) solo su quei punti
        etichette_da_mostrare = [array_orari[i] for i in indici_da_mostrare]
        plt.xticks(indici_da_mostrare, etichette_da_mostrare, rotation=15, fontsize=9)

    plt.grid(True, linestyle='--', alpha=0.5)
    plt.title(f"Andamento {nome_campo.title()}", fontsize=14, fontweight='bold', pad=15)
    plt.ylabel(nome_campo.title(), fontsize=10)
    plt.tight_layout()

    # Salvataggio in RAM
    buf = io.BytesIO()
    plt.savefig(buf, format='png', dpi=120)
    buf.seek(0)
    plt.close()
    
    return buf