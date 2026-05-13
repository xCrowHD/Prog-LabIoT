import re
from .models import PlantModel
from .base import init_db, SessionLocal


class PlantDatabaseManager:
    def __init__(self):
        init_db()

    def add_plant(self, name, img_path, t_min, t_max, h_min, h_max, l_min, l_max):
        session = SessionLocal()
        plant_id = self.generate_id(name) # Generiamo l'ID dal nome
        
        try:
            # Cerchiamo se esiste già una pianta con questo ID
            existing_plant = session.query(PlantModel).filter(PlantModel.id == plant_id).first()

            if existing_plant:
                # AGGIORNAMENTO (Overwrite)
                print(f"[DB] Pianta '{plant_id}' esistente. Aggiorno i valori.")
                existing_plant.name = name
                if img_path != None:
                    existing_plant.img_path = img_path
                existing_plant.temp_min = t_min
                existing_plant.temp_max = t_max
                existing_plant.hum_min = h_min
                existing_plant.hum_max = h_max
                existing_plant.light_min = l_min
                existing_plant.light_max = l_max
            else:
                # NUOVO INSERIMENTO
                print(f"[DB] Creo nuova pianta con ID: {plant_id}")
                new_plant = PlantModel(
                    id=plant_id, # Usiamo lo slug come ID primario
                    name=name, 
                    img_path=img_path,
                    temp_min=t_min, temp_max=t_max,
                    hum_min=h_min, hum_max=h_max,
                    light_min=l_min, light_max=l_max
                )
                session.add(new_plant)

            session.commit()
            return plant_id
            
        except Exception as e:
            session.rollback()
            print(f"[DB] Errore: {e}")
        finally:
            session.close()

    def get_all_plants(self):
        session = SessionLocal()
        plants = session.query(PlantModel).all()
        print(f"[DEBUG] Piante trovate nel DB: {len(plants)}")
        session.close()
        return plants
    
    def delete_plant_by_id(self, plant_id):
        session = SessionLocal()
        try:
            # Cerchiamo la pianta
            plant = session.query(PlantModel).filter(PlantModel.id == plant_id).first()
            
            if plant:
                session.delete(plant)
                session.commit()
                return True
            
            return False
        except Exception as e:
            session.rollback()
            print(f"Errore durante l'eliminazione: {e}")
            return False
        finally:
            session.close()

    def get_plant_by_position(self, position):
        session = SessionLocal()
        plant = session.query(PlantModel).offset(position).first()
        session.close()
        return plant
    
    def get_plant_by_id(self, plant_id):
        session = SessionLocal()
        plant = session.query(PlantModel).filter(PlantModel.id == plant_id).first()
        session.close()
        return plant
    
    def get_plants_count(self):
        session = SessionLocal()
        # Esegue una query di conteggio direttamente sul database
        count = session.query(PlantModel).count()
        session.close()
        return count
    
    def generate_id(self, name):
        # Tutto minuscolo, sostituisce spazi con _, rimuove caratteri speciali
        name = name.lower().strip()
        name = re.sub(r'\s+', '_', name) # Spazi -> _
        name = re.sub(r'[^\w]', '', name) # Rimuove tutto ciò che non è lettera/numero/_
        return name

# Istanza singola (Singleton) da importare ovunque
plant_db_manager = PlantDatabaseManager()