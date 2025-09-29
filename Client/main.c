#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <netdb.h>              // gethostbyname
#include <arpa/inet.h>          // inet_ntoa
#include "winsock_posix_compat.h"
#include "strutturedati.h"
#include "funzioni.h"

// Rimuovi #pragma comment(lib, "ws2_32.lib")

sock_t sock;
ClientState clientState = MENU;

int main() {
    #ifdef _WIN32
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Errore Winsock\n");
        return 1;
    }
    #endif

    struct sockaddr_in serv_addr;
    char buffer[1024];
    
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        printf("Errore creazione socket\n");
        #ifdef _WIN32
        WSACleanup();
        #endif
        return 1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // ?? Risolvi l'hostname "server" definito in docker-compose
    struct hostent *he = gethostbyname(SERVER_IP);
    if (he == NULL) {
        printf("Errore risoluzione DNS per %s\n", SERVER_IP);
        closesocket(sock);
        #ifdef _WIN32
        WSACleanup();
        #endif
        return 1;
    }
    memcpy(&serv_addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connessione fallita verso %s:%d\n", SERVER_IP, PORT);
        closesocket(sock);
        #ifdef _WIN32
        WSACleanup();
        #endif
        return 1;
    }

    printf("Connesso al server %s:%d\n", SERVER_IP, PORT);
    printf("Comandi disponibili:\n");
    printf("CREATE: Crea una nuova partita\n");
    printf("JOIN: Accedi al matchmaking per unirti a una partita disponibile\n");
    printf("SI: Accetta la richiesta\n");
    printf("NO: Rifiuta la richiesta\n");
    printf("<numero>: Durante una partita, seleziona la tua mossa\n");
    printf("TUTORIAL: Il sistema ti spieghera' come selezionare la mossa giusta\n\n");

    // Usa la macro per creare il thread di ricezione
    CREATE_RECEIVE_THREAD(&sock);

    while (1) {
        if (fgets(buffer, sizeof(buffer), stdin) != NULL) {
            size_t len = strlen(buffer);
            if (len > 0 && buffer[len-1] == '\n') buffer[len-1] = '\0';
            send(sock, buffer, strlen(buffer), 0);
        }
    }

    closesocket(sock);
    #ifdef _WIN32
    WSACleanup();
    #endif
    return 0;
}

