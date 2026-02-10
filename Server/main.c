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
    				char* namePart = buffer + 5;
                    namePart[strcspn(namePart, "\r\n ")] = 0; // Toglie \n e spazi finali
                    strncpy(connected_clients[i].name, namePart, 31);
                    connected_clients[i].name[31] = '\0';
                    printf("Socket %d registrato come: [%s]\n", (int)s, connected_clients[i].name);
                    fflush(stdout);
				}
                else if (strncmp(buffer, "CREATE", 6) == 0) createGame(s);
                else if (strncmp(buffer, "JOIN ", 5) == 0) { int id_richiesto = atoi(buffer + 5); joinGameById(s, id_richiesto); }
                else if (strncmp(buffer, "LIST", 4) == 0) availableGames(s);
                else if (strncmp(buffer, "DELETE ", 7) == 0) { int id_richiesto = atoi(buffer + 7); deleteGame(s, id_richiesto); }
                else if (strncmp(buffer, "DELETE", 6) == 0) ownAvailableGames(s);
               else if (strncmp(buffer, "RIVINCITA_SI", 12) == 0) {
                    for (int i = 0; i < gameCount; i++) {
                        if ((games[i].owner == s || games[i].challenger == s) && games[i].state == TERMINATA) {
                            
                            // Segniamo chi ha dato il consenso
                            if (s == games[i].owner) {
                                games[i].ownerRivincita = 1;
                            } else {
                                games[i].challengerRivincita = 1;
                            }

                            // Controlliamo se entrambi hanno accettato
                            if (games[i].ownerRivincita && games[i].challengerRivincita) {
                            	
                            	if(ownerHasActiveGame(games[i].owner)){
                            		send(games[i].challenger, "L'avversario non e' piu' disponibile!\n", 39, 0);
                            		removeGame(i);
                            		break;
								}
								else if(ownerHasActiveGame(games[i].challenger)){
                            		send(games[i].owner, "L'avversario non e' piu' disponibile!\n", 39, 0);
                            		removeGame(i);
                            		break;
								}
								else{
                                resetGame(&games[i]); // Pulisce scacchiera e resetta i flag Ready a 0
                                
                                sendBoard(&games[i]);
                                send(games[i].owner, "Entrambi hanno accettato! Inizia l'owner.\n", 43, 0);
                                send(games[i].challenger, "Entrambi hanno accettato! Inizia l'owner.\n", 43, 0);
                            	}
                            } else {
                                // Avvisa solo chi ha premuto SI
                                send(s, "Hai accettato la rivincita. In attesa dell'avversario...\n", 58, 0);
                                
                                // Opzionale: avvisa l'altro che il compagno sta aspettando
                                sock_t altro = (s == games[i].owner) ? games[i].challenger : games[i].owner;
                                send(altro, "L'avversario vuole la rivincita! Accetti?\n", 42, 0);
                            }
                            break;
                        }
                    }
                }
                else if (strncmp(buffer, "RIVINCITA_NO", 12) == 0) {
                    // Cercha la partita
                    for (int i = 0; i < gameCount; i++) {
                        if ((games[i].owner == s || games[i].challenger == s) && games[i].state == TERMINATA) {
                            
                            // Avvisa l'altro giocatore che la sfida è finita
                            send(games[i].owner, "Rivincita rifiutata. Partita chiusa.\n", 38, 0);
                            send(games[i].challenger, "Rivincita rifiutata. Partita chiusa.\n", 38, 0);
                            
                            // Rimove la partita 
                            removeGame(i); 
                            break;
                        }
                    }
                }

                else if (strncmp(buffer, "ACCETTA ", 8) == 0) {
                    char nomeTarget[32];
                    // Puliamo il buffer principale da newline prima di estrarre
                    buffer[strcspn(buffer, "\r\n")] = 0;

                    // Estraiamo il nome. %s si ferma al primo spazio.
                    if (sscanf(buffer + 8, "%s", nomeTarget) == 1) {
                        // Rimuoviamo forzatamente ogni residuo invisibile dal nome estratto
                        nomeTarget[strcspn(nomeTarget, "\r\n ")] = 0;

                        sock_t challengerSd = INVALID_SOCKET;
                        // Ricerca sicura nell'array
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (clients[j] != 0 && strcmp(connected_clients[j].name, nomeTarget) == 0) {
                                challengerSd = clients[j];
                                break;
                            }
                        }

                        if (challengerSd != INVALID_SOCKET) {
                            printf("Server: Owner %d accetta sfida da %s (socket %d)\n", (int)s, nomeTarget, (int)challengerSd);
                            handleOwnerResponse(s, 1, challengerSd);
                        } else {
                            // Debug utile per capire se il nome cercato è diverso da quello salvato
                            printf("Errore: Sfidante [%s] non trovato. Verificare registrazione nome.\n", nomeTarget);
                            send(s, "Errore: Sfidante non trovato o disconnesso.\n", 44, 0);
                        }
                    }
                }

                // --- GESTIONE RIFIUTA CON PULIZIA STRINGA ---
                else if (strncmp(buffer, "RIFIUTA ", 8) == 0) {
                    char nomeTarget[32];
                    buffer[strcspn(buffer, "\r\n")] = 0;

                    if (sscanf(buffer + 8, "%s", nomeTarget) == 1) {
                        nomeTarget[strcspn(nomeTarget, "\r\n ")] = 0;

                        sock_t challengerSd = INVALID_SOCKET;
                        for (int j = 0; j < MAX_CLIENTS; j++) {
                            if (clients[j] != 0 && strcmp(connected_clients[j].name, nomeTarget) == 0) {
                                challengerSd = clients[j];
                                break;
                            }
                        }

                        if (challengerSd != INVALID_SOCKET) {
                            printf("Server: Owner %d rifiuta sfida da %s\n", (int)s, nomeTarget);
                            handleOwnerResponse(s, 0, challengerSd);
                        }
                    }
                }
                else if(strncmp(buffer,"LEAVE",5)==0) handleDisconnection(s);
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

