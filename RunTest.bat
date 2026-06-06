@echo off
rem Compila il file di test inserendo l'output nella sottocartella Test
mkdir Test > nul 2>&1
g++ -std=c++20 ./Test/AlarmTest.cpp GreenhouseESP/*.cpp -o Test/AlarmTest.exe && (
    echo Compilazione riuscita! Esecuzione dei test...
    echo ----------------------------------------------
    rem Usiamo il backslash modificando / in \ per Windows
    ".\Test\AlarmTest.exe"
) || (
    echo ----------------------------------------------
    echo Errore di compilazione o di esecuzione. Test abortiti.
)