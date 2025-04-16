#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <libpq-fe.h>
#include <time.h>

#define PORT 8081
#define MAX_CLIENTS 2
#define BUFFER_SIZE 1024

PGconn *conn;
void *handle_client(void *client_socket);
int execute_query(const char *query);
void setup_database();

void register_user(int sock, char* arg1, char* arg2);
int login_user(int sock, char* arg1, char* arg2);
void search_film(int sock, char* arg1);
void rent_film(int sock, int id_user, char* arg2);

int main()
{
    int server_fd, client_fd;
    struct sockaddr_in server_addr, client_addr;
    socklen_t client_len = sizeof(client_addr);
    pthread_t thread_id;

    setup_database();
    server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1)
    {
        perror("Socket creation failed");
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
    printf("Server in ascolto sulla porta %d...\n", PORT);

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

    while ((bytes_read = recv(sock, buffer, BUFFER_SIZE, 0)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Messaggio ricevuto: %s", buffer);
        
        char command[BUFFER_SIZE], arg1[BUFFER_SIZE], arg2[BUFFER_SIZE];
        sscanf(buffer, "%s %s %s", command, arg1, arg2);

        if (strcmp(command, "REGISTRAZIONE") == 0) {
            register_user(sock, arg1, arg2);
        } else if (strcmp(command, "LOGIN") == 0) {
            id_user = login_user(sock, arg1, arg2);
        } else if (strcmp(command, "CERCA") == 0) {
            search_film(sock, arg1);
        } else if (strcmp(command, "NOLEGGIA") == 0) {
            rent_film(sock, id_user, arg1);
        } else if (strcmp(command, "CARRELLO") == 0) {
            //view_cart(sock);
        } else if (strcmp(command, "CHECK OUT") == 0) {
            //check_out(sock);
        } else if (strcmp(command, "RESTITUISCI") == 0) {
            //return_film(sock, arg1, arg2);
        } else {
            send(sock, "Comando non valido\n", 20, 0);
        }
    }

    printf("Client disconnesso.\n");
    close(sock);
    return NULL;
}

void setup_database() {
    conn = PQconnectdb("dbname=videoteca user=postgres password=admin host=localhost");
    if (PQstatus(conn) != CONNECTION_OK) {
        fprintf(stderr, "Errore di connessione al database: %s", PQerrorMessage(conn));
        PQfinish(conn);
        exit(EXIT_FAILURE);
    }
    printf("Connessione al database avvenuta con successo!\n");
    execute_query("CREATE TABLE IF NOT EXISTS utenti (id SERIAL PRIMARY KEY, username VARCHAR(50) UNIQUE, password VARCHAR(50));");
    execute_query("CREATE TABLE IF NOT EXISTS film (id SERIAL PRIMARY KEY, titolo VARCHAR(100), copie_disponibili INT, copie_prestito INT);");
    execute_query("CREATE TABLE IF NOT EXISTS prestiti (id SERIAL PRIMARY KEY, id_utente INT, id_film INT, data_prestito TIMESTAMP, data_restituzione TIMESTAMP, FOREIGN KEY (id_utente) REFERENCES utenti(id), FOREIGN KEY (id_film) REFERENCES film(id));");
}



int execute_query(const char *query) {
    PGresult *res = PQexec(conn, query);
    printf("RES: %d\n", PQresultStatus(res));
    if (PQresultStatus(res) != PGRES_COMMAND_OK) {
        fprintf(stderr, "Errore nell'esecuzione della query: %s", PQerrorMessage(conn));
        return 0;
    }
    return 1;
    PQclear(res);
}


void register_user(int sock, char *username, char *password) {
    if(strcmp(username, "") == 0 || strcmp(password, "") == 0){
        send(sock, "Inserisci credenziali valide", 28, 0);
        return;
    }
    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE, "INSERT INTO utenti (username, password) VALUES ('%s', '%s');", username, password);
    
    if(execute_query(query) == 0){
        send(sock, "Utente già registrato", 21, 0);
        return;
    }
    send(sock, "Registrazione completata\n", 24, 0);
}


int login_user(int sock, char *username, char *password) {

    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE, "SELECT * FROM utenti WHERE username = '%s' AND password = '%s';", username, password);
    PGresult *res = PQexec(conn, query);

    if (PQntuples(res) == 0) {
        fprintf(stderr, "Errore nella query: %s\n", PQerrorMessage(conn));
        PQclear(res);
        send(sock, "Inserisci credenziali valide", 28, 0);
        return -1;
    }
    int idUser = atoi(PQgetvalue(res, 0, 0));
    
    //manda lo username al client
    char message[BUFFER_SIZE] = "";
    snprintf(message, BUFFER_SIZE, "Login effettuato con successo %s", PQgetvalue(res, 0, 1));
    send(sock, message, strlen(message), 0);

    PQclear(res);
    return idUser;

}


void search_film(int sock, char *title) {
    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE, "SELECT * FROM film WHERE titolo ILIKE '%s';", title);
    
    PGresult *res = PQexec(conn, query);
    int rows = PQntuples(res);
    if (rows == 0){
        send(sock, "Film non trovato\n", 17, 0);
        PQclear(res);
        return;
    }

    char message[BUFFER_SIZE];
    for (int i = 0; i < rows; i++) {
        snprintf(message, BUFFER_SIZE, "Film trovato: %s, Copie disponibili: %s\n", PQgetvalue(res, i, 1), PQgetvalue(res, i, 2));
        send(sock, message, strlen(message), 0);
    }

    PQclear(res);
}


void rent_film(int sock, int id_user, char *title) {

    if (id_user == -1) {
        send(sock, "Devi essere registrato!\n", 25, 0);
        return;
    }

    char query[BUFFER_SIZE];
    snprintf(query, BUFFER_SIZE, "SELECT id, copie_disponibili FROM film WHERE titolo ILIKE '%s';", title);
    
    PGresult *res = PQexec(conn, query);
    if (PQntuples(res) == 0) {
        send(sock, "Film non trovato\n", 17, 0);
        PQclear(res);
        return;
    } 

    int film_id = atoi(PQgetvalue(res, 0, 0));
    int available_copies = atoi(PQgetvalue(res, 0, 1));
    if (available_copies <= 0) {
        send(sock, "Nessuna copia disponibile\n", 26, 0);
        PQclear(res);
        return;
    }

    snprintf(query, BUFFER_SIZE, "INSERT INTO prestiti (id_utente, id_film, stato) "
                                         "VALUES ((SELECT id FROM utenti WHERE id = '%d'), %d, 'in_attesa');", 
                                         id_user, film_id);
    execute_query(query);
    send(sock, "Film aggiunto al carrello!\n", 28, 0);
    
    PQclear(res);
}