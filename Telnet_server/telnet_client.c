#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 8080
#define BUFFER_SIZE 1024

int main() {
    int client_fd;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];

    // Create socket
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set server address
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &(server_address.sin_addr)) <= 0) {
        perror("Invalid address/Address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if (connect(client_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connect failed");
        exit(EXIT_FAILURE);
    }

    printf("Connected to the server.\n");

    while (1) {
        printf("Enter username: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        char username[BUFFER_SIZE];
        strcpy(username, buffer);

        printf("Enter password: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        char password[BUFFER_SIZE];
        strcpy(password, buffer);

        // Send username and password to the server
        char authentication[BUFFER_SIZE];
        sprintf(authentication, "%s %s", username, password);
        send(client_fd, authentication, strlen(authentication), 0);

        memset(buffer, 0, sizeof(buffer));
        int valread = read(client_fd, buffer, BUFFER_SIZE);
        printf("%s\n", buffer);

        if (strncmp(buffer, "Authentication successful", 24) == 0) {
            while (1) {
                printf("Enter command: ");
                fgets(buffer, BUFFER_SIZE, stdin);
                buffer[strcspn(buffer, "\n")] = '\0';

                // Send command to the server
                send(client_fd, buffer, strlen(buffer), 0);

                memset(buffer, 0, sizeof(buffer));
                valread = read(client_fd, buffer, BUFFER_SIZE);
                printf("%s\n", buffer);
            }
        } else {
            break;
        }
    }

    close(client_fd);

    return 0;
}
