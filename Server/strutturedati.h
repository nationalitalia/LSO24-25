#ifndef STRUTTUREDATI_H
#define STRUTTUREDATI_H

#include "winsock_posix_compat.h"

#define PORT 9100
#define MAX_CLIENTS 10
#define MAX_GAMES 10

typedef enum { NUOVA, IN_ATTESA, IN_CORSO, TERMINATA } GameState;

typedef struct {
int id[256][2];
sock_t owner; // Sostituito con il tipo compatibile
sock_t challenger; // Sostituito con il tipo compatibile
sock_t pendingChallenger; // Sostituito con il tipo compatibile
GameState state;
char board[3][3];
int turn;
} Game;

// Variabili globali
extern Game games[MAX_GAMES];
extern int gameCount;
extern sock_t clients[MAX_CLIENTS]; // Sostituito con il tipo compatibile

#endif
