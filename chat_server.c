#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <unistd.h>

#define MAX_CLIENTS 10
#define BUFFER_SIZE 1024

int main()
{
    int server_fd, client_fds[MAX_CLIENTS], max_fd, activity, i, valread, sd;
    int addrlen;
    struct sockaddr_in address;

    char buffer[BUFFER_SIZE];
    char client_names[MAX_CLIENTS][BUFFER_SIZE];
    char expected_prefix[] = "client_id: ";
    int expected_prefix_len = strlen(expected_prefix);

    fd_set readfds;

    // Khởi tạo mảng client_fds với giá trị -1 để đánh dấu các vị trí chưa kết nối
    for (i = 0; i < MAX_CLIENTS; i++)
    {
        client_fds[i] = -1;
    }

    // Tạo socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa chỉ và cổng
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(8888);

    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Lắng nghe kết nối
    if (listen(server_fd, MAX_CLIENTS) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    addrlen = sizeof(address);
    puts("Waiting for connections...");

    while (1)
    {
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        max_fd = server_fd;

        // Thêm các client file descriptors vào set
        for (i = 0; i < MAX_CLIENTS; i++)
        {
            sd = client_fds[i];
            if (sd > 0)
                FD_SET(sd, &readfds);
            if (sd > max_fd)
                max_fd = sd;
        }

        // Chờ đợi hoạt động trên các file descriptors
        activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);

        if (activity < 0)
        {
            perror("Select error");
        }

        // Kết nối mới
        if (FD_ISSET(server_fd, &readfds))
        {
            int new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
            if (new_socket < 0)
            {
                perror("Accept error");
                exit(EXIT_FAILURE);
            }

            // Đọc tên client
            memset(buffer, 0, sizeof(buffer));
            valread = read(new_socket, buffer, BUFFER_SIZE - 1);
            // Kiểm tra cú pháp
            if (strncmp(buffer, expected_prefix, expected_prefix_len) == 0)
            {
                strcpy(client_names[new_socket], buffer + expected_prefix_len);
                printf("New connection: %s\n", client_names[new_socket]);
                // Gửi phản hồi cho client
                char *response = "Kết nối thành công";
                send(new_socket, response, strlen(response), 0);
            }
            else
            {
                printf("Client gửi cú pháp không hợp lệ\n");
                // Gửi phản hồi cho client
                char *response = "Kết nối không thành công";
                send(new_socket, response, strlen(response), 0);
                exit(EXIT_FAILURE);
            }
            // sscanf(buffer, "client_id: %[^\n]", client_names[new_socket]);

            // // In thông báo kết nối mới
            // printf("New connection: %s\n", client_names[new_socket]);

            // // Gửi thông báo kết nối thành công cho client
            // char *welcome_message = "Connected to the chat server";
            // send(new_socket, welcome_message, strlen(welcome_message), 0);

            // Thêm client socket
            // Thêm client socket vào mảng client_fds
            for (i = 0; i < MAX_CLIENTS; i++)
            {
                if (client_fds[i] == -1)
                {
                    client_fds[i] = new_socket;
                    break;
                }
            }
        }

        // Xử lý input từ các client kết nối
        for (i = 0; i < MAX_CLIENTS; i++)
        {
            sd = client_fds[i];
            if (FD_ISSET(sd, &readfds))
            {
                // Đọc message từ client
                memset(buffer, 0, sizeof(buffer));
                valread = read(sd, buffer, BUFFER_SIZE - 1);

                // Ngắt kết nối nếu client đóng kết nối
                if (valread == 0)
                {
                    getpeername(sd, (struct sockaddr *)&address, (socklen_t *)&addrlen);
                    printf("Client disconnected: %s\n", client_names[sd]);
                    close(sd);
                    client_fds[i] = -1;
                }
                // Gửi message từ client cho các client khác
                else
                {
                    char message[BUFFER_SIZE];
                    snprintf(message, BUFFER_SIZE, "%s: %s", client_names[sd], buffer);

                    // In message lên server
                    printf("%s\n", message);

                    // Gửi message cho các client khác
                    for (int j = 0; j < MAX_CLIENTS; j++)
                    {
                        int dest_sd = client_fds[j];
                        if (dest_sd != -1 && dest_sd != sd)
                        {
                            send(dest_sd, message, strlen(message), 0);
                        }
                    }
                }
            }
        }
    }

    return 0;
}
