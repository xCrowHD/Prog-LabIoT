from sqlalchemy import create_engine, Column, Integer, String, Float
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker
import re

Base = declarative_base()

# Definiamo lo Schema (la tabella)
class PlantModel(Base):
    __tablename__ = "plants"
    id = Column(String, primary_key=True)
    name = Column(String, nullable=False)
    img_path = Column(String)
    temp_min = Column(Float)
    temp_max = Column(Float)
    hum_min = Column(Float)
    hum_max = Column(Float)
    light_min = Column(Float)
    light_max = Column(Float)

class DatabaseManager:
    def __init__(self, db_url="sqlite:///./plants.db"):
        # Crea il motore e la sessione
        self.engine = create_engine(db_url, connect_args={"check_same_thread": False})
        self.SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=self.engine)
        
        # Crea le tabelle se non esistono
        Base.metadata.create_all(bind=self.engine)
        print("[DB] Database e tabelle pronti.")

    def add_plant(self, name, img_path, t_min, t_max, h_min, h_max, l_min, l_max):
        session = self.SessionLocal()
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
        session = self.SessionLocal()
        plants = session.query(PlantModel).all()
        session.close()
        return plants
    
    def delete_plant_by_id(self, plant_id):
        session = self.SessionLocal()
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
        session = self.SessionLocal()
        plant = session.query(PlantModel).offset(position).first()
        session.close()
        return plant
    
    def get_plant_by_id(self, plant_id):
        session = self.SessionLocal()
        plant = session.query(PlantModel).filter(PlantModel.id == plant_id).first()
        session.close()
        return plant
    
    def get_plants_count(self):
        session = self.SessionLocal()
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
db_manager = DatabaseManager()