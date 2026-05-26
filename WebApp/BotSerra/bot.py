from telegram import InlineKeyboardButton, InlineKeyboardMarkup, Update
from telegram.ext import Application, CommandHandler, CallbackQueryHandler, ContextTypes
from config import TOKEN_BOT
from mqtt import mqtt_hub
from db.plants_db import plant_db_manager
from services.influx_service import get_plant_data, get_latest_plant_data
from utils.plant_data_graph import genera_immagine_grafico

# python3 -m BotSerra.bot

# --- COMANDI PRINCIPALI ---
async def comando_start(update: Update, context: ContextTypes.DEFAULT_TYPE):
    markup = genera_tastiera_esp(comando_origine="start")
    
    await update.message.reply_text(
        "▶️ Quale nodo vuoi avviare?\n*(Puoi selezionare anche i nodi offline, riceveranno il comando al risveglio)*", 
        reply_markup=markup,
        parse_mode="Markdown"
    )

async def comando_stop(update: Update, context: ContextTypes.DEFAULT_TYPE):
    markup = genera_tastiera_esp(comando_origine="stop")
    
    await update.message.reply_text(
        "▶️ Quale nodo vuoi fermare?\n*(Puoi selezionare anche i nodi offline, riceveranno il comando al risveglio)*", 
        reply_markup=markup,
        parse_mode="Markdown"
    )

async def comando_sync_plant(update: Update, context: ContextTypes.DEFAULT_TYPE):
    markup = genera_tastiera_esp(comando_origine="selplant")
    
    await update.message.reply_text(
        "▶️ Su quale nodo vuoi sincronizzare la nuova pianta?\n*(Puoi selezionare anche i nodi offline, riceveranno il comando al risveglio)*", 
        reply_markup=markup,
        parse_mode="Markdown"
    )

async def comando_plant_data(update: Update, context: ContextTypes.DEFAULT_TYPE):
    markup = genera_tastiera_piante(comando_origine="plantfield")
    
    await update.message.reply_text(
        "▶️ Di quale pianta vuoi i dati?\n", 
        reply_markup=markup,
        parse_mode="Markdown"
    )

# --- 2. GESTORE DEI CLIC SUI BOTTONI (Callback Query) ---

async def gestisci_start(update: Update, context: ContextTypes.DEFAULT_TYPE):
    query = update.callback_query
    await query.answer()
    
    # query.data sarà ad esempio "start:840D8EB0612D"
    _, id_esp = query.data.split(":")
    
    mqtt_hub.send_start_stop(id_esp, True)
    print(f"[Bot Telegram] Mandato comando MQTT 'START' a {id_esp}")
    
    await query.edit_message_text(text=f"✅ MCU `{id_esp}` avviato correttamente!", parse_mode="Markdown")

async def gestisci_stop(update: Update, context: ContextTypes.DEFAULT_TYPE):
    query = update.callback_query
    await query.answer()
    
    # query.data sarà ad esempio "stop:840D8EB0612D"
    _, id_esp = query.data.split(":")
    
    mqtt_hub.send_start_stop(id_esp, False)
    print(f"[Bot Telegram] Mandato comando MQTT 'STOP' a {id_esp}")
    
    await query.edit_message_text(text=f"✅ MCU `{id_esp}` fermato correttamente!", parse_mode="Markdown")

async def gestisci_select_plant(update: Update, context: ContextTypes.DEFAULT_TYPE):
    query = update.callback_query
    await query.answer()
    _, id_esp = query.data.split(":")
    markup = genera_tastiera_piante(f"syncplant:{id_esp}")
    print(f"[Bot Telegram] Mostra piante da sincronizzare a {id_esp}")
    await query.edit_message_text(text="▶️ Quale pianta vuoi sincronizzare?\n", 
                                  reply_markup=markup,
                                  parse_mode="Markdown")

async def gestisci_sync_plant(update: Update, context: ContextTypes.DEFAULT_TYPE):
    query = update.callback_query
    await query.answer()
    _, esp_id, plant_id = query.data.split(":")
    plant = plant_db_manager.get_plant_by_id(plant_id)
    print(f"[Bot Telegram] {plant.name} sync su {esp_id}")
    mqtt_hub.send_thresholds(esp_id, plant_id)
    await query.edit_message_text(text=f"✅ {plant.name} sincronizzata correttamente su `{esp_id}`!", parse_mode="Markdown")

async def gestisci_plant_field(update: Update, context: ContextTypes.DEFAULT_TYPE):
    query = update.callback_query
    await query.answer()
    print(query.data)
    _, plant_id = query.data.split(":")
    markup = genera_tastiera_field(f"planttime:{plant_id}")
    print(f"[Bot Telegram] Mostra Field da scegliere per {plant_id}")
    await query.edit_message_text(text="▶️ Quale Field ti interessa?\n", 
                                  reply_markup=markup,
                                  parse_mode="Markdown")

async def gestisci_plant_time(update: Update, context: ContextTypes.DEFAULT_TYPE):
    query = update.callback_query
    await query.answer()
    print(query.data)
    _, plant_id, field = query.data.split(":")
    markup = genera_tastiera_periodo(f"plantgraph:{plant_id}:{field}")
    print(f"[Bot Telegram] Mostra Periodo da scegliere per {plant_id}")
    await query.edit_message_text(text="▶️ Quale Periodo ti interessa?\n", 
                                  reply_markup=markup,
                                  parse_mode="Markdown")

async def gestisci_plant_graph(update: Update, context: ContextTypes.DEFAULT_TYPE):
    query = update.callback_query
    await query.answer()
    print(query.data)
    _, plant_id, field, time = query.data.split(":")
    data = get_plant_data(plant_id, time)
    plant_name = plant_db_manager.get_plant_by_id(plant_id).name
    
    field_data = [record[field] for record in data if field in record and record[field] is not None]
    time_data = [record["timestamp"] for record in data if field in record and record[field] is not None]

    image = genera_immagine_grafico(field_data, time_data, field)
    print(f"[Bot Telegram] Invio Foto dati {plant_id}")

    chat_id = update.effective_chat.id
    await context.bot.send_photo(
        chat_id=chat_id,
        photo=image,
        caption=f"📊 Grafico di *{field.upper()}* per la pianta `{plant_name}`",
        parse_mode="Markdown"
    )
    await query.delete_message()

    
# --- Helpers ---
def genera_tastiera_esp(comando_origine: str) -> InlineKeyboardMarkup:
    tastiera = []
    
    # Cicliamo sul dizionario esp_list
    for esp_id, info in mqtt_hub.esp_list.items():
        
        # Controlliamo lo stato per scegliere l'emoji corretta
        status = info.get("status", "OFFLINE") # Default offline se la chiave non esiste
        emoji_stato = "🟢" if status == "ONLINE" else "🔴"
        
        # Creiamo il testo del bottone
        testo_bottone = f"{emoji_stato} {esp_id} ({status.upper()})"
        
        # Creiamo il bottone e lo appendiamo alla tastiera
        bottone = InlineKeyboardButton(
            text=testo_bottone, 
            callback_data=f"{comando_origine}:{esp_id}"
        )
        tastiera.append([bottone])
        
    # Se il dizionario è vuoto mettiamo un bottone di avviso
    if not tastiera:
        tastiera.append([InlineKeyboardButton("❌ Nessun ESP rilevato", callback_data="error:no_esp")])
        
    return InlineKeyboardMarkup(tastiera)


def genera_tastiera_piante(comando_origine: str) -> InlineKeyboardMarkup:
    tastiera = []
    for pianta in plant_db_manager.get_all_plants():
        testo_bottone = f"{pianta.name}"
        # print(f"{comando_origine}:{pianta.id}")
        bottone = InlineKeyboardButton(
            text=testo_bottone, 
            callback_data=f"{comando_origine}:{pianta.id}"
        )
        tastiera.append([bottone])

    # Se il dizionario è vuoto mettiamo un bottone di avviso
    if not tastiera:
        tastiera.append([InlineKeyboardButton("❌ Nessuna Pianta rilevata", callback_data="error:no_plants")])
        
    return InlineKeyboardMarkup(tastiera)

def genera_tastiera_field(comando_origine: str) -> InlineKeyboardMarkup:
    tastiera = []
    
    opzioni_sensori = [
        ("🌡️ Temperatura", "temp"),
        ("💧 Umidità", "hum"),
        ("☀️ Luce", "lux")
    ]
    
    for nome, valore in opzioni_sensori:
        bottone = InlineKeyboardButton(
            text=nome,
            callback_data=f"{comando_origine}:{valore}"
        )
        tastiera.append([bottone]) 
        
    return InlineKeyboardMarkup(tastiera)


def genera_tastiera_periodo(comando_origine: str) -> InlineKeyboardMarkup:
    tastiera = []
    
    opzioni_periodo = [
        ("⏱️ Ultime 24 Ore", "24h"),
        ("📅 Ultimi 7 Giorni", "7d"),
        ("📆 Ultimi 30 Giorni", "30d")
    ]
    
    for nome, valore in opzioni_periodo:
        bottone = InlineKeyboardButton(
            text=nome,
            callback_data=f"{comando_origine}:{valore}"
        )
        tastiera.append([bottone])
        
    return InlineKeyboardMarkup(tastiera)

# --- CONFIGURAZIONE E AVVIO DEL BOT ---
def main():
    # Costruiamo l'applicazione con il Token
    app = Application.builder().token(TOKEN_BOT).build()
    
    # Associazioni: colleghiamo i comandi e i clic alle nostre funzioni Python
    app.add_handler(CommandHandler("start", comando_start))
    app.add_handler(CallbackQueryHandler(gestisci_start, pattern=r"^start:.*"))
    app.add_handler(CommandHandler("stop", comando_stop))
    app.add_handler(CallbackQueryHandler(gestisci_stop, pattern=r"^stop:.*"))
    app.add_handler(CommandHandler("syncplant", comando_sync_plant))
    app.add_handler(CallbackQueryHandler(gestisci_select_plant, pattern=r"^selplant:.*"))
    app.add_handler(CallbackQueryHandler(gestisci_sync_plant, pattern=r"^syncplant:.*:.*"))
    app.add_handler(CommandHandler("plantdata", comando_plant_data))
    app.add_handler(CallbackQueryHandler(gestisci_plant_field, pattern=r"^plantfield:.*"))
    app.add_handler(CallbackQueryHandler(gestisci_plant_time, pattern=r"^planttime:.*:.*"))
    app.add_handler(CallbackQueryHandler(gestisci_plant_graph, pattern=r"^plantgraph:.*:.*:.*"))
    # Facciamo partire il bot in ascolto continuo (Polling)
    print("[Bot Telegram] In esecuzione (Separato). Premi CTRL+Z per fermarlo.")
    app.run_polling()

if __name__ == "__main__":
    main()