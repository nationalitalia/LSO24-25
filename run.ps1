Write-Host "=== Avvio del gioco Tris Online ===" -ForegroundColor Cyan

# 1. Pulizia totale
Write-Host "Rimuovo eventuali container vecchi..." -ForegroundColor Yellow
docker-compose down

# 2. Build e Avvio del Server in background (-d)
Write-Host "Compilazione e avvio del Server C..." -ForegroundColor Yellow
# Usiamo -d (detached) per non bloccare lo script qui
docker-compose up --build -d server

# 3. Attesa per il boot del server
Start-Sleep -Seconds 2

# 4. Chiedi quanti client avviare
$clientCount = Read-Host "Quanti client vuoi avviare?" 

if (-not ($clientCount -as [int]) -or $clientCount -le 0) {
    Write-Host "Errore: inserisci un numero valido." -ForegroundColor Red
    exit
}

# 5. Compilazione del Client Java (UNA VOLA SOLA)
Write-Host "Compilazione Client Java..." -ForegroundColor Yellow
cd Client
javac TrisClient.java
if ($LASTEXITCODE -ne 0) { 
    Write-Host "Errore durante la compilazione Java!" -ForegroundColor Red
    cd ..
    exit 
}
cd ..

# 6. Avvio dei Client locali
for ($i = 1; $i -le $clientCount; $i++) {
    Write-Host "Avvio del client locale $i..." -ForegroundColor Green
    # Lanciamo il processo java direttamente
    $clientCommand = "cd Client; java TrisClient"
    Start-Process powershell -ArgumentList "-NoExit", "-Command", $clientCommand
}

Write-Host "=== Tutti i client sono stati avviati! ===" -ForegroundColor Cyan
Write-Host "Per vedere i log del server usa: docker logs -f my_server" -ForegroundColor Gray