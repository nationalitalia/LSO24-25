#ifndef FUNZIONI_H
#define FUNZIONI_H

#include "winsock_posix_compat.h"
#include "strutturedati.h"

// Thread per ricevere messaggi dal server
THREAD_RET_TYPE recvThread(THREAD_ARG_TYPE lpParam);

// Utility
Game* findGameById(int gameId);
void joinGameById(sock_t client, int id_richiesto);
Game* findAvailableGame();
void availableGames(sock_t client);
const char* getClientName(sock_t sd);

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
void handleOwnerResponse(sock_t owner, int accept, sock_t challenger);
int ownerHasActiveGame(sock_t client);
void cleanupEmptyGames(sock_t client);
void handleDisconnection(sock_t sd);
void ownAvailableGames(sock_t client);
void deleteGame(sock_t client, int gameId);

#endif

