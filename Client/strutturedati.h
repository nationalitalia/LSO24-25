#ifndef STRUTTUREDATI_H
#define STRUTTUREDATI_H

#include "winsock_posix_compat.h"

// Porta e IP del server
#define PORT 9100
#define SERVER_IP "server"

// Stati del client
typedef enum { MENU, IN_PARTITA, FINE_PARTITA } ClientState;

// Variabili globali (visibili nei file .c)
extern sock_t sock;
extern ClientState clientState;

#endif
