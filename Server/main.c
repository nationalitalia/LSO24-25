#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "winsock_posix_compat.h"
#include "strutturedati.h"
#include "funzioni.h"

// Variabili globali richieste da funzioni.c

sock_t clients[MAX_CLIENTS] = {0};
Game games[MAX_GAMES] = {0};
int gameCount = 0;


int main() {
    #ifdef _WIN32
        WSADATA wsa;
        if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
            printf("Errore WSAStartup\n");
            return 1;
        }
    #endif

    sock_t serverSocket, clientSocket;
    struct sockaddr_in serverAddr, clientAddr;
    socklen_t addrlen = sizeof(clientAddr);
    fd_set readfds;
    int max_sd, activity;

    // Creazione socket server
    serverSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (serverSocket < 0) { printf("Errore creazione socket\n"); return 1; }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = INADDR_ANY;
    serverAddr.sin_port = htons(PORT);

    if (bind(serverSocket, (struct sockaddr*)&serverAddr, sizeof(serverAddr)) < 0) {
        printf("Bind fallito\n"); return 1;
    }

    if (listen(serverSocket, MAX_CLIENTS) < 0) {
        printf("Listen fallita\n"); return 1;
    }

    printf("Server Tris avviato su porta %d\n", PORT);

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(serverSocket, &readfds);
        max_sd = serverSocket;

        for (int i = 0; i < MAX_CLIENTS; i++) {
            if (clients[i] > 0) {
                FD_SET(clients[i], &readfds);
                if (clients[i] > max_sd) max_sd = clients[i];
            }
        }

        #ifdef _WIN32
            activity = select(0, &readfds, NULL, NULL, NULL);
            if (activity == SOCKET_ERROR) { printf("Select error\n"); break; }
        #else
            activity = select(max_sd + 1, &readfds, NULL, NULL, NULL);
            if (activity < 0) { printf("Select error\n"); break; }
        #endif

        // Nuovo client
        if (FD_ISSET(serverSocket, &readfds)) {
            clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddr, &addrlen);
            if (clientSocket < 0) { printf("Accept fallito\n"); continue; }

            printf("Nuovo client connesso.\n");
            for (int i = 0; i < MAX_CLIENTS; i++) {
                if (clients[i] == 0) { clients[i] = clientSocket; break; }
            }

            send(clientSocket, "Benvenuto!\n", 11, 0);
        }

        // Client esistenti
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sock_t s = clients[i];
            if (s == 0) continue;

            if (FD_ISSET(s, &readfds)) {
                char buffer[1024];
                int valread = recv(s, buffer, sizeof(buffer) - 1, 0);
                if (valread <= 0) {
                    closesocket(s);
                    clients[i] = 0;
                    printf("Client disconnesso.\n");
                    continue;
                }
                buffer[valread] = '\0';

                // Comandi
                if (strncmp(buffer, "CREATE", 6) == 0) createGame(s);
                else if (strncmp(buffer, "JOIN", 4) == 0) joinGame(s);
                else if(strncmp(buffer,"SI",2)==0) handleOwnerResponse(s,1);
                else if(strncmp(buffer,"NO",2)==0) handleOwnerResponse(s,0);
                else if (strncmp(buffer, "TUTORIAL", 8) == 0) tutorialGame(s);
                else {
                    // Turni di gioco
                    for (int g = 0; g < gameCount; g++) {
                        Game *game = &games[g];
                        if (game->state == IN_CORSO && (s == game->owner || s == game->challenger)) {
                            sock_t currentPlayer = (game->turn == 0) ? game->owner : game->challenger;
                            char symbol = (game->turn == 0) ? 'X' : 'O';
                            if (s == currentPlayer) playTurn(game, currentPlayer, buffer, symbol);
                        }
                    }
                }
            }
        }
    }

    closesocket(serverSocket);
    #ifdef _WIN32
        WSACleanup();
    #endif

    return 0;
}

