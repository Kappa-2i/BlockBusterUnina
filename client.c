#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>


#define GREEN "\x1b[32m"
#define YELLOW "\x1b[33m"
#define CYAN "\x1b[36m"
#define WHITE "\x1b[0m"

#define BUFFER_SIZE 1024

void printMenu(const char *);
void handleServerResponse(char*, char*);

int main()
{
    char buffer[BUFFER_SIZE];
    char current_user[BUFFER_SIZE] = "";

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_aton("127.0.0.1", &(server_addr.sin_addr));

    if (connect(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Errore");
        exit(EXIT_FAILURE);
    }

    printf("Connesso al server\n");
    while (1){
        printMenu(current_user);

        //fgets(buffer, BUFFER_SIZE, stdin);

        if (fgets(buffer, BUFFER_SIZE, stdin) == NULL) {
            printf("Errore nella lettura dell'input.\n");
            continue;
        }
        
        buffer[strcspn(buffer, "\n")] = 0;
        if (!strcmp(buffer, "ESCI") || !strcmp(buffer, "0")){
            break;
        }


        send(sock, buffer, strlen(buffer), 0);

        int bytes_received = recv(sock, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received > 0){
            buffer[bytes_received] = '\0';
            handleServerResponse(buffer, current_user);
        }
    }
    close(sock);
    return 0;
}

void printMenu(const char *current_user){

    if (strlen(current_user) > 0){
        printf("\n%s👤 Utente loggato: %s%s\n\n", GREEN, current_user, WHITE);
    }

    printf("\n%s╔════════════════════════════════════╗%s\n", CYAN, WHITE);
    printf("%s║%s        🎬  %sMENU PRINCIPALE%s         %s║%s\n", CYAN, WHITE, YELLOW, WHITE, CYAN, WHITE);
    printf("%s╠════════════════════════════════════╣%s\n", CYAN, WHITE);
    printf("%s║%s  1. REGISTRAZIONE <user> <pass>    %s║%s\n", CYAN, WHITE, CYAN, WHITE);
    printf("%s║%s  2. LOGIN <user> <pass>            %s║%s\n", CYAN, WHITE, CYAN, WHITE);
    printf("%s║%s  3. CERCA <title>                  %s║%s\n", CYAN, WHITE, CYAN, WHITE);
    printf("%s║%s  4. AGGIUNGI_AL_CARRELLO <title>   %s║%s\n", CYAN, WHITE, CYAN, WHITE);
    printf("%s║%s  5. RIMUOVI_DAL_CARRELLO <title>   %s║%s\n", CYAN, WHITE, CYAN, WHITE);
    printf("%s║%s  6. VISUALIZZA_CARRELLO            %s║%s\n", CYAN, WHITE, CYAN, WHITE);
    printf("%s║%s  7. CHECKOUT                       %s║%s\n", CYAN, WHITE, CYAN, WHITE);
    printf("%s║%s  8. RESTITUISCI_FILM <title>       %s║%s\n", CYAN, WHITE, CYAN, WHITE);
    printf("%s║%s  9. VISUALIZZA_PRESTITI            %s║%s\n", CYAN, WHITE, CYAN, WHITE);
    printf("%s║%s  10. VISUALIZZA_MESSAGGI           %s║%s\n", CYAN, WHITE, CYAN, WHITE);
    printf("%s║%s  0. ESCI                           %s║%s\n", CYAN, WHITE, CYAN, WHITE);
    printf("%s╚════════════════════════════════════╝%s\n", CYAN, WHITE);
    printf("%s❯ Seleziona un'opzione: %s", GREEN, WHITE);
}

void handleServerResponse(char* buffer, char* current_user) {
    system("clear");
    printf("\n%s📨 Risposta del server:%s %s\n", YELLOW, WHITE, buffer);

    if (strncmp(buffer, "Login effettuato con successo ", 30) == 0) {
        sscanf(buffer + 30, "%s", current_user);
    }
}