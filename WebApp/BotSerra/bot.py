from telegram import InlineKeyboardButton, InlineKeyboardMarkup, Update
from telegram.ext import Application, CommandHandler, CallbackQueryHandler, ContextTypes
from config import TOKEN_BOT
from mqtt import mqtt_client

# python3 -m BotSerra.bot

# --- 1. COMANDO PRINCIPALE /START ---
async def comando_start(update: Update, context: ContextTypes.DEFAULT_TYPE):
    # Creiamo i bottoni per selezionare l'ESP.
    # 'callback_data' è la stringa invisibile che Telegram ci rimanderà quando l'utente clicca.
    tastiera = [
        [InlineKeyboardButton("🖥️ ESP Serra 1", callback_data="seleziona:ESP_01")],
        [InlineKeyboardButton("🖥️ ESP Serra 2", callback_data="seleziona:ESP_02")]
    ]
    reply_markup = InlineKeyboardMarkup(tastiera)
    
    # Inviamo il messaggio con i bottoni allegati
    await update.message.reply_text(
        "Benvenuto nel sistema IoT! Scegli quale ESP vuoi gestire:", 
        reply_markup=reply_markup
    )

# --- 2. GESTORE DEI CLIC SUI BOTTONI (Callback Query) ---
async def gestisci_bottoni(update: Update, context: ContextTypes.DEFAULT_TYPE):
    query = update.callback_query
    await query.answer() # Notifica a Telegram che il clic è stato preso in carico
    
    # Splittiamo i dati del bottone (es. "seleziona:ESP_01" diventa azione="seleziona", id_esp="ESP_01")
    azione, id_esp = query.data.split(":")
    
    # SE l'utente ha selezionato un ESP, mostriamo i bottoni di START e STOP
    if azione == "seleziona":
        tastiera_comandi = [
            [
                InlineKeyboardButton("▶️ Avvia", callback_data=f"cmd_start:{id_esp}"),
                InlineKeyboardButton("⏸️ Ferma", callback_data=f"cmd_stop:{id_esp}")
            ],
            [InlineKeyboardButton("🔙 Torna indietro", callback_data="reset:menu")]
        ]
        reply_markup = InlineKeyboardMarkup(tastiera_comandi)
        
        # Modifichiamo il messaggio esistente senza inviarne uno nuovo (UX pulita)
        await query.edit_message_text(
            text=f"Hai selezionato l'unità: {id_esp}.\nScegli un comando da inviare:",
            reply_markup=reply_markup
        )
        
    # SE l'utente ha cliccato su Avvia
    elif azione == "cmd_start":
        # 🟡 QUI INSERIRAI LA TUA CLASSE MQTT: mqtt_client.publish(...)
        print(f"DEBUG: Sto per mandare START a {id_esp} tramite MQTT")
        
        await query.edit_message_text(text=f"✅ Comando START inviato a {id_esp}!")
        
    # SE l'utente ha cliccato su Ferma
    elif azione == "cmd_stop":
        # 🟡 QUI INSERIRAI LA TUA CLASSE MQTT: mqtt_client.publish(...)
        print(f"DEBUG: Sto per mandare STOP a {id_esp} tramite MQTT")
        
        await query.edit_message_text(text=f"🛑 Comando STOP inviato a {id_esp}!")
        
    # SE l'utente vuole tornare alla lista principale
    elif azione == "reset":
        # Richiamiamo virtualmente il menu di start modificando il messaggio
        tastiera_iniziale = [
            [InlineKeyboardButton("🖥️ ESP Serra 1", callback_data="seleziona:ESP_01")],
            [InlineKeyboardButton("🖥️ ESP Serra 2", callback_data="seleziona:ESP_02")]
        ]
        await query.edit_message_text(
            text="Scegli quale ESP vuoi gestire:", 
            reply_markup=InlineKeyboardMarkup(tastiera_iniziale)
        )

# --- 3. CONFIGURAZIONE E AVVIO DEL BOT ---
def main():
    # Costruiamo l'applicazione con il Token
    app = Application.builder().token(TOKEN_BOT).build()
    
    # Associazioni: colleghiamo i comandi e i clic alle nostre funzioni Python
    app.add_handler(CommandHandler("start", comando_start))
    app.add_handler(CallbackQueryHandler(gestisci_bottoni))
    
    # Facciamo partire il bot in ascolto continuo (Polling)
    print("🚀 Bot Telegram in esecuzione (Separato). Premi CTRL+C per fermarlo.")
    app.run_polling()

if __name__ == "__main__":
    main()