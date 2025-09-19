#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 9100
#define MAX_CLIENTS 10
#define MAX_GAMES 10

typedef enum { NUOVA, IN_ATTESA, IN_CORSO, TERMINATA } GameState;

typedef struct {
    int id[256][2];
    SOCKET owner;
    SOCKET challenger;
    SOCKET pendingChallenger;
    GameState state;
    char board[3][3];  // Tabellone
    int turn;          // 0=owner, 1=challenger
} Game;

Game games[MAX_GAMES];
int gameCount = 0;

// ----------------------------------------------------
// Utility
Game* findGameById(ing gameId) {
	int i;
    for (i = 0; i < gameCount; i++) {
        if (games[i].id[i][0] == gameId) {  // l’ID della partita
            return &games[i];
        }
    }
    return NULL;
}

void tutorialGame(SOCKET client){
	send(client, "Per eseguire delle mosse durante una partita scrivi il numero della mossa e premi invio!\n", 89, 0);
	send(client, "Le mosse si basano su questo schema...\n\n", 39, 0);
	send(client, " 1 | 2 | 3 \n", 12, 0);
	send(client, " 4 | 5 | 6 \n", 12, 0);
	send(client, " 7 | 8 | 9 \n", 12, 0);
	
}

// Stampa e invia tabellone ai due giocatori
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

// Controlla vittoria per simbolo X o O
int checkVictory(Game *g, char symbol) {
	int i;
    for(i=0;i<3;i++) {
        if (g->board[i][0]==symbol && g->board[i][1]==symbol && g->board[i][2]==symbol) return 1;
        if (g->board[0][i]==symbol && g->board[1][i]==symbol && g->board[2][i]==symbol) return 1;
    }
    if (g->board[0][0]==symbol && g->board[1][1]==symbol && g->board[2][2]==symbol) return 1;
    if (g->board[0][2]==symbol && g->board[1][1]==symbol && g->board[2][0]==symbol) return 1;
    return 0;
}

// Controlla pareggio
int checkDraw(Game *g) {
	int r, c;
    for (r=0;r<3;r++)
        for (c=0;c<3;c++)
            if (g->board[r][c]==' ') return 0;
    return 1;
}

// Notifica risultato partita
void notifyPlayers(Game *g, char result) {
    if(result=='X') {
        send(g->owner,"Hai vinto! Puoi creare una nuova partita con CREATE o unirti al matchmaking con JOIN...\n",88,0);
        send(g->challenger,"Hai perso! Puoi creare una nuova partita con CREATE o unirti al matchmaking con JOIN...\n",88,0);
    } else if(result=='O') {
        send(g->owner,"Hai perso! Puoi creare una nuova partita con CREATE o unirti al matchmaking con JOIN...\n",88,0);
        send(g->challenger,"Hai vinto! Puoi creare una nuova partita con CREATE o unirti al matchmaking con JOIN...\n",88,0);
    } else {
        send(g->owner,"Pareggio!\n",10,0);
        send(g->challenger,"Pareggio!\n",10,0);
    }
}

// ----------------------------------------------------
// Creazione partita
void createGame(SOCKET client) {
	int r, c;
	
    
    if (gameCount >= MAX_GAMES) {
        send(client, "Numero massimo partite raggiunto.\n", 34, 0);
        return;
    }
	int current = gameCount+1;
	printf("Un client ha creato la partita [ID = %d]", current);
	
    Game *g = &games[current-1];
    g->id[current][0] = current;
    g->id[current][1] = 1;
    g->owner = client;
    g->challenger = INVALID_SOCKET;
    g->pendingChallenger = INVALID_SOCKET;
    g->state = IN_ATTESA;
    g->turn = 0;
	
    for(r=0;r<3;r++)
    	for(c=0;c<3;c++)
    		g->board[r][c]=' ';

    gameCount++;

    char msg[128];
    sprintf(msg, "La tua partita [ID = %d] e' stata creata. In attesa di un avversario...\n", g->id[current][0]);
    send(client, msg, strlen(msg), 0);
		
		
    
}

// ----------------------------------------------------
// Richiesta JOIN
void joinGame(SOCKET client) {
	int i, j;
    Game *g = findGame();
    if (!g || g->state != IN_ATTESA) {
        send(client, "Partita non disponibile.\n", 25, 0);
        return;
    }
    if (g->owner == client) {
        send(client, "Non puoi unirti alla tua stessa partita.\n", 41, 0);
        return;
    }

    g->pendingChallenger = client;
    printf("Partita trovata!");
    char msg[128];
    sprintf(msg, "Un giocatore vuole unirsi alla tua partita, Rispondi con SI o NO.\n");
    send(g->owner, msg, strlen(msg), 0);
    send(client, "Richiesta inviata, attendi risposta...\n", 39, 0);
}

// ----------------------------------------------------
// Risposta owner YES/NO
void handleOwnerResponse(SOCKET client, int gameId, int accept) {
    Game *g = findGameById(gameId);
    if (!g || g->owner != client || g->pendingChallenger==INVALID_SOCKET) return;

    SOCKET challenger = g->pendingChallenger;
    g->pendingChallenger = INVALID_SOCKET;

    if (accept) {
        g->challenger = challenger;
        g->state = IN_CORSO;
        send(challenger, "Richiesta ACCETTATA! La partita inizia. E' il turno dell'avversario.\n", 69, 0);
        send(client, "Hai accettato la richiesta. La partita inizia! E' il tuo turno!\n", 64, 0);
        sendBoard(g);
    } else {
        send(challenger, "Richiesta RIFIUTATA.\n", 22, 0);
        send(client, "Hai rifiutato la richiesta.\n", 28, 0);
    }
}

int playCheck(int x, int* r, int* c){
	switch(x)
	{
		case 1:
			*r = 0;
			*c = 0;
			break;
		case 2:
			*r = 0;
			*c = 1;
			break;
		case 3:
			*r = 0;
			*c = 2;
			break;
		case 4:
			*r = 1;
			*c = 0;
			break;
		case 5:
			*r = 1;
			*c = 1;
			break;
		case 6:
			*r = 1;
			*c = 2;
			break;
		case 7:
			*r = 2;
			*c = 0;
			break;
		case 8:
			*r = 2;
			*c = 1;
			break;
		case 9:
			*r = 2;
			*c = 2;
			break;
	}
}
// ----------------------------------------------------
void playTurn(Game *g, SOCKET currentPlayer, char symbol, char *buffer) {
    int x;
	int r, c;
    // interpreta la mossa
    if (sscanf(buffer, "%d", &x) != 1) {
        send(currentPlayer, "Comando non valido. Usa: <scelta>\n", 39, 0);
        return;
    }
	
    if (x <= 0 || x > 9) {
        send(currentPlayer, "Mossa non valida.\n", 19, 0);
        return;
    }
	
	playCheck(x, &r, &c);
	
	if(g->board[r][c] == ' '){
		printf("goodtest - ");
		g->board[r][c] = symbol;
	}
	else{
		printf("test");
		send(currentPlayer, "Mossa non valida.\n", 19, 0);
        return;
	}


    if (checkVictory(g, symbol)) {
    	sendBoard(g);
        g->state = TERMINATA;
        notifyPlayers(g, symbol);

        resetGame(g);
        printf("Partita terminata: vittoria di %c. Slot resettato.\n", symbol);

        return;
    }
    if (checkDraw(g)) {
    	sendBoard(g);
        g->state = TERMINATA;
        notifyPlayers(g, 'D');

        resetGame(g);
        printf("Partita terminata: pareggio. Slot resettato.\n");
        
        return;
    }

    // passa turno
    g->turn = 1 - g->turn;
    sendBoard(g);
}


// Resetta la partita per un nuovo gioco
void resetGame(Game *g) {
    g->owner = INVALID_SOCKET;
    g->challenger = INVALID_SOCKET;
    g->pendingChallenger = INVALID_SOCKET;
    g->state = IN_ATTESA; // oppure NUOVA
    g->turn = 0;
    for (int r=0; r<3; r++){
        for (int c=0; c<3; c++){
            g->board[r][c] = ' ';
        }
    }
}


// ----------------------------------------------------
int main() {
	int i, g;
    WSADATA wsa;
    SOCKET serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    int addrlen = sizeof(clientAddr);
    fd_set readfds;
    SOCKET clients[MAX_CLIENTS]={0};

    if(WSAStartup(MAKEWORD(2,2),&wsa)!=0){printf("WSAStartup fallita\n");return 1;}
    serverSocket=socket(AF_INET,SOCK_STREAM,0);
    if(serverSocket==INVALID_SOCKET){printf("Errore socket\n");return 1;}

    serverAddr.sin_family=AF_INET;
    serverAddr.sin_addr.s_addr=INADDR_ANY;
    serverAddr.sin_port=htons(PORT);

    if(bind(serverSocket,(struct sockaddr*)&serverAddr,sizeof(serverAddr))<0){printf("Bind fallito\n");return 1;}
    if(listen(serverSocket,3)<0){printf("Listen fallita\n");return 1;}

    printf("Server Tris avviato su porta %d\n",PORT);

    while(1){
        FD_ZERO(&readfds);
        FD_SET(serverSocket,&readfds);

        for(i=0;i<MAX_CLIENTS;i++) if(clients[i]>0) FD_SET(clients[i],&readfds);

        if(select(0,&readfds,NULL,NULL,NULL)==SOCKET_ERROR){printf("Select error\n");break;}

        if(FD_ISSET(serverSocket,&readfds)){
            clientSocket=accept(serverSocket,(struct sockaddr*)&clientAddr,&addrlen);
            if(clientSocket!=INVALID_SOCKET){
                printf("Nuovo client connesso.\n");
                for(i=0;i<MAX_CLIENTS;i++){
                    if(clients[i]==0){clients[i]=clientSocket;break;}
                }
                send(clientSocket,"Benvenuto!\n",11,0);
            }
        }

        for(i=0;i<MAX_CLIENTS;i++){
            SOCKET s=clients[i];
            if(s==0) continue;
            if(FD_ISSET(s,&readfds)){
                char buffer[1024];
                int valread=recv(s,buffer,sizeof(buffer)-1,0);
                if(valread<=0){
                    closesocket(s);
                    clients[i]=0;
                    printf("Client disconnesso.\n");
                    continue;
                }
                buffer[valread]='\0';

                // Comandi
                if(strncmp(buffer,"CREATE",6)==0) createGame(s);
                else if(strncmp(buffer,"JOIN",4)==0) joinGame(s);
                else if(strncmp(buffer,"SI",2) == 0 || strncmp(buffer,"NO",2) == 0) {
                    // legge l'ID della partita dopo lo spazio
                    int gameId = atoi(buffer + 3);  // es. "SI 3" o "NO 3"
                    handleOwnerResponse(s, gameId, (buffer[0]=='S') ? 1 : 0);
                }
                else if(strncmp(buffer,"TUTORIAL",8)==0) tutorialGame(s);
                else {
                    // Controlla se il client � in partita e se � il suo turno
                    for(g=0;g<gameCount;g++){
                        Game *game=&games[g];
                        if(game->state==IN_CORSO && (s==game->owner || s==game->challenger)){
                            SOCKET currentPlayer=(game->turn==0)?game->owner:game->challenger;
                            char symbol=(game->turn==0)?'X':'O';
                            if(s==currentPlayer) playTurn(game,currentPlayer,symbol,buffer);
                        }
                    }
                }
            }
        }
    }

    closesocket(serverSocket);
    WSACleanup();
    return 0;
}
