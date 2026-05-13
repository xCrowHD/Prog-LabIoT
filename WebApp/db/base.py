import os
from sqlalchemy import create_engine
from sqlalchemy.ext.declarative import declarative_base
from sqlalchemy.orm import sessionmaker

Base = declarative_base()

current_dir = os.path.dirname(os.path.abspath(__file__))
db_path = os.path.join(current_dir, "database.db")
DB_URL = f"sqlite:///{db_path}"

engine = create_engine(DB_URL, connect_args={"check_same_thread": False})
SessionLocal = sessionmaker(autocommit=False, autoflush=False, bind=engine)

def init_db():
    """Crea tutte le tabelle definite nei modelli se non esistono."""
    Base.metadata.create_all(bind=engine)
    print(f"[DB] Database inizializzato correttamente in: {db_path}")