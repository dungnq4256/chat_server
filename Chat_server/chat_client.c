#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>

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

    // Prompt for client name
    printf("Enter your name: ");
    fgets(buffer, BUFFER_SIZE, stdin);
    buffer[strcspn(buffer, "\n")] = '\0';

    // Send client name to the server
    send(client_fd, buffer, strlen(buffer), 0);

    struct pollfd fds[1];
    fds[0].fd = client_fd;
    fds[0].events = POLLIN;

    while (1) {
        printf("Enter message: ");
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        // Send message to the server
        send(client_fd, buffer, strlen(buffer), 0);

        // Wait for incoming messages from the server
        int activity = poll(fds, 1, -1);
        if (activity < 0) {
            perror("Poll failed");
            exit(EXIT_FAILURE);
        }

        if (fds[0].revents & POLLIN) {
            memset(buffer, 0, sizeof(buffer));
            int valread = read(client_fd, buffer, BUFFER_SIZE);
            printf("%s\n", buffer);
        }
    }

    close(client_fd);

    return 0;
}
