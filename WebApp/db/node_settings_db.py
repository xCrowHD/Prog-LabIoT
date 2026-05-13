from .models import NodeSettingsModel
from .base import init_db, SessionLocal


class SettingsDatabaseManager:
    def __init__(self):
        init_db()
        
    def get_node_settings_by_id(self, node_id):
        """Restituisce l'oggetto NodeSettingsModel basato sull'ID (MAC Address)."""
        session = SessionLocal()
        try:
            # Cerchiamo il nodo nel database
            node = session.query(NodeSettingsModel).filter(NodeSettingsModel.id == node_id).first()
            return node 
        except Exception as e:
            print(f"[DB-SETTINGS] Errore durante il recupero del nodo {node_id}: {e}")
            return None
        finally:
            session.close()
