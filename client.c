#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <windows.h>

#pragma comment(lib, "ws2_32.lib")

#define PORT 9100
#define SERVER_IP "127.0.0.1"

typedef enum { MENU, IN_PARTITA, FINE_PARTITA } ClientState;

SOCKET sock;
ClientState clientState = MENU;

// Thread che riceve messaggi dal server
DWORD WINAPI recvThread(LPVOID lpParam) {
    char buffer[1024];
    int valread;

    while (1) {
        valread = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (valread <= 0) {
            printf("\nServer disconnesso.\n");
            exit(0);
        }
        buffer[valread] = '\0';
        printf("%s", buffer);

        // Controllo messaggi speciali del server → cambio stato
        if (strstr(buffer, "Hai vinto!") || strstr(buffer, "Hai perso!") ||
            strstr(buffer, "Pareggio")   || strstr(buffer, "Partita terminata")) {
            clientState = FINE_PARTITA;
        }
        else if (strstr(buffer, "La partita") || strstr(buffer, "inizia")) {
            clientState = IN_PARTITA;
        }
    }
    return 0;
}


int main() {
    WSADATA wsa;
    struct sockaddr_in serv_addr;
    char buffer[1024];

    if(WSAStartup(MAKEWORD(2,2), &wsa) != 0) {
        printf("Errore Winsock\n");
        return 1;
    }

    sock = socket(AF_INET, SOCK_STREAM, 0);
    if(sock == INVALID_SOCKET) {
        printf("Errore creazione socket\n");
        WSACleanup();
        return 1;
    }

    memset(&serv_addr, 0, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);
    inet_pton(AF_INET, SERVER_IP, &serv_addr.sin_addr);

    if(connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("Connessione fallita\n");
        closesocket(sock);
        WSACleanup();
        return 1;
    }

    printf("Connesso al server %s:%d\n", SERVER_IP, PORT);
    printf("Comandi disponibili:\n");
	printf("CREATE: Crea una nuova partita\n");
	printf("JOIN: Accedi al matchmaking per unirti a una partita disponibile\n");
	printf("SI: Accetta la richiesta\n");  
	printf("NO: Rifiuta la richiesta\n");  
	printf("<numero>: Durante una partita, seleziona la tua mossa\n");
	printf("TUTORIAL: Il sistema ti spiegher� come selezionare la mossa giusta\n\n");

    // Thread per ricevere messaggi dal server
    CreateThread(NULL, 0, recvThread, NULL, 0, NULL);

    while(1) {
        if(fgets(buffer, sizeof(buffer), stdin) != NULL) {
            // Rimuovi il newline finale
            size_t len = strlen(buffer);
            if(len>0 && buffer[len-1]=='\n') buffer[len-1]='\0';
            send(sock, buffer, strlen(buffer), 0);
        }
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}


