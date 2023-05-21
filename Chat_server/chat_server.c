#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <poll.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

typedef struct {
    int fd;
    char name[BUFFER_SIZE];
} Client;

void handle_client_message(Client* clients, int client_index, char* message) {
    for (int i = 0; i < MAX_CLIENTS; i++) {
        if (clients[i].fd != -1 && i != client_index) {
            char formatted_message[BUFFER_SIZE];
            sprintf(formatted_message, "%s: %s", clients[client_index].name, message);
            send(clients[i].fd, formatted_message, strlen(formatted_message), 0);
        }
    }
}

int main() {
    int server_fd, new_socket, activity, valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;
    char buffer[BUFFER_SIZE];
    struct pollfd fds[MAX_CLIENTS];

    // Initialize client list
    Client clients[MAX_CLIENTS];
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        strcpy(clients[i].name, "");
    }

    // Create socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
        perror("Setsockopt failed");
        exit(EXIT_FAILURE);
    }

    // Bind socket to a port
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8080);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Start listening for connections
    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    printf("Waiting for clients...\n");

    // Accept incoming connections
    for (int i = 0; i < MAX_CLIENTS; i++) {
        fds[i].fd = -1;
    }

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    while (1) {
        activity = poll(fds, MAX_CLIENTS, -1);
        if (activity < 0) {
            perror("Poll failed");
            exit(EXIT_FAILURE);
        }

        if (fds[0].revents & POLLIN) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            printf("New client connected\n");

            // Add new client to the list
            int client_index = -1;
            for (int i = 1; i < MAX_CLIENTS; i++) {
                if (clients[i].fd == -1) {
                    client_index = i;
                    break;
                }
            }

            if (client_index == -1) {
                printf("Maximum number of clients reached. Connection rejected.\n");
                close(new_socket);
            } else {
                clients[client_index].fd = new_socket;
                fds[client_index].fd = new_socket;
                fds[client_index].events = POLLIN;
            }
        }

        for (int i = 1; i < MAX_CLIENTS; i++) {
            if (fds[i].revents & POLLIN) {
                memset(buffer, 0, sizeof(buffer));
                if ((valread = read(fds[i].fd, buffer, BUFFER_SIZE)) == 0) {
                    // Client disconnected
                    printf("Client disconnected: %s\n", clients[i].name);
                    close(fds[i].fd);
                    fds[i].fd = -1;
                    clients[i].fd = -1;
                    strcpy(clients[i].name, "");
                } else {
                    if (strlen(clients[i].name) == 0) {
                        // Expecting client name
                        strcpy(clients[i].name, buffer);
                        printf("Client name received: %s\n", clients[i].name);
                    } else {
                        // Handle client message
                        printf("Received message from %s: %s\n", clients[i].name, buffer);
                        handle_client_message(clients, i, buffer);
                    }
                }
            }
        }
    }

    return 0;
}
