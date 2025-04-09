#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h> 
#include <sys/types.h> 
#include <arpa/inet.h>


#define BUFFER_SIZE 1024


int main(){
    char buffer[BUFFER_SIZE];

    int sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);


    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);
    inet_aton("127.0.0.1", &(server_addr.sin_addr));

    if(connect(sock, (struct sockaddr*) &server_addr, sizeof(server_addr))<0){
        perror("Errore");
        exit(EXIT_FAILURE);
    }

    printf("Connesso al server");
     while (1) {
        //Codice
    }
    
    close(sock);
    printf("Disconnesso dal server.\n");
    return 0;
}