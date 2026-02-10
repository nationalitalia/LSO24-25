#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "winsock_posix_compat.h"
#include "strutturedati.h"
#include "funzioni.h"

int globalNextId = 1;
// ----------------------------------------------------
// Thread ricezione messaggi
THREAD_RET_TYPE recvThread(THREAD_ARG_TYPE lpParam) {
    sock_t sock = *(sock_t*)lpParam;
    char buffer[1024];
    int valread;

    while (1) {
        valread = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (valread <= 0) {
            printf("\nServer disconnesso.\n");
            return 0;
        }
        buffer[valread] = '\0';

        printf("%s", buffer);
    }

    return 0;
}


Game* findGameById(int gameId) {
    for (int i = 0; i < gameCount; i++) {
        if (games[i].id[0][0] == gameId) {
            return &games[i];
        }
    }
    return NULL;
}

// Recupera il nome di un giocatore partendo dal socket
const char* getClientName(sock_t sd) {
    for (int j = 0; j < MAX_CLIENTS; j++) {
        if (connected_clients[j].socket == sd && connected_clients[j].socket != 0) {
            return connected_clients[j].name;
        }
    }
    return "Anonimo";
}

// ----------------------------------------------------
// Trova la partita dato l'ID
void joinGameById(sock_t client, int id_richiesto) {
    // Usiamo la tua funzione per trovare la partita
    Game *g = findGameById(id_richiesto);

    if (g == NULL) {
        send(client, "Errore: ID partita non valido.\n", 31, 0);
        return;
    }

    if (g->state != IN_ATTESA) {
        send(client, "Errore: La partita non e' piu' disponibile.\n", 44, 0);
        return;
    }

    if (g->owner == client) {
        send(client, "Non puoi unirti alla tua stessa partita!\n", 42, 0);
        return;
    }

    const char* nomeSfidante = getClientName(client);
    
    char msg[256];
    sprintf(msg, "Il giocatore %s vuole unirsi alla tua partita [ID=%d]\n", nomeSfidante, id_richiesto);
    send(g->owner, msg, strlen(msg), 0);
    
    send(client, "Richiesta inviata all'owner, attendi...\n", 41, 0);
    printf("Client %d ha chiesto di unirsi alla partita %d\n", (int)client, id_richiesto);
}

// ----------------------------------------------------
// Trova partita disponibile per JOIN
Game* findAvailableGame() {
    for (int i = 0; i < gameCount; i++) {
        if (games[i].state == IN_ATTESA && games[i].owner != INVALID_SOCKET) {
            printf("Il Matchmaking ha trovato una partita disponibile (ID=%d)...\n", games[i].id[0][0]);
            return &games[i];
        }
    }
    return NULL;
}

// ----------------------------------------------------
// Trova partita dato owner (in attesa di risposta)
Game* findGameByOwner(sock_t owner) {
    for (int i = 0; i < gameCount; i++) {
        if (games[i].owner == owner && games[i].state == IN_ATTESA) {
            return &games[i];
        }
    }
    return NULL;
}

// ----------------------------------------------------
// Gestione owner con singola partita
int ownerHasActiveGame(sock_t client) {
    for (int i = 0; i < gameCount; i++) {
        if (games[i].state == IN_CORSO && games[i].owner == client || games[i].state == IN_CORSO && games[i].challenger == client) {
            return 1; // gi� in una partita attiva come owner
        }
    }
    return 0;
}

// ----------------------------------------------------
// Tutorial
void tutorialGame(sock_t client) {
    send(client, "Per eseguire delle mosse durante una partita scrivi il numero della mossa e premi invio!\n", 89, 0);
    send(client, "Le mosse si basano su questo schema...\n\n", 39, 0);
    send(client, " 1 | 2 | 3 \n", 12, 0);
    send(client, " 4 | 5 | 6 \n", 12, 0);
    send(client, " 7 | 8 | 9 \n", 12, 0);
}

void availableGames(sock_t client) {
    char msg[512] = "LIST_START\n";
    send(client, msg, strlen(msg), 0);
    
    int check = 0;
    for (int i = 0; i < gameCount; i++) {
    if (games[i].state == IN_ATTESA) {
        const char* nomeCreatore = "Anonimo"; 

        // Cerca il nome associato al socket dell'owner
        	for(int j = 0; j < MAX_CLIENTS; j++) {
            	if(connected_clients[j].socket == games[i].owner && connected_clients[j].socket != 0) {
            	    nomeCreatore = connected_clients[j].name;
            	    break;
            	}
        	}

        	char msg[256];
        	sprintf(msg, "GAME:%d - Sfida: %s\n", games[i].id[0][0], nomeCreatore);
        	send(client, msg, strlen(msg), 0);
        	check++;
    	}
	}
    
    if(check == 0) {
        send(client, "GAME:Nessuna partita disponibile\n", 33, 0);
    }
    
    send(client, "LIST_END\n", 9, 0);
}

void ownAvailableGames(sock_t client) {
    char msg[512] = "LIST_DELETE_START\n";
    send(client, msg, strlen(msg), 0);
    
    int check = 0;
    for (int i = 0; i < gameCount; i++) {
    if (games[i].state == IN_ATTESA && games[i].owner == client) {
        const char* nomeCreatore = "Anonimo"; 

        // Cerca il nome associato al socket dell'owner
        	for(int j = 0; j < MAX_CLIENTS; j++) {
            	if(connected_clients[j].socket == games[i].owner && connected_clients[j].socket != 0) {
            	    nomeCreatore = connected_clients[j].name;
            	    break;
            	}
        	}

        	char msg[256];
        	sprintf(msg, "OWNGAME:%d - Sfida: %s\n", games[i].id[0][0], nomeCreatore);
        	send(client, msg, strlen(msg), 0);
        	check++;
    	}
	}
    
    if(check == 0) {
        send(client, "GAME:Nessuna partita disponibile\n", 33, 0);
    }
    
    send(client, "LIST_DELETE_END\n", 16, 0);
}

// ----------------------------------------------------
// Invia tabellone
void sendBoard(Game *g) {
    char msg[128];
    sprintf(msg,
        "\nBoard:\n %c | %c | %c\n %c | %c | %c\n %c | %c | %c\n\n",
        g->board[0][0], g->board[0][1], g->board[0][2],
        g->board[1][0], g->board[1][1], g->board[1][2],
        g->board[2][0], g->board[2][1], g->board[2][2]
    );
    send(g->owner, msg, strlen(msg), 0);
    send(g->challenger, msg, strlen(msg), 0);
}

// ----------------------------------------------------
// Controlla vittoria
int checkVictory(Game *g, char symbol) {
    for (int i = 0; i < 3; i++) {
        if (g->board[i][0] == symbol && g->board[i][1] == symbol && g->board[i][2] == symbol) return 1;
        if (g->board[0][i] == symbol && g->board[1][i] == symbol && g->board[2][i] == symbol) return 1;
    }
    if (g->board[0][0] == symbol && g->board[1][1] == symbol && g->board[2][2] == symbol) return 1;
    if (g->board[0][2] == symbol && g->board[1][1] == symbol && g->board[2][0] == symbol) return 1;
    return 0;
}

// ----------------------------------------------------
// Controlla pareggio
int checkDraw(Game *g) {
    for (int r = 0; r < 3; r++){
        for (int c = 0; c < 3; c++){
            if (g->board[r][c] == ' ') return 0;
        }
    }
    return 1;
}

// ----------------------------------------------------
// Notifica risultato
void notifyPlayers(Game *g, char result) {
    if (result == 'X') {
        send(g->owner, "Hai vinto! Scegli se rigiocare...\n", 36, 0);
        send(g->challenger, "Hai perso! Scegli se rigiocare...\n", 36, 0);
    } else if (result == 'O') {
        send(g->owner, "Hai perso! Scegli se rigiocare...\n", 36, 0);
        send(g->challenger, "Hai vinto! Scegli se rigiocare...\n", 36, 0);
    } else {
        send(g->owner, "Pareggio! Scegli se rigiocare...\n", 35, 0);
        send(g->challenger, "Pareggio! Scegli se rigiocare...\n", 35, 0);
    }
}

// ----------------------------------------------------
// Converte scelta in coordinate
void playCheck(int x, int* r, int* c) {
    switch(x){
        case 1: *r=0; *c=0; break;
        case 2: *r=0; *c=1; break;
        case 3: *r=0; *c=2; break;
        case 4: *r=1; *c=0; break;
        case 5: *r=1; *c=1; break;
        case 6: *r=1; *c=2; break;
        case 7: *r=2; *c=0; break;
        case 8: *r=2; *c=1; break;
        case 9: *r=2; *c=2; break;
    }
}

// ----------------------------------------------------
// Turno giocatore
void playTurn(Game *g, sock_t currentPlayer, char *buffer, char symbol) {
    int x,r,c;

    if(sscanf(buffer,"%d",&x)!=1 || x<1 || x>9){
        send(currentPlayer,"Mossa non valida.\n",19,0);
        return;
    }

    playCheck(x,&r,&c);
    if(g->board[r][c]!=' '){
        send(currentPlayer,"Mossa non valida.\n",19,0);
        return;
    }

    g->board[r][c] = symbol;


    if (checkVictory(g, symbol)) {
        sendBoard(g);
        g->state = TERMINATA;
        notifyPlayers(g, symbol); 
        
        char *domanda = "RIVINCITA_DOMANDA Vuoi la rivincita?\n";
        send(g->owner, domanda, strlen(domanda), 0);
        send(g->challenger, domanda, strlen(domanda), 0);
        return;
    } 
    
    if (checkDraw(g)) {
        sendBoard(g);
        g->state = TERMINATA;
        notifyPlayers(g, 'D'); 
        
        char *domanda = "RIVINCITA_DOMANDA Vuoi la rivincita?\n";
        send(g->owner, domanda, strlen(domanda), 0);
        send(g->challenger, domanda, strlen(domanda), 0);
        return;
    }

    g->turn = 1 - g->turn;
    sendBoard(g);
}

// Funzione interna per rimuovere fisicamente una partita dall'array e scalare gli ID
void removeGame(int index) {
    if (index < 0 || index >= gameCount) return;

    // Compattamento dell'array globale games
    for (int j = index; j < gameCount - 1; j++) {
        games[j] = games[j + 1];
    }

    gameCount--; // Questo resetta il contatore per la prossima CREATE
    printf("Server: Partita rimanenti: %d\n", gameCount);
}

void deleteGame(sock_t client, int gameId) {
    int index = -1;

    // Cerca l'indice della partita
    for (int i = 0; i < gameCount; i++) {
        if (games[i].id[0][0] == gameId) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        send(client, "Errore: Partita non trovata.\n", 29, 0);
        return;
    }

    // Solo l'owner può eliminare 
    if (games[index].owner != client) {
        send(client, "Errore: Non puoi eliminare una partita non tua.\n", 48, 0);
        return;
    }

    // Si possono eliminare solo partite non ancora iniziate 
    if (games[index].state != IN_ATTESA) {
        send(client, "Errore: Partita gia' in corso o terminata.\n", 43, 0);
        return;
    }


    removeGame(index);

    send(client, "Partita eliminata correttamente.\n", 33, 0);
    printf("Server: Partita ID %d rimossa. Partite totali: %d\n", gameId, gameCount);
}

// ----------------------------------------------------
// Reset partita
void resetGame(Game *g) {
    g->state = IN_CORSO;
    g->turn = 0;
    g->ownerRivincita = 0;
    g->challengerRivincita = 0;
    
    for(int r=0;r<3;r++){
        for(int c=0;c<3;c++){
            g->board[r][c]=' ';
        }
    }
}

// ----------------------------------------------------
// Rimuove le partite IN_ATTESA senza challenger per un determinato owner
void cleanupEmptyGames(sock_t client) {
    for (int i = 0; i < gameCount; i++) {
        Game *g = &games[i];
        if (g->owner == client && g->state == IN_ATTESA) {
            // Shift all'elementi successivi a sinistra
            for (int j = i; j < gameCount - 1; j++) {
                games[j] = games[j + 1];
            }
            gameCount--; // decrementa il numero di partite
            i--; // ripeti il ciclo sul nuovo elemento in posizione i
        }
    }
}


// ----------------------------------------------------
// Creazione partita
void createGame(sock_t client) {
    if(gameCount>=MAX_GAMES){
        send(client,"Numero massimo partite raggiunto.\n",34,0);
        return;
    }

    int index = gameCount;
    Game *g = &games[index];
    g->id[0][0] = globalNextId++;
    g->owner = client;
    g->challenger = INVALID_SOCKET;
    g->state = IN_ATTESA;
    g->turn = 0;
    for(int r=0;r<3;r++)
        for(int c=0;c<3;c++)
            g->board[r][c]=' ';
    gameCount++;

    char msg[128];
    sprintf(msg,"La tua partita [ID=%d] e' stata creata.\n", g->id[0][0]);
    send(client,msg,strlen(msg),0);
}

// ----------------------------------------------------
// JOIN partita
void joinGame(sock_t client){
	cleanupEmptyGames(client);
	
    Game *g = findAvailableGame();
    if(!g){
        send(client,"Partita non disponibile.\n",25,0);
        return;
    }
	if (g->owner == client) {
        send(client, "Non puoi unirti alla tua stessa partita!\n", 42, 0);
        return;
    }
    const char* nomeSfidante = getClientName(client);
    char msg[256];
    sprintf(msg, "Il giocatore %s vuole unirsi alla tua partita [ID=%d], rispondi SI o NO.\n", nomeSfidante, g->id[0][0]);

    send(g->owner, msg, strlen(msg), 0);
    send(client,"Richiesta inviata, attendi risposta...\n",39,0);
}

// ----------------------------------------------------
// Risposta owner SI/NO
void handleOwnerResponse(sock_t owner, int accept, sock_t challenger) {
    Game *g = findGameByOwner(owner);
    
    if(!g) {
        send(owner, "Errore: Partita non trovata.\n", 29, 0);
        return;
    }

    if(accept) {
        // Controlla se l'owner è già occupato
        if (ownerHasActiveGame(owner)) {
            send(owner, "Hai gia' una partita in corso!\n", 31, 0);
            send(challenger, "Il giocatore e' gia' occupato.\n", 31, 0);
            return;
        }

        g->challenger = challenger;
        g->state = IN_CORSO;

        const char* nomeOwner = getClientName(owner);
        const char* nomeChallenger = getClientName(challenger);
        char msgNome[256];

        // Comunica all'Owner chi è lo sfidante
        sprintf(msgNome, "OPPONENT_IS %s\n", nomeChallenger);
        send(owner, msgNome, strlen(msgNome), 0);

        // Comunica allo Sfidante chi è l'Owner
        sprintf(msgNome, "OPPONENT_IS %s\n", nomeOwner);
        send(challenger, msgNome, strlen(msgNome), 0);
        
        send(owner, "Sfida accettata! Inizia tu.\n", 28, 0);
        send(challenger, "Sfida accettata! Attendi il turno.\n", 35, 0);
        sendBoard(g);
    } else {
        if (challenger != INVALID_SOCKET) {
        send(challenger, "La tua richiesta è stata rifiutata.\n", 37, 0);
        }
        send(owner, "Hai rifiutato la sfida.\n", 25, 0);
    }
}

//CASO DISCONNESSIONE O CRASH O COSE
void handleDisconnection(sock_t sd) {
    for (int i = 0; i < gameCount; i++) {
        Game *g = &games[i];
        
        if (g->owner == sd || g->challenger == sd) {
            sock_t opponent = (g->owner == sd) ? g->challenger : g->owner;

            if (g->state == IN_CORSO && opponent != INVALID_SOCKET) {
                const char* msg = "L'avversario ha abbandonato. Hai vinto!\n";
                send(opponent, msg, strlen(msg), 0);
            }
            
            printf("Rimozione partita ID %d a causa di disconnessione\n", g->id[0][0]);

            for(int j= i; j < gameCount - 1; j++){
                games[j] = games[j + 1];
            }
            gameCount--;

            break; // <--- Esci dal ciclo: abbiamo trovato la sua partita
        }
    }
    fflush(stdout);
}
