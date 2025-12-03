#include <winsock2.h>      // Libreria per API di rete Winsock (socket, bind, listen, accept, send, recv, ecc.)
#include <ws2tcpip.h>      // Libreria per funzioni aggiuntive IP (inet_ntop, ecc.)
#include <stdio.h>         // Libreria standard per input/output (printf, ecc.)
#include <stdlib.h>        // Libreria standard per funzioni generiche (exit, ecc.)
#include <string.h>        // Libreria per gestione stringhe (strlen, memset, ecc.)

#pragma comment(lib, "Ws2_32.lib")  // Per MSVC: link automatico a Ws2_32.lib (ignorato da gcc/MinGW)

#define SERVER_PORT 5000   // Porta TCP su cui il server resta in ascolto
#define BUF_SIZE 256       // Dimensione massima del buffer di comunicazione

// Funzione di utilità: riceve una riga terminata da '\n' dal socket           // Commento di descrizione
int recv_line(SOCKET sock, char *buffer, int maxlen) {         // Definizione funzione recv_line
    int count = 0;                                             // Contatore caratteri letti
    while (count < maxlen - 1) {                               // Continua finché c'è spazio nel buffer
        char c;                                                // Variabile per un singolo carattere
        int ret = recv(sock, &c, 1, 0);                        // Riceve 1 byte dal socket
        if (ret == 0) {                                        // Se ret == 0, il client ha chiuso la connessione
            if (count == 0) return 0;                          // Se non è stato letto nulla, ritorna 0
            break;                                             // Se qualcosa è stato letto, esce dal ciclo
        }
        if (ret == SOCKET_ERROR) {                             // Se recv ha restituito errore
            return -1;                                         // Ritorna -1 per segnalare l'errore
        }
        if (c == '\r') continue;                               // Ignora eventuali '\r' (CR) in sequenze CRLF
        buffer[count++] = c;                                   // Salva il carattere nel buffer e incrementa count
        if (c == '\n') break;                                  // Se è '\n', la riga è terminata → esce
    }
    buffer[count] = '\0';                                      // Aggiunge terminatore di stringa '\0'
    return count;                                              // Ritorna il numero di caratteri letti
}

// Funzione di utilità: invia tutti i byte di un buffer                            // Commento di descrizione
int send_all(SOCKET sock, const char *data, int length) {      // Definizione funzione send_all
    int total_sent = 0;                                        // Contatore dei byte già inviati
    while (total_sent < length) {                              // Finché non abbiamo inviato tutto
        int sent = send(sock, data + total_sent, length - total_sent, 0); // Invia la parte rimanente del buffer
        if (sent == SOCKET_ERROR) return SOCKET_ERROR;         // In caso di errore, ritorna SOCKET_ERROR
        total_sent += sent;                                    // Aggiorna il totale inviato
    }
    return total_sent;                                         // Ritorna il numero totale di byte inviati
}

// Funzione di utilità: invia una stringa (già terminata da '\n' se serve)        // Commento di descrizione
int send_line(SOCKET sock, const char *line) {                 // Definizione funzione send_line
    return send_all(sock, line, (int)strlen(line));            // Chiama send_all con la lunghezza della stringa
}

int main(void) {                                               // Funzione principale del server
    WSADATA wsa;                                               // Struttura per informazioni su Winsock
    int result;                                                // Variabile per esiti delle funzioni

    printf("[SERVER] Avvio server TCP...\n");                  // Messaggio: server in avvio

    result = WSAStartup(MAKEWORD(2, 2), &wsa);                 // Inizializza Winsock, richiede versione 2.2
    if (result != 0) {                                         // Controlla se WSAStartup è fallita
        printf("WSAStartup failed: %d\n", result);             // Stampa il codice di errore
        return 1;                                              // Termina con errore
    }

    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP); // Crea la socket di ascolto TCP
    if (listen_sock == INVALID_SOCKET) {                       // Controlla la creazione della socket
        printf("socket failed: %d\n", WSAGetLastError());      // Stampa il codice di errore
        WSACleanup();                                          // Libera Winsock
        return 1;                                              // Termina con errore
    }

    struct sockaddr_in server_addr;                            // Struttura indirizzo del server
    memset(&server_addr, 0, sizeof(server_addr));              // Azzera la struttura

    server_addr.sin_family = AF_INET;                          // Imposta IPv4
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);           // Accetta connessioni su tutte le interfacce
    server_addr.sin_port = htons(SERVER_PORT);                 // Imposta la porta del server

    result = bind(listen_sock,                                 // Associa la socket all'indirizzo indicato
                  (struct sockaddr *)&server_addr,             // Puntatore alla struttura indirizzo
                  sizeof(server_addr));                        // Dimensione della struttura
    if (result == SOCKET_ERROR) {                              // Se bind fallisce
        printf("bind failed: %d\n", WSAGetLastError());        // Stampa codice errore
        closesocket(listen_sock);                              // Chiude la socket
        WSACleanup();                                          // Libera Winsock
        return 1;                                              // Termina con errore
    }

    result = listen(listen_sock, SOMAXCONN);                   // Mette la socket in ascolto per connessioni in ingresso
    if (result == SOCKET_ERROR) {                              // Se listen fallisce
        printf("listen failed: %d\n", WSAGetLastError());      // Stampa errore
        closesocket(listen_sock);                              // Chiude la socket
        WSACleanup();                                          // Libera Winsock
        return 1;                                              // Termina
    }

    printf("[SERVER] In ascolto sulla porta %d...\n", SERVER_PORT); // Messaggio: server pronto ad accettare connessioni

    while (1) {                                                // Ciclo infinito per accettare più client
        struct sockaddr_in client_addr;                        // Struttura indirizzo del client
        int client_len = sizeof(client_addr);                  // Lunghezza della struttura indirizzo

        printf("[SERVER] In attesa di una nuova connessione...\n"); // Messaggio: in attesa di client
        SOCKET client_sock = accept(listen_sock,               // Accetta una connessione in ingresso
                                    (struct sockaddr *)&client_addr, // Puntatore alla struttura che verrà riempita con dati client
                                    &client_len);              // Puntatore a lunghezza struttura
        if (client_sock == INVALID_SOCKET) {                   // Se accept fallisce
            printf("accept failed: %d\n", WSAGetLastError());  // Stampa errore
            continue;                                          // Torna ad ascoltare un altro client
        }

        char client_ip[INET_ADDRSTRLEN];                       // Buffer per memorizzare IP del client in formato testuale
        inet_ntop(AF_INET, &client_addr.sin_addr,              // Converte l'indirizzo IP binario in stringa
                  client_ip, sizeof(client_ip));               // Salva nell'array client_ip
        int client_port = ntohs(client_addr.sin_port);         // Converte la porta del client in host byte order

        printf("[SERVER] Connessione accettata da %s:%d\n", client_ip, client_port); // Stampa IP e porta del client

        char buffer[BUF_SIZE];                                 // Buffer per comunicazione col client

        const char *welcome = "connessione avvenuta\n";        // Messaggio iniziale di benvenuto
        if (send_line(client_sock, welcome) == SOCKET_ERROR) { // Invia messaggio di benvenuto al client
            printf("[SERVER] Errore nell'invio del messaggio iniziale.\n"); // Stampa errore
            closesocket(client_sock);                          // Chiude la socket del client
            continue;                                          // Torna ad accettare nuovi client
        }

        printf("[SERVER] Messaggio di benvenuto inviato al client.\n"); // Conferma invio messaggio di benvenuto

        int n = recv_line(client_sock, buffer, BUF_SIZE);      // Riceve la lettera dell'operazione dal client
        if (n <= 0) {                                          // Se errore o chiusura connessione
            printf("[SERVER] Client %s:%d ha chiuso prima di inviare l'operazione.\n", client_ip, client_port); // Messaggio diagnostico
            closesocket(client_sock);                          // Chiude la socket
            continue;                                          // Torna al prossimo client
        }

        char op = buffer[0];                                   // Prende il primo carattere come operazione
        if (op >= 'a' && op <= 'z') {                          // Se operazione in minuscolo
            op = (char)(op - 'a' + 'A');                       // Converti in maiuscolo
        }

        printf("[SERVER] Operazione richiesta dal client %s:%d: '%c'\n", client_ip, client_port, op); // Stampa operazione richiesta

        if (op != 'A' && op != 'S' && op != 'M' && op != 'D') { // Controlla se è una operazione non valida
            const char *end_msg = "TERMINE PROCESSO CLIENT\n";  // Messaggio di termine per operazione non valida
            send_line(client_sock, end_msg);                   // Invia il messaggio di termine al client
            printf("[SERVER] Operazione non valida, invio TERMINE PROCESSO CLIENT e chiudo.\n"); // Log server
            closesocket(client_sock);                          // Chiude la connessione col client
            continue;                                          // Torna al ciclo per accettare nuovi client
        }

        const char *op_msg = NULL;                             // Puntatore al messaggio descrittivo dell'operazione
        switch (op) {                                          // Selezione in base al tipo di operazione
            case 'A':                                          // Caso A: addizione
                op_msg = "ADDIZIONE\n";                        // Stringa che descrive l'operazione
                break;                                         // Esce dal case
            case 'S':                                          // Caso S: sottrazione
                op_msg = "SOTTRAZIONE\n";                      // Stringa descrittiva
                break;                                         // Esce
            case 'M':                                          // Caso M: moltiplicazione
                op_msg = "MOLTIPLICAZIONE\n";                  // Stringa descrittiva
                break;                                         // Esce
            case 'D':                                          // Caso D: divisione
                op_msg = "DIVISIONE\n";                        // Stringa descrittiva
                break;                                         // Esce
        }

        if (send_line(client_sock, op_msg) == SOCKET_ERROR) {  // Invia al client il nome dell'operazione scelta
            printf("[SERVER] Errore nell'invio del nome operazione al client.\n"); // Messaggio errore
            closesocket(client_sock);                          // Chiude socket
            continue;                                          // Torna prossimo client
        }

        printf("[SERVER] Descrizione operazione inviata al client.\n"); // Log invio descrizione operazione

        long a, b;                                             // Variabili per i due operandi
        n = recv_line(client_sock, buffer, BUF_SIZE);          // Riceve il primo intero come stringa
        if (n <= 0) {                                          // Controllo errore o chiusura
            printf("[SERVER] Client %s:%d ha chiuso prima di inviare il primo numero.\n", client_ip, client_port); // Log
            closesocket(client_sock);                          // Chiude socket
            continue;                                          // Prossimo client
        }
        a = strtol(buffer, NULL, 10);                          // Converte la stringa in long

        printf("[SERVER] Primo operando ricevuto dal client %s:%d: %ld\n", client_ip, client_port, a); // Stampa primo operando

        n = recv_line(client_sock, buffer, BUF_SIZE);          // Riceve il secondo intero come stringa
        if (n <= 0) {                                          // Controllo errore o chiusura
            printf("[SERVER] Client %s:%d ha chiuso prima di inviare il secondo numero.\n", client_ip, client_port); // Log
            closesocket(client_sock);                          // Chiude socket
            continue;                                          // Prossimo client
        }
        b = strtol(buffer, NULL, 10);                          // Converte la stringa in long

        printf("[SERVER] Secondo operando ricevuto dal client %s:%d: %ld\n", client_ip, client_port, b); // Stampa secondo operando

        int error = 0;                                         // Flag per eventuali errori (es. divisione per zero)
        long result_value = 0;                                 // Variabile per il risultato

        switch (op) {                                          // Esegue operazione in base alla lettera
            case 'A':                                          // Addizione
                result_value = a + b;                          // Calcola a + b
                break;                                         // Esce dal case
            case 'S':                                          // Sottrazione
                result_value = a - b;                          // Calcola a - b
                break;                                         // Esce
            case 'M':                                          // Moltiplicazione
                result_value = a * b;                          // Calcola a * b
                break;                                         // Esce
            case 'D':                                          // Divisione
                if (b == 0) {                                  // Controlla divisione per zero
                    error = 1;                                 // Imposta flag errore
                } else {                                       // Altrimenti
                    result_value = a / b;                      // Divisione intera a / b
                }
                break;                                         // Esce
        }

        if (error) {                                           // Se c'è stato un errore
            printf("[SERVER] Errore nell'operazione (es. divisione per zero) per client %s:%d\n", client_ip, client_port); // Log errore
        } else {                                               // Se non ci sono errori
            printf("[SERVER] Risultato operazione per client %s:%d: %ld (come float: %.2f)\n",
                   client_ip,                                  // IP del client
                   client_port,                                // Porta del client
                   result_value,                               // Risultato intero
                   (double)result_value);                      // Risultato convertito in double (float) per debug
        }

        char resp[BUF_SIZE];                                   // Buffer per la risposta al client
        if (error) {                                           // Se c'è errore (divisione per zero)
            snprintf(resp, BUF_SIZE, "Errore: divisione per zero\n"); // Scrive messaggio di errore nel buffer
        } else {                                               // Altrimenti
            snprintf(resp, BUF_SIZE, "%ld\n", result_value);   // Scrive il risultato come stringa intera
        }

        if (send_line(client_sock, resp) == SOCKET_ERROR) {    // Invia il risultato (o errore) al client
            printf("[SERVER] Errore nell'invio del risultato al client %s:%d\n", client_ip, client_port); // Log errore
        } else {                                               // Se l'invio è andato a buon fine
            printf("[SERVER] Risultato inviato al client %s:%d: %s", client_ip, client_port, resp); // Log risultato inviato
        }

        closesocket(client_sock);                              // Chiude la connessione con il client
        printf("[SERVER] Connessione con %s:%d chiusa.\n", client_ip, client_port); // Log chiusura connessione
    }

    closesocket(listen_sock);                                  // (In pratica mai raggiunto) Chiude la socket di ascolto
    WSACleanup();                                              // Libera tutte le risorse della libreria Winsock
    return 0;                                                  // Termina il programma con successo
}
