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

    def get_main_id_by_name(self, name: str):
        """Trova l'ID del nodo Main associato a un determinato nome."""
        session = SessionLocal()
        try:
            # Cerchiamo il nodo che ha lo stesso nome ma NON è un backup
            node = session.query(NodeSettingsModel).filter(
                NodeSettingsModel.name == name,
                NodeSettingsModel.is_backup == False
            ).first()
            return node.id if node else None
        finally:
            session.close()
    
    def ensure_node_exists(self, node_id):
        """
        Verifica se un nodo esiste nel DB. Se non esiste, lo crea con 
        impostazioni di default.
        """
        session = SessionLocal()
        try:
            # Cerchiamo il nodo
            node = session.query(NodeSettingsModel).filter(NodeSettingsModel.id == node_id).first()
            
            if not node:
                print(f"[DB] Nodo {node_id} mai visto prima. Registrazione in corso...")
                # Creiamo un nuovo record con valori di default
                new_node = NodeSettingsModel(
                id=node_id
                )
                session.add(new_node)
                session.commit()
                print(f"[DB] Nodo {node_id} registrato con successo.")
            else:
                # Il nodo esiste già, non facciamo nulla
                pass
                
        except Exception as e:
            session.rollback()
            print(f"[DB] Errore in ensure_node_exists: {e}")
        finally:
            session.close()
    
    def get_node_settings_by_id(self, node_id: str):
        """Recupera l'oggetto NodeSettings completo dal DB."""
        session = SessionLocal()
        try:
            return session.query(NodeSettingsModel).filter(NodeSettingsModel.id == node_id).first()
        finally:
            session.close()

    def get_backup_node_by_name(self, name: str):
        """Trova il nodo configurato come backup per un determinato nome."""
        session = SessionLocal()
        try:
            return session.query(NodeSettingsModel).filter(
                NodeSettingsModel.name == name,
                NodeSettingsModel.is_backup == True
            ).first()
        finally:
            session.close()

    def get_nodes_by_name(self, name):
        """Cerca i nodi che hanno un determinato nome."""
        session = SessionLocal()
        try:
            # Restituiamo tutti i nodi con quel nome per controllare i ruoli
            nodes = session.query(NodeSettingsModel).filter(NodeSettingsModel.name == name).all()
            return nodes
        finally:
            session.close()
    
    def update_node_plant(self, node_id, plant_id):
        """Aggiorna solo la pianta associata a un nodo."""
        session = SessionLocal()
        try:
            node = session.query(NodeSettingsModel).filter(NodeSettingsModel.id == node_id).first()
            if node:
                node.plant_id = plant_id
                session.commit()
                return True
            return False
        except Exception as e:
            session.rollback()
            print(f"[DB] Errore update_node_plant: {e}")
            return False
        finally:
            session.close()

    def update_node_running_state(self, node_id, is_running):
        """
        Aggiorna lo stato operativo (start-stop) di un nodo nel database.
        is_running: Boolean (True per START, False per STOP)
        """
        session = SessionLocal()
        try:
            # Cerchiamo il nodo nel database
            node = session.query(NodeSettingsModel).filter(NodeSettingsModel.id == node_id).first()
            
            if node:
                node.is_running = is_running
                session.commit()
                print(f"[DB] Stato running aggiornato per {node_id}: {is_running}")
                return True
            
            print(f"[DB] Impossibile aggiornare is_running: nodo {node_id} non trovato.")
            return False
            
        except Exception as e:
            session.rollback()
            print(f"[DB] Errore update_node_running_state: {e}")
            return False
        finally:
            session.close()
    
    def update_node_settings(self, node_id, name, is_backup, timer):
        """
        Aggiorna i parametri di configurazione del nodo.
        """
        session = SessionLocal()
        try:
            node = session.query(NodeSettingsModel).filter(NodeSettingsModel.id == node_id).first()
            
            if node:
                node.name = name
                node.is_backup = is_backup
                node.timer = timer # Assicuriamoci che sia un intero per il DB
                
                session.commit()
                print(f"[DB] Configurazione aggiornata per {node_id}: {name}, Backup={is_backup}, Timer={timer}")
                return True
            
            print(f"[DB] Errore: Nodo {node_id} non trovato per aggiornamento settings.")
            return False
            
        except Exception as e:
            session.rollback()
            print(f"[DB] Errore update_node_settings: {e}")
            return False
        finally:
            session.close()

# Istanza singola (Singleton) da importare ovunque
settings_db_manager = SettingsDatabaseManager()