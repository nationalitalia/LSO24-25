#!/bin/bash

# Chiede all'utente quanti client aprire
read -p "Quanti client vuoi aprire? " NUM_CLIENTS

# Verifica che l'input sia un numero maggiore di 0
if ! [[ "$NUM_CLIENTS" =~ ^[0-9]+$ ]] || [ "$NUM_CLIENTS" -le 0 ]; then
    echo "Per favore inserisci un numero valido maggiore di 0."
    exit 1
fi

# Avvia il server in background
echo "Avvio del server..."
docker compose up -d server

# Funzione per aprire un client interattivo
start_client() {
  CLIENT_NUM=$1
  echo "Avvio client $CLIENT_NUM..."
  # Avvia il container client in modalità interattiva e collegato alla rete del server
  # Cambio di comando per aprire il programma a causa di un'aggiornamento di gnome Nobara
  kgx -e bash -c "docker compose run --rm --service-ports client; exec bash" &
}

# Avvia i client
for ((i=1;i<=NUM_CLIENTS;i++)); do
  start_client $i
done

echo "Server e client avviati!"
echo "Per fermare tutto: docker-compose down"
