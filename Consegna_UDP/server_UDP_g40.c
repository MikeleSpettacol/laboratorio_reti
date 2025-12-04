#include <winsock2.h>      // API di rete per Windows (socket, bind, recvfrom, sendto, ecc.)
#include <ws2tcpip.h>      // Funzioni aggiuntive per IP (inet_ntop, ecc.)
#include <stdio.h>         // Libreria standard per I/O (printf, ecc.)
#include <stdlib.h>        // Libreria standard per funzioni generiche
#include <string.h>        // Funzioni per gestione stringhe (memset, strlen, ecc.)

#pragma comment(lib, "Ws2_32.lib")  // Per MSVC: link automatico a Ws2_32.lib

#define SERVER_PORT 5000   // Porta UDP su cui il server resta in ascolto
#define BUF_SIZE 256       // Dimensione massima buffer

int main(void) {                                               // Funzione principale del server UDP
    WSADATA wsa;                                               // Struttura info Winsock
    int result;                                                // Variabile per codici di ritorno

    printf("[UDP SERVER] Avvio server UDP...\n");              // Messaggio: avvio server UDP

    result = WSAStartup(MAKEWORD(2, 2), &wsa);                 // Inizializza Winsock versione 2.2
    if (result != 0) {                                         // Controlla se inizializzazione fallita
        printf("WSAStartup failed: %d\n", result);             // Stampa codice errore
        return 1;                                              // Termina con errore
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);    // Crea socket UDP
    if (sock == INVALID_SOCKET) {                              // Controlla creazione socket
        printf("socket failed: %d\n", WSAGetLastError());      // Stampa errore
        WSACleanup();                                          // Libera Winsock
        return 1;                                              // Termina
    }

    struct sockaddr_in server_addr;                            // Struttura indirizzo server
    memset(&server_addr, 0, sizeof(server_addr));              // Azzera la struttura

    server_addr.sin_family = AF_INET;                          // IPv4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);           // Riceve datagram su tutte le interfacce locali
    server_addr.sin_port = htons(SERVER_PORT);                 // Porta del server

    result = bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)); // Associa socket a indirizzo/porta
    if (result == SOCKET_ERROR) {                              // Se bind fallisce
        printf("bind failed: %d\n", WSAGetLastError());        // Stampa errore
        closesocket(sock);                                     // Chiude socket
        WSACleanup();                                          // Libera Winsock
        return 1;                                              // Termina
    }

    printf("[UDP SERVER] In ascolto sulla porta %d...\n", SERVER_PORT); // Messaggio: server pronto

    char buffer[BUF_SIZE];                                     // Buffer per dati ricevuti
    struct sockaddr_in client_addr;                            // Struttura per indirizzo client
    int client_len;                                            // Lunghezza struttura indirizzo client
    int nbytes;                                                // Numero di byte ricevuti
    long a, b;                                                 // Operandi
    long result_value;                                         // Risultato operazione
    int error;                                                 // Flag errore (es. divisione per zero)

    while (1) {                                                // Ciclo infinito (server sempre in ascolto)
        client_len = sizeof(client_addr);                      // Imposta dimensione della struttura client_addr

        printf("[UDP SERVER] In attesa di HELLO da un client...\n"); // Log: in attesa di HELLO
        nbytes = recvfrom(sock,                                // Riceve datagram dal client
                           buffer,                             // Buffer di destinazione
                           BUF_SIZE - 1,                       // Numero max byte da ricevere
                           0,                                  // Flag standard
                           (struct sockaddr *)&client_addr,    // Indirizzo del client (riempito da recvfrom)
                           &client_len);                       // Lunghezza indirizzo client (aggiornata)
        if (nbytes == SOCKET_ERROR) {                          // Controlla errore ricezione
            printf("recvfrom (HELLO) failed: %d\n", WSAGetLastError()); // Stampa errore
            continue;                                          // Torna ad ascoltare
        }

        buffer[nbytes] = '\0';                                 // Termina la stringa ricevuta
        char client_ip[INET_ADDRSTRLEN];                       // Buffer per IP client in formato testuale
        inet_ntop(AF_INET, &client_addr.sin_addr,              // Converte IP binario in stringa
                  client_ip, sizeof(client_ip));               // Salva nell'array client_ip
        int client_port = ntohs(client_addr.sin_port);         // Converte porta client in host byte order

        printf("[UDP SERVER] Ricevuto HELLO da %s:%d: '%s'\n", client_ip, client_port, buffer); // Log: HELLO + contenuto

        const char *welcome = "connessione avvenuta\n";        // Messaggio di benvenuto per il client
        nbytes = sendto(sock,                                  // Invia datagram
                        welcome,                               // Dati da inviare
                        (int)strlen(welcome),                  // Numero di byte
                        0,                                     // Flag
                        (struct sockaddr *)&client_addr,       // Indirizzo client
                        client_len);                           // Dimensione indirizzo client
        if (nbytes == SOCKET_ERROR) {                          // Controlla errore sendto
            printf("sendto (welcome) failed: %d\n", WSAGetLastError()); // Stampa errore
            continue;                                          // Torna ad ascoltare
        }

        printf("[UDP SERVER] Messaggio di benvenuto inviato a %s:%d\n", client_ip, client_port); // Log

        client_len = sizeof(client_addr);                      // Reimposta dimensione indirizzo prima di nuova recv

        printf("[UDP SERVER] In attesa dell'operazione dal client %s:%d...\n", client_ip, client_port); // Log
        nbytes = recvfrom(sock,                                // Riceve operazione
                           buffer,                             // Buffer destinazione
                           BUF_SIZE - 1,                       // Max byte
                           0,                                  // Flag
                           (struct sockaddr *)&client_addr,    // Indirizzo client
                           &client_len);                       // Dimensione indirizzo client
        if (nbytes == SOCKET_ERROR) {                          // Se errore
            printf("recvfrom (operazione) failed: %d\n", WSAGetLastError()); // Stampa errore
            continue;                                          // Torna al while
        }

        buffer[nbytes] = '\0';                                 // Termina la stringa
        char op = buffer[0];                                   // Primo carattere come operazione
        if (op >= 'a' && op <= 'z') {                          // Se minuscolo
            op = (char)(op - 'a' + 'A');                       // Converti in maiuscolo
        }

        printf("[UDP SERVER] Operazione richiesta da %s:%d: '%c'\n", client_ip, client_port, op); // Log operazione

        if (op != 'A' && op != 'S' && op != 'M' && op != 'D') { // Se operazione non valida
            const char *end_msg = "TERMINE PROCESSO CLIENT\n";  // Messaggio di termine
            sendto(sock,                                       // Invia messaggio di termine
                   end_msg,
                   (int)strlen(end_msg),
                   0,
                   (struct sockaddr *)&client_addr,
                   client_len);
            printf("[UDP SERVER] Operazione non valida da %s:%d, inviato TERMINE PROCESSO CLIENT.\n", client_ip, client_port); // Log
            continue;                                          // Torna in attesa di un nuovo HELLO (anche dallo stesso client)
        }

        const char *op_msg = NULL;                             // Puntatore a descrizione operazione
        switch (op) {                                          // Seleziona descrizione
            case 'A': op_msg = "ADDIZIONE\n";      break;      // A: addizione
            case 'S': op_msg = "SOTTRAZIONE\n";    break;      // S: sottrazione
            case 'M': op_msg = "MOLTIPLICAZIONE\n"; break;     // M: moltiplicazione
            case 'D': op_msg = "DIVISIONE\n";      break;      // D: divisione
        }

        nbytes = sendto(sock,                                  // Invia descrizione operazione
                        op_msg,
                        (int)strlen(op_msg),
                        0,
                        (struct sockaddr *)&client_addr,
                        client_len);
        if (nbytes == SOCKET_ERROR) {                          // Controlla errore
            printf("sendto (op_msg) failed: %d\n", WSAGetLastError()); // Stampa errore
            continue;                                          // Torna ciclo
        }

        printf("[UDP SERVER] Descrizione operazione inviata a %s:%d: %s", client_ip, client_port, op_msg); // Log

        client_len = sizeof(client_addr);                      // Reimposta dimensione indirizzo

        printf("[UDP SERVER] In attesa del primo numero da %s:%d...\n", client_ip, client_port); // Log
        nbytes = recvfrom(sock,                                // Riceve primo intero
                           buffer,
                           BUF_SIZE - 1,
                           0,
                           (struct sockaddr *)&client_addr,
                           &client_len);
        if (nbytes == SOCKET_ERROR) {                          // Se errore
            printf("recvfrom (primo intero) failed: %d\n", WSAGetLastError()); // Stampa errore
            continue;                                          // Torna ciclo
        }
        buffer[nbytes] = '\0';                                 // Termina stringa
        a = strtol(buffer, NULL, 10);                          // Converte stringa in long

        printf("[UDP SERVER] Primo operando da %s:%d: %ld\n", client_ip, client_port, a); // Stampa primo operando

        client_len = sizeof(client_addr);                      // Reimposta dimensione indirizzo

        printf("[UDP SERVER] In attesa del secondo numero da %s:%d...\n", client_ip, client_port); // Log
        nbytes = recvfrom(sock,                                // Riceve secondo intero
                           buffer,
                           BUF_SIZE - 1,
                           0,
                           (struct sockaddr *)&client_addr,
                           &client_len);
        if (nbytes == SOCKET_ERROR) {                          // Se errore
            printf("recvfrom (secondo intero) failed: %d\n", WSAGetLastError()); // Stampa errore
            continue;                                          // Torna ciclo
        }
        buffer[nbytes] = '\0';                                 // Termina stringa
        b = strtol(buffer, NULL, 10);                          // Converte stringa in long

        printf("[UDP SERVER] Secondo operando da %s:%d: %ld\n", client_ip, client_port, b); // Stampa secondo operando

        error = 0;                                             // Reset flag errore
        result_value = 0;                                      // Reset risultato

        switch (op) {                                          // Esegue operazione effettiva
            case 'A': result_value = a + b; break;             // Addizione
            case 'S': result_value = a - b; break;             // Sottrazione
            case 'M': result_value = a * b; break;             // Moltiplicazione
            case 'D':
                if (b == 0) {                                  // Controllo divisione per zero
                    error = 1;                                 // Segna errore
                } else {
                    result_value = a / b;                      // Divisione intera
                }
                break;                                         // Fine case D
        }

        if (error) {                                           // Se c'è errore
            printf("[UDP SERVER] Errore nell'operazione (es. divisione per zero) per client %s:%d\n", client_ip, client_port); // Log
        } else {                                               // Se non c'è errore
            printf("[UDP SERVER] Risultato operazione per client %s:%d: %ld (come float: %.2f)\n",
                   client_ip,                                  // IP client
                   client_port,                                // Porta client
                   result_value,                               // Risultato intero
                   (double)result_value);                      // Risultato come double per debug
        }

        char resp[BUF_SIZE];                                   // Buffer per risposta da inviare
        if (error) {                                           // Se errore (divisione per zero)
            snprintf(resp, BUF_SIZE, "Errore: divisione per zero\n"); // Messaggio di errore
        } else {                                               // Se tutto ok
            snprintf(resp, BUF_SIZE, "%ld\n", result_value);   // Inserisce risultato come stringa intera
        }

        nbytes = sendto(sock,                                  // Invia risultato al client
                        resp,
                        (int)strlen(resp),
                        0,
                        (struct sockaddr *)&client_addr,
                        client_len);
        if (nbytes == SOCKET_ERROR) {                          // Se invio fallisce
            printf("sendto (risultato) failed: %d\n", WSAGetLastError()); // Stampa errore
        } else {                                               // Se invio ok
            printf("[UDP SERVER] Risultato inviato a %s:%d: %s", client_ip, client_port, resp); // Log risultato inviato
        }

        // Dopo questa operazione, il server torna all'inizio del while         // Commento: pronto per un nuovo HELLO
    }

    closesocket(sock);                                         // (In pratica mai raggiunto) chiude socket
    WSACleanup();                                              // Libera Winsock
    return 0;                                                  // Termina con successo
}
