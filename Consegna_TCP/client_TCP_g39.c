#include <winsock2.h>      // API di rete Winsock (socket, connect, send, recv, ecc.)
#include <ws2tcpip.h>      // Funzioni aggiuntive per indirizzi IP (struct sockaddr_in, ecc.)
#include <stdio.h>         // Libreria standard per input/output (printf, scanf, fgets, ecc.)
#include <stdlib.h>        // Libreria standard per funzioni generiche (exit, ecc.)
#include <string.h>        // Funzioni per la gestione di stringhe (strlen, strncmp, strcspn, ecc.)

#pragma comment(lib, "Ws2_32.lib")  // Per MSVC: link automatico con la libreria Ws2_32.lib (ignored da gcc/MinGW)

#define SERVER_PORT 5000   // Definisce la porta TCP del server a cui il client si collegherà
#define BUF_SIZE 256       // Dimensione massima dei buffer di lettura/scrittura delle stringhe

// Riceve una riga terminata da '\n' dal socket (come nel server)               // Commento di descrizione della funzione recv_line
int recv_line(SOCKET sock, char *buffer, int maxlen) {          // Definizione funzione: legge una linea da un socket TCP
    int count = 0;                                              // Inizializza il contatore dei caratteri letti a 0
    while (count < maxlen - 1) {                                // Continua finché c'è spazio nel buffer (lascia 1 per '\0')
        char c;                                                 // Variabile per il singolo carattere ricevuto
        int ret = recv(sock, &c, 1, 0);                         // Riceve un byte dal socket e lo memorizza in c
        if (ret == 0) {                                         // Se ret == 0, il server ha chiuso la connessione
            if (count == 0) return 0;                           // Se non è stato letto nulla, ritorna 0 (connessione chiusa subito)
            break;                                              // Se qualcosa è stato letto, esce dal ciclo
        }
        if (ret == SOCKET_ERROR) {                              // Se recv restituisce SOCKET_ERROR, c'è stato un errore
            return -1;                                          // Ritorna -1 per segnalare errore al chiamante
        }
        if (c == '\r') continue;                                // Se è '\r' (CR), lo ignora (gestione CRLF come LF)
        buffer[count++] = c;                                    // Salva il carattere c nel buffer e incrementa il contatore
        if (c == '\n') break;                                   // Se il carattere è '\n', la riga è terminata, esce dal ciclo
    }
    buffer[count] = '\0';                                       // Aggiunge il terminatore di stringa '\0' alla fine del buffer
    return count;                                               // Ritorna il numero di caratteri letti (escluso '\0')
}

// Invia TUTTI i byte di "data" (lunghezza length)                              // Commento di descrizione della funzione send_all
int send_all(SOCKET sock, const char *data, int length) {       // Definizione funzione: invia esattamente 'length' byte su un socket
    int total_sent = 0;                                         // Inizializza il contatore totale di byte inviati
    while (total_sent < length) {                               // Continua finché non sono stati inviati tutti i byte richiesti
        int sent = send(sock, data + total_sent, length - total_sent, 0); // Chiama send per inviare i byte rimanenti
        if (sent == SOCKET_ERROR) return SOCKET_ERROR;          // Se send fallisce, ritorna SOCKET_ERROR
        total_sent += sent;                                     // Aggiunge il numero di byte appena inviati al totale
    }
    return total_sent;                                          // Ritorna il numero totale di byte inviati (dovrebbe essere = length)
}

// Invia una stringa (già con '\n' se serve)                                    // Commento di descrizione della funzione send_line
int send_line(SOCKET sock, const char *line) {                 // Definizione funzione: invia una stringa di testo su un socket
    return send_all(sock, line, (int)strlen(line));            // Chiama send_all usando la lunghezza della stringa calcolata con strlen
}

int main(void) {                                               // Funzione principale del programma client TCP
    WSADATA wsa;                                               // Struttura che conterrà informazioni sulla versione di Winsock
    int result;                                                // Variabile per memorizzare codici di ritorno delle funzioni Winsock

    printf("[DEBUG] Avvio client TCP...\n");                   // Stampa di debug: indica che il client TCP è stato avviato

    // 1) Inizializza Winsock
    result = WSAStartup(MAKEWORD(2, 2), &wsa);                 // Inizializza la libreria Winsock richiedendo la versione 2.2
    if (result != 0) {                                         // Se il valore di ritorno non è 0, l'inizializzazione è fallita
        printf("WSAStartup failed: %d\n", result);             // Stampa il codice di errore ritornato da WSAStartup
        return 1;                                              // Termina il programma con codice di errore 1
    }

    // 2) Leggo hostname
    char hostname[256];                                        // Buffer per memorizzare l'hostname inserito dall'utente
    printf("Inserisci hostname del server (es. localhost): "); // Chiede all'utente di inserire il nome del server
    if (!fgets(hostname, sizeof(hostname), stdin)) {           // Legge una riga da stdin nel buffer hostname; se fallisce restituisce NULL
        printf("Errore lettura hostname\n");                   // Stampa messaggio di errore per la lettura dell'hostname
        WSACleanup();                                          // Libera le risorse allocate da Winsock
        return 1;                                              // Termina il programma con codice di errore 1
    }
    hostname[strcspn(hostname, "\n")] = '\0';                  // Rimuove il carattere '\n' finale dalla stringa hostname (se presente)

    printf("[DEBUG] Hostname inserito: '%s'\n", hostname);     // Stampa di debug: mostra l'hostname che l'utente ha inserito

    // 3) Risolvo hostname
    struct hostent *he = gethostbyname(hostname);              // Usa gethostbyname per risolvere l'hostname in indirizzo IP
    if (he == NULL) {                                          // Se gethostbyname restituisce NULL, la risoluzione è fallita
        printf("gethostbyname failed\n");                      // Stampa un messaggio di errore generico
        WSACleanup();                                          // Libera le risorse Winsock
        return 1;                                              // Termina il programma con codice di errore 1
    }

    // 4) Preparo indirizzo server
    struct sockaddr_in server_addr;                            // Struttura che conterrà indirizzo IP e porta del server
    memset(&server_addr, 0, sizeof(server_addr));              // Azzeramento della struttura server_addr per evitare dati sporchi
    server_addr.sin_family = AF_INET;                          // Imposta la famiglia di indirizzi a AF_INET (IPv4)
    server_addr.sin_port = htons(SERVER_PORT);                 // Imposta la porta del server convertendola in network byte order
    memcpy(&server_addr.sin_addr,                             // Copia l'indirizzo IP risolto nella struttura server_addr.sin_addr
           he->h_addr_list[0],                                 // Usa il primo indirizzo IP presente nella lista h_addr_list
           he->h_length);                                      // Copia he->h_length byte (dimensione dell'indirizzo IP)

    // 5) Creo socket
    SOCKET sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);   // Crea una socket TCP (AF_INET = IPv4, SOCK_STREAM = TCP)
    if (sock == INVALID_SOCKET) {                              // Controlla se la creazione della socket è fallita
        printf("socket failed: %d\n", WSAGetLastError());      // Stampa il codice di errore restituito da WSAGetLastError
        WSACleanup();                                          // Libera le risorse Winsock
        return 1;                                              // Termina il programma con codice di errore 1
    }

    // 6) Connect
    printf("[DEBUG] Connessione al server...\n");              // Stampa di debug: indica che sta tentando la connessione al server
    result = connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)); // Tenta di connettersi al server specificato
    if (result == SOCKET_ERROR) {                              // Se connect restituisce SOCKET_ERROR, la connessione è fallita
        printf("connect failed: %d\n", WSAGetLastError());     // Stampa il codice d'errore restituito da WSAGetLastError
        closesocket(sock);                                     // Chiude la socket creata
        WSACleanup();                                          // Libera le risorse Winsock
        return 1;                                              // Termina il programma con codice di errore 1
    }

    char buffer[BUF_SIZE];                                     // Buffer per memorizzare messaggi inviati/ricevuti dal server

    // 7) Ricevo "connessione avvenuta"
    printf("[DEBUG] Attendo messaggio iniziale dal server...\n"); // Stampa di debug: attende il messaggio iniziale dal server
    int n = recv_line(sock, buffer, BUF_SIZE);                 // Chiama recv_line per leggere una riga dal socket (messaggio iniziale)
    if (n <= 0) {                                              // Se n <= 0, c'è stato errore o chiusura connessione
        printf("Server ha chiuso la connessione prima del messaggio iniziale (n=%d).\n", n); // Stampa motivo con valore n
        closesocket(sock);                                     // Chiude la socket
        WSACleanup();                                          // Libera le risorse Winsock
        return 1;                                              // Termina il programma con codice di errore 1
    }
    printf("Messaggio dal server: %s", buffer);                // Stampa il messaggio iniziale ricevuto dal server

    // 8) Operazione
    printf("a) Addizione\n");
    printf("s) Sottrazione\n");
    printf("m) Moltiplicazione\n");
    printf("d) Divisione\n");
    printf("Inserisci operazione (a/s/m/d o un'altra lettera per terminare): "); // Chiede all'utente l'operazione desiderata
    if (!fgets(buffer, sizeof(buffer), stdin)) {               // Legge l'operazione inserita dall'utente in buffer
        printf("Errore input operazione.\n");                  // Stampa messaggio di errore se fgets fallisce
        closesocket(sock);                                     // Chiude la socket
        WSACleanup();                                          // Libera le risorse Winsock
        return 1;                                              // Termina il programma con codice di errore 1
    }

    char op = buffer[0];                                       // Prende il primo carattere del buffer come codice dell'operazione
    char op_line[3];                                           // Buffer per inviare l'operazione (carattere + '\n' + '\0')
    op_line[0] = op;                                           // Inserisce l'operazione nella prima posizione del buffer
    op_line[1] = '\n';                                         // Inserisce un newline come secondo carattere (per protocollo testuale)
    op_line[2] = '\0';                                         // Aggiunge il terminatore di stringa '\0'

    printf("[DEBUG] Invio operazione '%c' al server...\n", op); // Stampa di debug: mostra l'operazione che verrà inviata
    if (send_line(sock, op_line) == SOCKET_ERROR) {            // Chiama send_line per inviare l'operazione al server
        printf("Errore nell'invio dell'operazione. Codice: %d\n", WSAGetLastError()); // Stampa errore se send_line fallisce
        closesocket(sock);                                     // Chiude la socket
        WSACleanup();                                          // Libera le risorse Winsock
        return 1;                                              // Termina il programma con codice di errore 1
    }

    // 9) Ricevo risposta operazione
    printf("[DEBUG] Attendo risposta operazione dal server...\n"); // Stampa di debug: attende la risposta relativa all'operazione
    n = recv_line(sock, buffer, BUF_SIZE);                    // Legge dal socket la risposta del server (nome operazione o messaggio di termine)
    if (n <= 0) {                                              // Se n <= 0, c'è stato errore o chiusura connessione da parte del server
        printf("Server ha chiuso la connessione dopo l'operazione (n=%d).\n", n); // Stampa messaggio di errore/chiusura
        closesocket(sock);                                     // Chiude la socket
        WSACleanup();                                          // Libera le risorse Winsock
        return 1;                                              // Termina il programma con codice di errore 1
    }
    printf("[DEBUG] Stringa ricevuta per operazione: '%s'\n", buffer); // Stampa di debug: mostra la stringa ricevuta dal server

    if (strncmp(buffer, "TERMINE PROCESSO CLIENT", 23) == 0) { // Confronta i primi 23 caratteri per vedere se è il messaggio di termine
        printf("Risposta dal server: %s", buffer);             // Stampa il messaggio di termine ricevuto
        closesocket(sock);                                     // Chiude la socket
        WSACleanup();                                          // Libera le risorse Winsock
        printf("Premi INVIO per uscire...");                   // Chiede all'utente di premere INVIO prima di chiudere
        getchar();                                             // Attende l'input dell'utente
        return 0;                                              // Termina il programma con codice di successo 0
    }

    printf("Operazione scelta dal server: %s", buffer);        // Stampa la descrizione dell'operazione (ADDIZIONE, ecc.) ricevuta dal server

    // 10) Chiedo i due interi
    long a, b;                                                 // Dichiarazione delle variabili per i due interi da inviare al server
    printf("Inserisci primo intero: ");                        // Chiede all'utente il primo intero
    if (scanf("%ld", &a) != 1) {                               // Legge il primo intero da stdin e verifica che la lettura sia valida
        printf("Input non valido per il primo intero.\n");     // Stampa messaggio di errore se l'input non è un intero
        closesocket(sock);                                     // Chiude la socket
        WSACleanup();                                          // Libera le risorse Winsock
        return 1;                                              // Termina il programma con codice di errore 1
    }

    printf("Inserisci secondo intero: ");                      // Chiede all'utente il secondo intero
    if (scanf("%ld", &b) != 1) {                               // Legge il secondo intero da stdin e verifica la validità dell'input
        printf("Input non valido per il secondo intero.\n");   // Stampa messaggio di errore se l'input non è valido
        closesocket(sock);                                     // Chiude la socket
        WSACleanup();                                          // Libera le risorse Winsock
        return 1;                                              // Termina il programma con codice di errore 1
    }

    int c;                                                     // Variabile temporanea usata per pulire il buffer di input stdin
    while ((c = getchar()) != '\n' && c != EOF) {              // Consuma i caratteri rimanenti nel buffer (es. '\n' lasciato da scanf)
        // svuota stdin                                         // Commento: questo ciclo serve solo a svuotare stdin
    }

    char num_buf[BUF_SIZE];                                    // Buffer per convertire e inviare i numeri interi come stringhe

    // 11) Invio primo intero
    snprintf(num_buf, BUF_SIZE, "%ld\n", a);                   // Converte il primo intero 'a' in stringa e aggiunge '\n' finale
    printf("[DEBUG] Invio primo intero: '%s'\n", num_buf);     // Stampa di debug: mostra la stringa del primo intero
    if (send_line(sock, num_buf) == SOCKET_ERROR) {            // Invia la stringa del primo intero al server
        printf("Errore invio primo intero. Codice: %d\n", WSAGetLastError()); // Stampa messaggio di errore se send_line fallisce
        closesocket(sock);                                     // Chiude la socket
        WSACleanup();                                          // Libera le risorse Winsock
        return 1;                                              // Termina il programma con codice di errore 1
    }

    // 12) Invio secondo intero
    snprintf(num_buf, BUF_SIZE, "%ld\n", b);                   // Converte il secondo intero 'b' in stringa con '\n' finale
    printf("[DEBUG] Invio secondo intero: '%s'\n", num_buf);   // Stampa di debug: mostra la stringa del secondo intero
    if (send_line(sock, num_buf) == SOCKET_ERROR) {            // Invia la stringa del secondo intero al server
        printf("Errore invio secondo intero. Codice: %d\n", WSAGetLastError()); // Stampa il codice di errore se l'invio fallisce
        closesocket(sock);                                     // Chiude la socket
        WSACleanup();                                          // Libera le risorse Winsock
        return 1;                                              // Termina il programma con codice di errore 1
    }

    // 13) Ricevo risultato
    printf("[DEBUG] Attendo risultato dal server...\n");       // Stampa di debug: indica che il client attende il risultato dal server
    n = recv_line(sock, buffer, BUF_SIZE);                     // Chiama recv_line per leggere il risultato dal socket
    if (n < 0) {                                               // Se n < 0, recv_line ha indicato un errore
        printf("Errore in recv_line risultato. Codice: %d\n", WSAGetLastError()); // Stampa il codice d'errore
        closesocket(sock);                                     // Chiude la socket
        WSACleanup();                                          // Libera le risorse Winsock
        return 1;                                              // Termina il programma con codice di errore 1
    }
    if (n == 0) {                                              // Se n == 0, il server ha chiuso la connessione senza inviare il risultato
        printf("Server ha chiuso la connessione senza inviare il risultato (n=0).\n"); // Stampa spiegazione dell'evento
        closesocket(sock);                                     // Chiude la socket
        WSACleanup();                                          // Libera le risorse Winsock
        return 1;                                              // Termina il programma con codice di errore 1
    }

    printf("Risultato dal server: %s", buffer);                // Stampa il risultato (numero o messaggio d'errore) ricevuto dal server

    // 14) Chiusura
    closesocket(sock);                                         // Chiude la socket del client
    WSACleanup();                                              // Libera tutte le risorse allocate dalla libreria Winsock

    printf("Premi INVIO per uscire...");                       // Chiede all'utente di premere INVIO prima di terminare il programma
    getchar();                                                 // Attende che l'utente prema INVIO (utile se avviato con doppio click)
    return 0;                                                  // Termina il programma con codice di successo 0
}
