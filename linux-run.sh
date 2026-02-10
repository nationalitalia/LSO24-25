#!/bin/bash

# Pulizia totale (Server e Client precedenti)
echo "Pulizia ambiente Docker..."
docker compose down

# hiede il numero di client
read -p "Quanti client vuoi aprire? " NUM_CLIENTS

if ! [[ "$NUM_CLIENTS" =~ ^[0-9]+$ ]] || [ "$NUM_CLIENTS" -le 0 ]; then
    echo "Errore: inserisci un numero valido."
    exit 1
fi

# Avvia il server
echo "Avvio del server in background..."
docker compose up -d server

# Avvia i client in modalità Detached (-d)
# Questo sostituisce kgx Docker avvia il processo ma non apre un terminale
for ((i=1;i<=NUM_CLIENTS;i++)); do
    echo "Avvio container client $i..."
    docker compose run -d --rm client
done

echo "Tutti i client sono stati avviati!"
echo "I log del server sono visibili con: docker logs -f server"