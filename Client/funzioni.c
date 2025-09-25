#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "winsock_posix_compat.h"
#include "strutturedati.h"
#include "funzioni.h"

THREAD_RET_TYPE recvThread(THREAD_ARG_TYPE lpParam) {
    char buffer[1024];
    int valread;
    
    // Il cast è necessario per ottenere il socket in modo compatibile.
    sock_t sock = *(sock_t*)lpParam;

    while (1) {
        valread = recv(sock, buffer, sizeof(buffer) - 1, 0);
        if (valread <= 0) {
            printf("\nServer disconnesso.\n");
            // Sostituisci exit(0) con un'uscita più pulita del thread.
            return 0;
        }
        buffer[valread] = '\0';

        if (strstr(buffer, "È stata creata una nuova partita")) {
            printf("\n\n=== AVVISO SERVER ===\n%s\n=====================\n\n", buffer);
        } else {
            printf("%s", buffer);
        }

        if (strstr(buffer, "Hai vinto!") || strstr(buffer, "Hai perso!") ||
            strstr(buffer, "Pareggio") || strstr(buffer, "Partita terminata")) {
            clientState = FINE_PARTITA;
        }
        else if (strstr(buffer, "La partita") || strstr(buffer, "inizia")) {
            clientState = IN_PARTITA;
        }
    }
    return 0;
}