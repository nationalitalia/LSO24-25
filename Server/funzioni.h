#ifndef FUNZIONI_H
#define FUNZIONI_H

#include "winsock_posix_compat.h"
#include "strutturedati.h"

// Thread per ricevere messaggi dal server
THREAD_RET_TYPE recvThread(THREAD_ARG_TYPE lpParam);

// Utility
Game* findGameById(int gameId);
Game* findAvailableGame();

// Gioco
void tutorialGame(sock_t client);
void sendBoard(Game *g);
int checkVictory(Game *g, char symbol);
int checkDraw(Game *g);
void notifyPlayers(Game *g, char result);

// Turni e reset
void playCheck(int x, int* r, int* c);
void playTurn(Game *g, sock_t currentPlayer, char *buffer, char symbol);
void resetGame(Game *g);

// Gestione partite
void createGame(sock_t client);
void joinGame(sock_t client);
void handleOwnerResponse(sock_t client, int accept);

#endif

