#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define BUFFER_SIZE 1024

int main() {
    int client_fd;
    struct sockaddr_in server_address;
    char buffer[BUFFER_SIZE];
    char client_name[BUFFER_SIZE];

    // Tạo socket
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    // Thiết lập địa chỉ và cổng của server
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(8888);

    // Chuyển đổi địa chỉ IP từ dạng chuỗi sang dạng số nguyên
    if (inet_pton(AF_INET, "127.0.0.1", &(server_address.sin_addr)) <= 0) {
        perror("Invalid address/ Address not supported");
        exit(EXIT_FAILURE);
    }

    // Kết nối tới server
    if (connect(client_fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("Connection failed");
        exit(EXIT_FAILURE);
    }

    // Nhập tên client
    printf("Enter (client_id: \"your name\"): ");
    fgets(client_name, BUFFER_SIZE, stdin);
    client_name[strcspn(client_name, "\n")] = '\0';

    // Gửi tên client tới server
    snprintf(buffer, BUFFER_SIZE, client_name);
    send(client_fd, buffer, strlen(buffer), 0);

    // Nhận thông báo kết nối
    // Nhận thông báo kết nối thành công từ server
    memset(buffer, 0, sizeof(buffer));
    int valread = read(client_fd, buffer, BUFFER_SIZE - 1);
    printf("%s\n", buffer);

    // Cắt đi phần chuỗi "client_id: "
    sscanf(client_name, "client_id: %[^\n]", client_name);

    while (1) {
        // Nhập message từ client
        printf("%s: ", client_name);
        fgets(buffer, BUFFER_SIZE, stdin);
        buffer[strcspn(buffer, "\n")] = '\0';

        // Gửi message tới server
        send(client_fd, buffer, strlen(buffer), 0);

        // Nhận và in message từ server
        memset(buffer, 0, sizeof(buffer));
        valread = read(client_fd, buffer, BUFFER_SIZE - 1);
        printf("%s\n", buffer);
    }

    // Đóng kết nối
    close(client_fd);

    return 0;
}
