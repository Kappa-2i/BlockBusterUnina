#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <libpq-fe.h>
#include <time.h>

#define PORT 8080
#define MAX_CLIENTS 2
#define BUFFER_SIZE 1024
#define MAX_RENT 10

pthread_mutex_t db_mutex = PTHREAD_MUTEX_INITIALIZER;

PGconn *conn;
void *handle_client(void *client_socket);
int execute_query(const char *query);
PGresult* execute_select(const char* query);
void setup_database();

void register_user(int sock, char* arg1, char* arg2);
int login_user(int sock, char* arg1, char* arg2);
void search_film(int sock, char* arg1);
void add_to_cart(int sock, int id_user, char* arg1);
void remove_from_cart(int sock, int id_user, char* arg1);
void view_cart(int sock, int id_user);
void check_out(int sock, int id_user);
void return_film(int sock, int id_user, char* arg1);
void view_rent(int sock, int id_user);
void view_notifications(int sock, int id_user);


int main()
{
    printf("Avvio del server...\n");
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t thread_id;

    setup_database();
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("Creazione Socket Fallita");
        exit(EXIT_FAILURE);
    }

    // Adattiamo gli attributi del server alla socket
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);

    // Assegna alla socket un indirizzo IP e la porta su cui ascoltare.
    if (bind(server_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Mette il socket in attesa di ricevere richieste dai client (MAX 10 Client in coda)
    if (listen(server_fd, MAX_CLIENTS) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }
    printf("Server in ascolto sulla porta %d...\n", PORT);  // Aggiungi un log qui
    fflush(stdout);
    while (1)
    {
        client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0)
        {
            perror("Accept failed");
            continue;
        }
        printf("Nuovo client connesso!\n");
        pthread_create(&thread_id, NULL, handle_client, (void *)&client_fd);
        pthread_detach(thread_id);
    }

    PQfinish(conn);
    close(server_fd);
    return 0;
}

void *handle_client(void *client_socket){
    int sock = * (int *)client_socket;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_read;
    int id_user = -1; 

    while ((bytes_read = recv(sock, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Messaggio ricevuto: %s", buffer);

        // Parsing con strtok
        char *command = strtok(buffer, " ");
        char *args = strtok(NULL, ""); // prende TUTTO il resto

        if (command == NULL) {
            send(sock, "Comando non valido\n", 20, 0);
            continue;
        }

        if (!strcmp(command, "REGISTRAZIONE") || !strcmp(command, "1")) {
            char *username = strtok(args, " ");
            char *password = strtok(NULL, " ");
            register_user(sock, username, password);
        } else if (!strcmp(command, "LOGIN") || !strcmp(command, "2")) {
            char *username = strtok(args, " ");
            char *password = strtok(NULL, " ");
            id_user = login_user(sock, username, password);
        } else if (!strcmp(command, "CERCA") || !strcmp(command, "3")) {
            search_film(sock, args);  
        } else if (!strcmp(command, "AGGIUNGI_AL_CARRELLO" ) || !strcmp(command, "4")) {
            add_to_cart(sock, id_user, args);
        } else if (!strcmp(command, "RIMUOVI_DAL_CARRELLO") || !strcmp(command, "5")) {
            remove_from_cart(sock, id_user, args);
        } else if (!strcmp(command, "VISUALIZZA_CARRELLO") || !strcmp(command, "6")) {
            view_cart(sock, id_user);
        } else if (!strcmp(command, "CHECKOUT") || !strcmp(command, "7")) {
            check_out(sock, id_user);
        } else if (!strcmp(command, "RESTITUISCI_FILM") || !strcmp(command, "8")) {
            return_film(sock, id_user, args);
        } else if (!strcmp(command, "VISUALIZZA_PRESTITI") || !strcmp(command, "9")) {
            view_rent(sock, id_user);
        } else if (!strcmp(command, "VISUALIZZA_MESSAGGI") || !strcmp(command, "10")) {
            view_notifications(sock, id_user); 
        } else {
            send(sock, "Comando non valido\n", 20, 0);
        }
    }


    printf("Client disconnesso.\n");
    close(sock);
    return NULL;
}

void setup_database() {
    for (int i = 0; i < 10; i++) {
        conn = PQconnectdb("port=5432 dbname=videoteca user=postgres password=admin host=db");
        if (PQstatus(conn) == CONNECTION_OK) break;
        printf("Connessione fallita, ritento in 2 secondi...\n");
        PQfinish(conn);
        sleep(2);
    }

    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Errore di connessione al database: %s", PQerrorMessage(conn));
        PQfinish(conn);
        exit(EXIT_FAILURE);
    }
    printf("Connessione al database avvenuta con successo!\n");
}



int execute_query(const char *query) {

    pthread_mutex_lock(&db_mutex);  
    PGresult *res = PQexec(conn, query);
    pthread_mutex_unlock(&db_mutex);

    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Errore nell'esecuzione della query: %s", PQerrorMessage(conn));
        PQclear(res);
        return 0;
    }

    PQclear(res);
    return 1;
}

PGresult* execute_select(const char* query) {
    PGresult *res;

    pthread_mutex_lock(&db_mutex);
    res = PQexec(conn, query);
    pthread_mutex_unlock(&db_mutex);

    if (!res || PQresultStatus(res) != PGRES_TUPLES_OK) {
        if (res) PQclear(res);
        return NULL;
    }

    return res;
}


void register_user(int sock, char *username, char *password) {
     if(!username || !password){
         send(sock, "Inserisci credenziali valide", 28, 0);
         return;
    }
    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE, "INSERT INTO utenti (username, password) VALUES ('%s', '%s');", username, password);
    
    if(execute_query(query) == 0){
        send(sock, "Utente già registrato", 23, 0);
        return;
    }
    send(sock, "Registrazione completata\n", 24, 0);
}


int login_user(int sock, char *username, char *password) {

    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE,
             "SELECT id, username FROM utenti WHERE username = '%s' AND password = '%s';",
             username, password);
   PGresult *res = execute_select(query);
    if (!res || PQntuples(res) == 0) {
        if (res) PQclear(res);
        send(sock, "Inserisci credenziali valide", 28, 0);
        return -1;
    }

    int idUser = atoi(PQgetvalue(res, 0, 0));
    const char* nomeUtente = PQgetvalue(res, 0, 1);
    PQclear(res);


    snprintf(query, BUFFER_SIZE,
             "UPDATE prestiti SET stato = 'scaduto' "
             "WHERE stato = 'effettuato' AND id_utente = %d AND data_restituzione < CURRENT_DATE;",
             idUser);
    execute_query(query);

    snprintf(query, BUFFER_SIZE,
             "SELECT COUNT(*) FROM prestiti WHERE id_utente = %d AND stato = 'scaduto';",
             idUser);
    res = execute_select(query);
    if (!res) return -1;

    int countScaduti = atoi(PQgetvalue(res, 0, 0));
    PQclear(res);

    char message[BUFFER_SIZE] = "";
    if (countScaduti > 0) {
        snprintf(message, BUFFER_SIZE,
                "Login effettuato con successo %s\n\n🔔(%d) Hai dei nuovi messaggi!",
                nomeUtente, countScaduti);
    } else {
        snprintf(message, BUFFER_SIZE,
                "Login effettuato con successo %s", nomeUtente);
    }

    send(sock, message, strlen(message), 0);
    return idUser;
}



void search_film(int sock, char *title) {

    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE, "SELECT * FROM film WHERE titolo ILIKE '%s';", title);
    
    PGresult *res = execute_select(query);
    if (!res || PQntuples(res) == 0 ){
        if (res) PQclear(res);
        send(sock, "Film non trovato\n", 17, 0);
        return;
    }

    char message[BUFFER_SIZE];
    snprintf(message, BUFFER_SIZE, "Film trovato: %s, Copie disponibili: %s\n", PQgetvalue(res, 0, 1), PQgetvalue(res, 0, 2));
    send(sock, message, strlen(message), 0);

    PQclear(res);
}


void add_to_cart(int sock, int id_user, char *title) {

    if (id_user == -1) {
        send(sock, "Devi essere registrato!\n", 25, 0);
        return;
    }

    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE, "SELECT id, copie_disponibili FROM film WHERE titolo ILIKE '%s';", title);
    
    PGresult *res = execute_select(query);
    if (!res || PQntuples(res) == 0) {
        if (res) PQclear(res);
        send(sock, "Film non trovato\n", 18, 0);
        return;
    } 

    int film_id = atoi(PQgetvalue(res, 0, 0));
    int available_copies = atoi(PQgetvalue(res, 0, 1));
    if (available_copies <= 0) {
        send(sock, "Nessuna copia disponibile\n", 27, 0);
        PQclear(res);
        return;
    }

    snprintf(query, BUFFER_SIZE, "INSERT INTO prestiti (id_utente, id_film, stato) "
                                         "VALUES (%d, %d, 'in_attesa');", 
                                         id_user, film_id);
    execute_query(query);

    snprintf(query, BUFFER_SIZE, "UPDATE film SET copie_disponibili = copie_disponibili - 1, copie_prestito = copie_prestito + 1 WHERE id = %d;", film_id );
    execute_query(query);
    
    send(sock, "Film aggiunto al carrello!\n", 28, 0);

}


void remove_from_cart(int sock, int id_user, char* title) {
    if (id_user == -1) {
        send(sock, "Devi essere registrato!\n", 25, 0);
        return;
    }

    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE, "SELECT * FROM prestiti WHERE id_film = (SELECT id FROM film WHERE titolo ILIKE '%s') AND id_utente = %d AND stato = 'in_attesa'; ", title, id_user);
    
    PGresult *res = execute_select(query);
    if (!res || PQntuples(res) == 0) {
        if (res) PQclear(res);
        send(sock, "Film non trovato\n", 18, 0);
        return;
    } 

    snprintf(query, BUFFER_SIZE, "DELETE FROM prestiti " 
                                    "WHERE id IN ( "
                                    "SELECT id FROM prestiti "
                                    "WHERE id_utente = %d "
                                        "AND id_film = (SELECT id FROM film WHERE titolo ILIKE '%s') "
                                        "AND stato = 'in_attesa' "
                                    "LIMIT 1); ", id_user, title);
    execute_query(query);

    snprintf(query, BUFFER_SIZE, "UPDATE film SET copie_disponibili = copie_disponibili + 1, copie_prestito = copie_prestito - 1 WHERE titolo ILIKE '%s';", title);
    execute_query(query);

    send(sock, "Film rimosso dal carrello!\n", 28, 0);

}


void view_cart(int sock, int id_user) {
    if (id_user == -1) {
        send(sock, "Devi essere registrato!\n", 25, 0);
        return;
    }

    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE,
        "SELECT titolo FROM prestiti p JOIN film f ON p.id_film = f.id "
        "WHERE id_utente = %d AND stato = 'in_attesa';", id_user);

    PGresult *res = execute_select(query);
    if (!res || PQntuples(res) == 0) {
        if (res) PQclear(res);
        send(sock, "Carrello vuoto!\n", 18, 0);
        return;
    }

    int rows = PQntuples(res);

    char message[BUFFER_SIZE * 4] = "\n";  
    for (int i = 0; i < rows; i++) {
        char line[BUFFER_SIZE];
        snprintf(line, BUFFER_SIZE, "%d - %s\n", i+1, PQgetvalue(res, i, 0));
        strncat(message, line, sizeof(message) - strlen(message) - 1);
    }

    send(sock, message, strlen(message), 0);
    PQclear(res);
}

void check_out(int sock, int id_user){

    if (id_user == -1) {
        send(sock, "Devi essere registrato!\n", 25, 0);
        return;
    }

    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE,
        "SELECT titolo FROM prestiti p JOIN film f ON p.id_film = f.id "
        "WHERE id_utente = %d AND stato = 'in_attesa';", id_user);

    PGresult *res = execute_select(query);
    

    if (!res || PQntuples(res) == 0) {
        if (res) PQclear(res);
        send(sock, "Carrello vuoto!\n", 18, 0);
        return;
    } 

    int rows = PQntuples(res);
    
    snprintf(query, BUFFER_SIZE,
             "SELECT COUNT(*) FROM prestiti WHERE id_utente = %d AND (stato = 'scaduto' OR stato = 'effettuato');",
             id_user);
    res = execute_select(query);
    if (!res) return;

    int countPrestiti = atoi(PQgetvalue(res, 0, 0));

    if((countPrestiti + rows) > MAX_RENT) {
        char message[BUFFER_SIZE];
        snprintf(message, BUFFER_SIZE, "Puoi noleggiare al massimo %d film!\n", MAX_RENT);
        send(sock, message, strlen(message), 0);
        PQclear(res);
        return;
    }

    PQclear(res);

    snprintf(query, BUFFER_SIZE, "UPDATE prestiti SET stato = 'effettuato', data_prestito = CURRENT_DATE, data_restituzione = CURRENT_DATE + INTERVAL '7 days' WHERE id_utente = %d AND stato = 'in_attesa';", id_user);
    execute_query(query);

    send(sock, "Film noleggiati con successo!\n", 31, 0);
}


void return_film(int sock, int id_user, char *title){

    if (id_user == -1) {
        send(sock, "Devi essere registrato!\n", 25, 0);
        return;
    }

    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE,
        "SELECT * FROM prestiti "
        "WHERE id_utente = %d AND (stato = 'effettuato' OR stato = 'scaduto'); ", id_user);

    PGresult *res = execute_select(query);
    
    if (!res || PQntuples(res) == 0) {
        if (res) PQclear(res);
        send(sock, "Non hai nessun film da restituire!\n", 33, 0);
        return;
    }
    int rows = PQntuples(res);
    PQclear(res);

    snprintf(query, BUFFER_SIZE,
        "SELECT * FROM prestiti p JOIN film f ON p.id_film = f.id "
        "WHERE id_utente = %d AND f.titolo = '%s' AND (stato = 'effettuato' or stato = 'scaduto'); ", id_user, title);

    res = execute_select(query);
    
    if (!res || PQntuples(res) == 0) {
        if (res) PQclear(res);
        send(sock, "Non hai noleggiato questo film!\n", 33, 0);
        return;
    }
    

    snprintf(query, BUFFER_SIZE, "DELETE FROM prestiti " 
                                    "WHERE id IN ( "
                                    "SELECT id FROM prestiti "
                                    "WHERE id_utente = %d "
                                        "AND id_film = (SELECT id FROM film WHERE titolo ILIKE '%s') "
                                        "AND (stato = 'effettuato' OR stato = 'scaduto') "
                                        "ORDER BY data_restituzione "
                                    "LIMIT 1); ", id_user, title);
    execute_query(query);

    snprintf(query, BUFFER_SIZE, "UPDATE film SET copie_disponibili = copie_disponibili + 1, copie_prestito = copie_prestito - 1 WHERE titolo ILIKE '%s';", title);
    execute_query(query);

    send(sock, "Film restituito con successo!\n", 31, 0);
}



void view_rent(int sock, int id_user){
    if (id_user == -1) {
        send(sock, "Devi essere registrato!\n", 25, 0);
        return;
    }

    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE,
        "SELECT titolo, TO_CHAR(data_restituzione, 'DD-MM-YYYY') AS data_restituzione FROM prestiti p JOIN film f ON p.id_film = f.id "
        "WHERE id_utente = %d AND (stato = 'effettuato' OR stato = 'scaduto') ;", id_user);

    PGresult *res = execute_select(query);
    
    if (!res || PQntuples(res) == 0) {
        if (res) PQclear(res);
        send(sock, "Nessun film noleggiato!\n", 25, 0);
        return;
    }

    int rows = PQntuples(res);

    char message[BUFFER_SIZE * 4] = "\n"; 
    for (int i = 0; i < rows; i++) {
        char line[BUFFER_SIZE];
        snprintf(line, BUFFER_SIZE, "%d - %s da consegnare entro il %s\n", i+1, PQgetvalue(res, i, 0), PQgetvalue(res, i, 1));
        strncat(message, line, sizeof(message) - strlen(message) - 1);
    }

    send(sock, message, strlen(message), 0);
    PQclear(res);
}


void view_notifications(int sock, int id_user){

    if (id_user == -1) {
        send(sock, "Devi essere registrato!\n", 25, 0);
        return;
    }
    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE,
        "SELECT f.titolo, TO_CHAR(p.data_restituzione, 'DD-MM-YYYY') FROM prestiti p JOIN film f ON p.id_film = f.id "
        "WHERE id_utente = %d AND p.id_film = f.id AND stato = 'scaduto'; ", id_user);
    PGresult *res = execute_select(query);

    
    if (!res || PQntuples(res) == 0) {
        if (res) PQclear(res);
        send(sock, "Non hai nessun messaggio!\n", 27, 0);
        return;
    }

    int rows = PQntuples(res);

    char message[BUFFER_SIZE * 4] = "\n";  // 
    for (int i = 0; i < rows; i++) {
        char line[BUFFER_SIZE];
        snprintf(line, BUFFER_SIZE, "%d - Ti ricordiamo che il periodo di noleggio del film %s è scaduto in data %s.\n", i+1, PQgetvalue(res, i, 0), PQgetvalue(res, i, 1));
        strncat(message, line, sizeof(message) - strlen(message) - 1);
    }

    send(sock, message, strlen(message), 0);
    PQclear(res);
}