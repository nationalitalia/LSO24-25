#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "winsock_posix_compat.h"
#include "strutturedati.h"
#include "funzioni.h"

// Variabili globali richieste da funzioni.c

sock_t clients[MAX_CLIENTS] = {0};
ClientInfo connected_clients[MAX_CLIENTS];
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
            if (clientSocket < 0) { printf("Accept fallito\n"); }
            else {
                printf("Nuovo client connesso al socket %d\n", (int)clientSocket);
                for (int i = 0; i < MAX_CLIENTS; i++) {
                    if (clients[i] == 0) { 
                        clients[i] = clientSocket; 
                        // Salvo anche nelle info per il nome
                        connected_clients[i].socket = clientSocket;
                        strcpy(connected_clients[i].name, "Anonimo");
                        break; 
                    }
                }
                send(clientSocket, "Benvenuto!\n", 11, 0);
            }
        }

        // Client esistenti
        for (int i = 0; i < MAX_CLIENTS; i++) {
            sock_t s = clients[i];
            if (s == 0) continue;

            if (FD_ISSET(s, &readfds)) {
                char buffer[1024];
                int valread = recv(s, buffer, sizeof(buffer) - 1, 0);
                
                if (valread <= 0) {
                    handleDisconnection(s);
                    closesocket(s);
                    clients[i] = 0; 
                    connected_clients[i].socket = 0;
                    strcpy(connected_clients[i].name, "Anonimo");
                    printf("Client rimosso.\n");
                    continue;
                }
                
                buffer[valread] = '\0';

                // Comandi
                if (strncmp(buffer, "NOME ", 5) == 0) {
    				strncpy(connected_clients[i].name, buffer + 5, 31);
    				connected_clients[i].name[strcspn(connected_clients[i].name, "\r\n")] = 0;
    				printf("Socket %d associato al nome: %s\n", (int)s, connected_clients[i].name);
    				fflush(stdout);
				}
                else if (strncmp(buffer, "CREATE", 6) == 0) createGame(s);
                else if (strncmp(buffer, "JOIN ", 5) == 0) { int id_richiesto = atoi(buffer + 5); joinGameById(s, id_richiesto); }
                else if (strncmp(buffer, "JOIN", 4) == 0) joinGame(s);
                else if (strncmp(buffer, "LIST", 4) == 0) availableGames(s);
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

