from sqlalchemy import Column, Integer, String, Float, Boolean, ForeignKey
from sqlalchemy.orm import relationship
from .base import Base

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
    
    # Relazione: una pianta può essere associata a più nodi (Main e Backup)
    nodes = relationship("NodeSettingsModel", back_populates="plant")

class NodeSettingsModel(Base):
    __tablename__ = "node_settings"
    # Usiamo il MAC address come ID primario
    id = Column(String, primary_key=True) 
    name = Column(String, nullable=False)
    is_backup = Column(Boolean, default=False)
    timer = Column(Integer, default=30)
    is_running = Column(Boolean, default=False)
    # Chiave esterna verso la pianta
    plant_id = Column(String, ForeignKey("plants.id"))
    plant = relationship("PlantModel", back_populates="nodes")