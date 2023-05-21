#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/select.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024
#define USER_FILE "user.txt"

typedef struct {
    int fd;
    char username[BUFFER_SIZE];
} Client;

int authenticate_client(char* username, char* password) {
    FILE* file = fopen(USER_FILE, "r");
    if (file == NULL) {
        perror("User file not found");
        exit(EXIT_FAILURE);
    }

    char line[BUFFER_SIZE];
    while (fgets(line, sizeof(line), file)) {
        line[strcspn(line, "\n")] = '\0';

        char stored_username[BUFFER_SIZE];
        char stored_password[BUFFER_SIZE];
        sscanf(line, "%s %s", stored_username, stored_password);

        if (strcmp(username, stored_username) == 0 && strcmp(password, stored_password) == 0) {
            fclose(file);
            return 1;
        }
    }

    fclose(file);
    return 0;
}

int main() {
    int server_fd, new_socket, activity, valread;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    int opt = 1;
    char buffer[BUFFER_SIZE];
    fd_set readfds;
    int client_sockets[MAX_CLIENTS] = {0};
    Client clients[MAX_CLIENTS];

    // Initialize client list
    for (int i = 0; i < MAX_CLIENTS; i++) {
        clients[i].fd = -1;
        strcpy(clients[i].username, "");
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

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);

        int max_fd = server_fd;
        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_fd = clients[i].fd;
            if (client_fd > 0) {
                FD_SET(client_fd, &readfds);
                if (client_fd > max_fd) {
                    max_fd = client_fd;
                }
            }
        }

        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0) {
            perror("Select failed");
            exit(EXIT_FAILURE);
        }

        if (FD_ISSET(server_fd, &readfds)) {
            if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }

            printf("New client connected\n");

            // Add new client to the list
            int client_index = -1;
            for (int i = 0; i < MAX_CLIENTS; i++) {
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
                client_sockets[client_index] = new_socket;
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++) {
            int client_fd = client_sockets[i];
            if (client_fd > 0 && FD_ISSET(client_fd, &readfds)) {
                memset(buffer, 0, sizeof(buffer));
                if ((valread = read(client_fd, buffer, BUFFER_SIZE)) == 0) {
                    // Client disconnected
                    printf("Client disconnected: %s\n", clients[i].username);
                    close(client_fd);
                    clients[i].fd = -1;
                    strcpy(clients[i].username, "");
                    client_sockets[i] = 0;
                } else {
                    if (strlen(clients[i].username) == 0) {
                        // Expecting username and password
                        char username[BUFFER_SIZE];
                        char password[BUFFER_SIZE];
                        sscanf(buffer, "%s %s", username, password);

                        if (authenticate_client(username, password)) {
                            strcpy(clients[i].username, username);
                            printf("Client authenticated: %s\n", clients[i].username);
                            send(client_fd, "Authentication successful. You can now execute commands.\n", strlen("Authentication successful. You can now execute commands.\n"), 0);
                        } else {
                            printf("Invalid credentials from client\n");
                            send(client_fd, "Authentication failed. Please provide valid username and password.\n", strlen("Authentication failed. Please provide valid username and password.\n"), 0);
                            close(client_fd);
                            clients[i].fd = -1;
                            strcpy(clients[i].username, "");
                            client_sockets[i] = 0;
                        }
                    } else {
                        // Execute command and send result to client
                        char command[BUFFER_SIZE];
                        sprintf(command, "echo \"%s\" | system > out.txt", buffer);
                        system(command);

                        FILE* output_file = fopen("out.txt", "r");
                        if (output_file == NULL) {
                            perror("Failed to open output file");
                            exit(EXIT_FAILURE);
                        }

                        char output_buffer[BUFFER_SIZE];
                        memset(output_buffer, 0, sizeof(output_buffer));
                        fread(output_buffer, sizeof(char), BUFFER_SIZE, output_file);
                        fclose(output_file);

                        send(client_fd, output_buffer, strlen(output_buffer), 0);
                    }
                }
            }
        }
    }

    return 0;
}
