@echo off

:: Apre il browser (questo DEVE usare start, altrimenti blocca il file bat)
start http://127.0.0.1:8000

:: Entra nella cartella della WebApp
cd WebApp

:: Esegue Python direttamente in questa finestra, senza aprirne altre
python -m uvicorn app:app --reload --host 127.0.0.1 --port 8000

pause