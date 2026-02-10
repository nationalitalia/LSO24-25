Write-Host "=== Avvio del gioco Tris Online ===" -ForegroundColor Cyan

# Pulizia totale
Write-Host "Rimuovo eventuali container vecchi..." -ForegroundColor Yellow
docker-compose down

# Chiude i client Java invisibili ancora aperti
Write-Host "Pulizia processi Java redidui... " -ForegroundColor Yellow
Stop-Process -Name "javaw" -ErrorAction SilentlyContinue

# Build e Avvio del Server in background (-d)
Write-Host "Compilazione e avvio del Server C..." -ForegroundColor Yellow
# Usiamo -d (detached) per non bloccare lo script qui
docker-compose up --build -d server

# Attesa per il boot del server
Start-Sleep -Seconds 2

# Chiedi quanti client avviare
$clientCount = Read-Host "Quanti client vuoi avviare?" 

if (-not ($clientCount -as [int]) -or $clientCount -le 0) {
    Write-Host "Errore: inserisci un numero valido." -ForegroundColor Red
    exit
}

# Compilazione del Client Java (UNA VOLA SOLA)
Write-Host "Compilazione Client Java..." -ForegroundColor Yellow
cd Client
javac TrisClient.java
if ($LASTEXITCODE -ne 0) { 
    Write-Host "Errore durante la compilazione Java!" -ForegroundColor Red
    cd ..
    exit 
}
cd ..

# Avvio dei Client locali
for ($i = 1; $i -le $clientCount; $i++) {
    Write-Host "Avvio del client locale $i (Senza terminale)..." -ForegroundColor Green
    # Lanciamo il processo java direttamente
    Start-Process "javaw" -ArgumentList "-cp Client TrisClient" -WindowStyle Hidden
}

Write-Host "=== Tutti i client sono stati avviati! ===" -ForegroundColor Cyan
Write-Host "Per vedere i log del server usa: docker logs -f my_server" -ForegroundColor Gray