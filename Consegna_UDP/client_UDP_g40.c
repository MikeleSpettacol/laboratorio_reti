#include <winsock2.h>      // Include le API di rete per Windows (socket, sendto, recvfrom, ecc.)
#include <ws2tcpip.h>      // Include funzioni aggiuntive per indirizzi IP e conversioni
#include <stdio.h>         // Include funzioni standard di I/O (printf, scanf, fgets, ecc.)
#include <stdlib.h>        // Include funzioni generiche (exit, ecc.)
#include <string.h>        // Include funzioni per la gestione delle stringhe (strlen, strcspn, strncmp, ecc.)

#pragma comment(lib, "Ws2_32.lib")  // Per MSVC: chiede al linker di collegare automaticamente la libreria Ws2_32.lib

#define SERVER_PORT 5000   // Definisce la porta UDP del server; deve coincidere con quella del server
#define BUF_SIZE 256       // Definisce la dimensione massima del buffer per i messaggi

int main(void) {                                               // Funzione principale del programma client UDP
    WSADATA wsa;                                               // Struttura per memorizzare le informazioni su Winsock
    int result;                                                // Variabile per memorizzare i valori di ritorno delle funzioni Winsock

    printf("[DEBUG] Avvio client UDP...\n");                   // Messaggio di debug: indica l'avvio del client UDP

    result = WSAStartup(MAKEWORD(2, 2), &wsa);                 // Inizializza la libreria Winsock richiedendo la versione 2.2
    if (result != 0) {                                         // Controlla se WSAStartup è fallita (valore di ritorno diverso da 0)
        printf("WSAStartup failed: %d\n", result);             // Stampa il codice di errore restituito da WSAStartup
        printf("Premi INVIO per uscire...");                   // Chiede all'utente di premere INVIO prima di uscire
        getchar();                                             // Attende che l'utente prema INVIO
        return 1;                                              // Termina il programma con codice di errore 1
    }

    char hostname[256];                                        // Buffer per memorizzare l'hostname del server inserito dall'utente
    printf("Inserisci hostname del server (es. localhost): "); // Chiede all'utente di inserire il nome del server
    if (!fgets(hostname, sizeof(hostname), stdin)) {           // Legge una riga da stdin e la salva in hostname; se fallisce, restituisce NULL
        printf("Errore lettura hostname\n");                   // Stampa un messaggio di errore per la lettura dell'hostname
        WSACleanup();                                          // Libera le risorse allocate da Winsock
        printf("Premi INVIO per uscire...");                   // Chiede all'utente di premere INVIO
        getchar();                                             // Attende l'input dell'utente
        return 1;                                              // Termina il programma con codice di errore 1
    }

    hostname[strcspn(hostname, "\n")] = '\0';                  // Rimuove il carattere '\n' finale dalla stringa hostname (se presente)

    printf("[DEBUG] Hostname inserito: '%s'\n", hostname);     // Stampa di debug: mostra l'hostname che l'utente ha inserito

    struct hostent *he = gethostbyname(hostname);              // Usa gethostbyname per risolvere l'hostname in indirizzo IP
    if (he == NULL) {                                          // Se il risultato è NULL, la risoluzione è fallita
        printf("gethostbyname failed\n");                      // Stampa un messaggio di errore generico
        WSACleanup();                                          // Libera le risorse di Winsock
        printf("Premi INVIO per uscire...");                   // Chiede all'utente di premere INVIO
        getchar();                                             // Attende input
        return 1;                                              // Termina il programma con codice di errore 1
    }

    SOCKET sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);    // Crea una socket UDP (AF_INET = IPv4, SOCK_DGRAM = UDP, IPPROTO_UDP = protocollo UDP)
    if (sock == INVALID_SOCKET) {                              // Controlla se la creazione della socket è fallita
        printf("socket failed: %d\n", WSAGetLastError());      // Stampa il codice di errore specifico restituito da WSAGetLastError
        WSACleanup();                                          // Libera le risorse Winsock
        printf("Premi INVIO per uscire...");                   // Chiede all'utente di premere INVIO
        getchar();                                             // Attende input
        return 1;                                              // Termina il programma con codice di errore 1
    }

    struct sockaddr_in server_addr;                            // Struttura che conterrà l'indirizzo IP e la porta del server
    memset(&server_addr, 0, sizeof(server_addr));              // Azzeramento della struttura server_addr per evitare dati sporchi

    server_addr.sin_family = AF_INET;                          // Imposta il campo sin_family della struttura a AF_INET (IPv4)
    server_addr.sin_port = htons(SERVER_PORT);                 // Imposta il campo sin_port con la porta del server convertita in network byte order
    memcpy(&server_addr.sin_addr,                             // Copia l'indirizzo IP risolto nella struttura server_addr.sin_addr
           he->h_addr_list[0],                                 // Usa il primo indirizzo IP nella lista h_addr_list della struttura hostent
           he->h_length);                                      // Copia un numero di byte pari a he->h_length

    int server_len = sizeof(server_addr);                      // Memorizza la dimensione della struttura server_addr (serve per sendto/recvfrom)
    char buffer[BUF_SIZE];                                     // Buffer generico per inviare e ricevere messaggi dal server
    int nbytes;                                                // Variabile per memorizzare il numero di byte inviati o ricevuti

    const char *hello = "HELLO\n";                             // Stringa di "handshake" iniziale da inviare al server

    printf("[DEBUG] Invio HELLO al server...\n");              // Messaggio di debug: indica che il client sta inviando HELLO
    nbytes = sendto(sock,                                      // Funzione per inviare un datagram tramite UDP
                    hello,                                     // Puntatore ai dati da inviare (la stringa "HELLO\n")
                    (int)strlen(hello),                        // Numero di byte da inviare (lunghezza della stringa)
                    0,                                         // Flag (0 = nessuna opzione speciale)
                    (struct sockaddr *)&server_addr,           // Puntatore alla struttura dell'indirizzo del server
                    server_len);                               // Dimensione della struttura server_addr
    if (nbytes == SOCKET_ERROR) {                              // Controlla se sendto ha restituito un errore
        printf("sendto (HELLO) failed: %d\n", WSAGetLastError()); // Stampa il codice di errore restituito da WSAGetLastError
        closesocket(sock);                                     // Chiude la socket del client
        WSACleanup();                                          // Libera le risorse Winsock
        printf("Premi INVIO per uscire...");                   // Chiede all'utente di premere INVIO
        getchar();                                             // Attende input
        return 1;                                              // Termina il programma con codice di errore 1
    }

    printf("[DEBUG] Attendo messaggio di benvenuto dal server...\n"); // Messaggio di debug: il client attende la risposta di benvenuto
    nbytes = recvfrom(sock,                                   // Funzione per ricevere un datagram via UDP
                      buffer,                                 // Buffer in cui memorizzare i dati ricevuti
                      BUF_SIZE - 1,                           // Numero massimo di byte da ricevere (lasciando spazio per '\0')
                      0,                                      // Flag standard (nessuna opzione speciale)
                      (struct sockaddr *)&server_addr,        // Puntatore alla struttura che conterrà l'indirizzo del mittente (server)
                      &server_len);                           // Puntatore alla variabile che contiene la dimensione della struttura server_addr
    if (nbytes == SOCKET_ERROR) {                             // Controlla se recvfrom ha restituito un errore
        printf("recvfrom (welcome) failed: %d\n", WSAGetLastError()); // Stampa il codice di errore
        closesocket(sock);                                    // Chiude la socket
        WSACleanup();                                         // Libera le risorse Winsock
        printf("Premi INVIO per uscire...");                  // Chiede all'utente di premere INVIO
        getchar();                                            // Attende input
        return 1;                                             // Termina il programma con codice di errore 1
    }

    buffer[nbytes] = '\0';                                    // Aggiunge il terminatore di stringa '\0' alla fine dei dati ricevuti
    printf("Messaggio dal server: %s", buffer);               // Stampa il messaggio di benvenuto ricevuto dal server

    printf("a) Addizione\n");
    printf("s) Sottrazione\n");
    printf("m) Moltiplicazione\n");
    printf("d) Divisione\n");
    printf("Inserisci operazione (a/s/m/d o un'altra lettera per terminare): "); // Chiede all'utente di inserire il tipo di operazione
    if (!fgets(buffer, sizeof(buffer), stdin)) {              // Legge una riga da stdin e la salva in buffer
        printf("Errore input operazione\n");                  // Stampa messaggio di errore se fgets fallisce
        closesocket(sock);                                    // Chiude la socket
        WSACleanup();                                         // Libera le risorse Winsock
        printf("Premi INVIO per uscire...");                  // Chiede all'utente di premere INVIO
        getchar();                                            // Attende input
        return 1;                                             // Termina il programma con codice di errore 1
    }

    char op = buffer[0];                                      // Prende il primo carattere del buffer come lettera dell'operazione
    char op_buf[3];                                           // Definisce un piccolo buffer per inviare l'operazione (carattere + '\n' + '\0')
    op_buf[0] = op;                                           // Copia la lettera dell'operazione nel primo carattere
    op_buf[1] = '\n';                                         // Inserisce '\n' come secondo carattere, per coerenza col protocollo
    op_buf[2] = '\0';                                         // Termina la stringa con '\0'

    printf("[DEBUG] Invio operazione '%c' al server...\n", op); // Messaggio di debug: indica quale operazione viene inviata
    nbytes = sendto(sock,                                     // Invia il datagram contenente l'operazione al server
                    op_buf,                                   // Buffer dei dati da inviare (op + '\n')
                    (int)strlen(op_buf),                      // Lunghezza della stringa op_buf
                    0,                                        // Flag standard
                    (struct sockaddr *)&server_addr,          // Indirizzo del server
                    server_len);                              // Dimensione della struttura server_addr
    if (nbytes == SOCKET_ERROR) {                             // Controlla se sendto ha fallito
        printf("sendto (operazione) failed: %d\n", WSAGetLastError()); // Stampa codice d'errore
        closesocket(sock);                                    // Chiude la socket
        WSACleanup();                                         // Libera le risorse Winsock
        printf("Premi INVIO per uscire...");                  // Chiede all'utente di premere INVIO
        getchar();                                            // Attende input
        return 1;                                             // Termina il programma con codice di errore 1
    }

    printf("[DEBUG] Attendo risposta operazione dal server...\n"); // Messaggio di debug: attende la risposta relativa all'operazione
    nbytes = recvfrom(sock,                                    // Riceve la risposta del server sull'operazione
                      buffer,                                  // Buffer di destinazione
                      BUF_SIZE - 1,                            // Massimo numero di byte da ricevere
                      0,                                       // Flag standard
                      (struct sockaddr *)&server_addr,         // Indirizzo del server (riempito da recvfrom)
                      &server_len);                            // Dimensione della struttura server_addr
    if (nbytes == SOCKET_ERROR) {                              // Controlla se recvfrom ha restituito errore
        printf("recvfrom (operazione) failed: %d\n", WSAGetLastError()); // Stampa codice d'errore
        closesocket(sock);                                     // Chiude la socket
        WSACleanup();                                          // Libera le risorse Winsock
        printf("Premi INVIO per uscire...");                   // Chiede all'utente di premere INVIO
        getchar();                                             // Attende input
        return 1;                                              // Termina il programma con codice di errore 1
    }

    buffer[nbytes] = '\0';                                     // Aggiunge terminatore di stringa '\0' ai dati ricevuti
    printf("[DEBUG] Stringa risposta operazione: '%s'\n", buffer); // Stampa di debug: mostra la risposta ricevuta dal server

    if (strncmp(buffer, "TERMINE PROCESSO CLIENT", 23) == 0) { // Confronta i primi 23 caratteri per vedere se il server ha inviato il messaggio di termine
        printf("Risposta dal server: %s", buffer);             // Stampa il messaggio di termine ricevuto dal server
        closesocket(sock);                                     // Chiude la socket
        WSACleanup();                                          // Libera le risorse Winsock
        printf("Premi INVIO per uscire...");                   // Chiede all'utente di premere INVIO
        getchar();                                             // Attende input
        return 0;                                              // Termina il programma con codice di successo 0
    }

    printf("Operazione scelta dal server: %s", buffer);        // Stampa la descrizione dell'operazione inviata dal server

    long a, b;                                                 // Variabili per memorizzare i due interi inseriti dall'utente
    printf("Inserisci primo intero: ");                        // Chiede all'utente il primo intero
    if (scanf("%ld", &a) != 1) {                               // Legge il primo intero da stdin e controlla se è valido
        printf("Input non valido (primo intero).\n");          // Messaggio di errore se l'input non è un numero
        closesocket(sock);                                     // Chiude la socket
        WSACleanup();                                          // Libera le risorse Winsock
        printf("Premi INVIO per uscire...");                   // Chiede all'utente di premere INVIO
        getchar();                                             // Attende input
        return 1;                                              // Termina il programma con codice di errore 1
    }

    printf("Inserisci secondo intero: ");                      // Chiede all'utente il secondo intero
    if (scanf("%ld", &b) != 1) {                               // Legge il secondo intero da stdin e controlla la validità
        printf("Input non valido (secondo intero).\n");        // Messaggio di errore se l'input non è valido
        closesocket(sock);                                     // Chiude la socket
        WSACleanup();                                          // Libera le risorse Winsock
        printf("Premi INVIO per uscire...");                   // Chiede all'utente di premere INVIO
        getchar();                                             // Attende input
        return 1;                                              // Termina il programma con codice di errore 1
    }

    int c;                                                     // Variabile per consumare il carattere residuo nel buffer di input
    while ((c = getchar()) != '\n' && c != EOF) {              // Consuma caratteri rimanenti (es. '\n' generato da scanf)
        // Nessuna operazione all'interno del ciclo (solo pulizia del buffer di input)
    }

    char num_buf[BUF_SIZE];                                    // Buffer per convertire gli interi in stringa da inviare

    snprintf(num_buf, BUF_SIZE, "%ld\n", a);                   // Converte il primo intero in stringa con '\n' finale
    printf("[DEBUG] Invio primo intero: '%s'\n", num_buf);     // Messaggio di debug: mostra il primo intero che verrà spedito
    nbytes = sendto(sock,                                      // Invia il primo intero al server tramite UDP
                    num_buf,                                   // Puntatore ai dati (stringa del primo intero)
                    (int)strlen(num_buf),                      // Numero di byte da inviare
                    0,                                         // Flag standard
                    (struct sockaddr *)&server_addr,           // Indirizzo del server
                    server_len);                               // Dimensione della struttura server_addr
    if (nbytes == SOCKET_ERROR) {                              // Controlla se sendto ha restituito errore
        printf("sendto (primo intero) failed: %d\n", WSAGetLastError()); // Stampa il codice di errore
        closesocket(sock);                                     // Chiude la socket
        WSACleanup();                                          // Libera le risorse Winsock
        printf("Premi INVIO per uscire...");                   // Chiede all'utente di premere INVIO
        getchar();                                             // Attende input
        return 1;                                              // Termina il programma con codice di errore 1
    }

    snprintf(num_buf, BUF_SIZE, "%ld\n", b);                   // Converte il secondo intero in stringa con '\n' finale
    printf("[DEBUG] Invio secondo intero: '%s'\n", num_buf);   // Messaggio di debug: mostra il secondo intero che verrà spedito
    nbytes = sendto(sock,                                      // Invia il secondo intero al server
                    num_buf,                                   // Puntatore ai dati (stringa del secondo intero)
                    (int)strlen(num_buf),                      // Numero di byte da inviare
                    0,                                         // Flag standard
                    (struct sockaddr *)&server_addr,           // Indirizzo del server
                    server_len);                               // Dimensione della struttura server_addr
    if (nbytes == SOCKET_ERROR) {                              // Controlla se sendto ha restituito un errore
        printf("sendto (secondo intero) failed: %d\n", WSAGetLastError()); // Stampa il codice di errore
        closesocket(sock);                                     // Chiude la socket
        WSACleanup();                                          // Libera le risorse Winsock
        printf("Premi INVIO per uscire...");                   // Chiede all'utente di premere INVIO
        getchar();                                             // Attende input
        return 1;                                              // Termina il programma con codice di errore 1
    }

    printf("[DEBUG] Attendo risultato dal server...\n");       // Messaggio di debug: il client si mette in attesa del risultato
    nbytes = recvfrom(sock,                                   // Riceve il datagram con il risultato (o errore) dal server
                      buffer,                                 // Buffer di destinazione
                      BUF_SIZE - 1,                           // Numero massimo di byte da leggere
                      0,                                      // Flag standard
                      (struct sockaddr *)&server_addr,        // Indirizzo del server (riempito da recvfrom)
                      &server_len);                           // Dimensione della struttura server_addr
    if (nbytes == SOCKET_ERROR) {                             // Controlla se recvfrom ha restituito un errore
        printf("recvfrom (risultato) failed: %d\n", WSAGetLastError()); // Stampa il codice di errore
        closesocket(sock);                                    // Chiude la socket
        WSACleanup();                                         // Libera le risorse Winsock
        printf("Premi INVIO per uscire...");                  // Chiede all'utente di premere INVIO
        getchar();                                            // Attende input
        return 1;                                             // Termina il programma con codice di errore 1
    }

    buffer[nbytes] = '\0';                                    // Aggiunge terminatore di stringa '\0' ai dati ricevuti
    printf("Risultato dal server: %s", buffer);               // Stampa il risultato finale (o eventuale messaggio di errore) ricevuto dal server

    closesocket(sock);                                        // Chiude la socket del client
    WSACleanup();                                             // Libera tutte le risorse allocate da Winsock

    printf("Premi INVIO per uscire...");                      // Chiede all'utente di premere INVIO prima di chiudere la finestra
    getchar();                                                // Attende l'input finale dell'utente
    return 0;                                                 // Termina il programma con codice di successo 0
}
