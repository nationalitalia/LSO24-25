# Assicurati di essere nella cartella del progetto
# Controlla che Docker Desktop sia in esecuzione

# Numero di client da avviare
$clientCount = Read-Host "Quanti client vuoi avviare?" 

Write-Host "=== Avvio del gioco con Docker Compose ===" -ForegroundColor Cyan

# Ferma eventuali container esistenti
Write-Host "Rimuovo eventuali container vecchi..." -ForegroundColor Yellow
docker-compose down

# Costruzione dei container
Write-Host "Costruzione dei container..." -ForegroundColor Yellow
docker-compose build

# Avvio del server in background
Write-Host "Avvio del server..." -ForegroundColor Green
Start-Process powershell -ArgumentList "-NoExit", "-Command", "docker-compose up server"

# Attesa breve per assicurarsi che il server parta
Start-Sleep -Seconds 3

# Avvio dei client
for ($i = 1; $i -le $clientCount; $i++) {
    Write-Host "Avvio del client $i..." -ForegroundColor Green
    Start-Process powershell -ArgumentList "-NoExit", "-Command", "docker-compose run --rm client"
}
