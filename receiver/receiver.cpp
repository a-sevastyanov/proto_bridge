#include <iostream>      // std::cout, std::cerr — вывод в консоль
#include <cstring>       // memset() — обнулить структуру sockaddr_in
#include <cstdint>
#include <vector>

#include <unistd.h>      // close() — закрыть файловый дескриптор
#include <arpa/inet.h>   // htons(), INADDR_ANY, inet_pton() — работа с сетевыми адресами
#include <sys/socket.h>  // socket(), bind(), listen(), accept() — сами системные вызовы
#include <netinet/in.h>  // struct sockaddr_in — структура адреса

#include "proto/user.pb.h"

std::vector<uint8_t> recv_exact(int sock_fd, size_t n)
{
    std::vector<uint8_t> buffer(n);
    size_t total_bytes_read = 0;

    while (total_bytes_read < n) {
        // Читаем в оставшуюся часть буфера
        ssize_t bytes_read = recv(sock_fd, buffer.data() + total_bytes_read, n - total_bytes_read, 0);
        if (bytes_read == -1) {
            std::cerr << "Receive failed!" << std::endl;
            return {};
        } else if (bytes_read == 0) {
            std::cerr << "Connection closed by peer before reading " << n << " bytes" << std::endl;
            return {};
        }
        total_bytes_read += bytes_read;
    }
    return buffer;
}

int main()
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        std::cerr << "Socket initialization failed!" << std::endl;
        return 1;
    }

    sockaddr_in server_addr;
    // Обнуление структуры обязательно, т.к. сейчас там мусор
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(5000);
    server_addr.sin_addr.s_addr = INADDR_ANY;  // слушать на всех интерфейсах
    // Если нужно указать конкретный IP, нужно использовать inet_pton()

    if (bind(server_fd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Bind failed!" << std::endl;
        return 1;
    }

    listen(server_fd, 1);
    std::cout << "Listening on port 5000!" << std::endl;

    int conn_fd = accept(server_fd, nullptr, nullptr);
    if (conn_fd == -1) {
        std::cerr << "Accept failed!" << std::endl;
        return 1;
    }
    std::cout << "Client connected!" << std::endl;

    std::vector<uint8_t> header_bytes = recv_exact(conn_fd, 4);
    if (header_bytes.size() != 4) {
        std::cerr << "Connection probably closed!" << std::endl;
        return 1;
    }
    uint32_t raw_len = 0;
    std::memcpy(&raw_len, header_bytes.data(), sizeof(raw_len));
    uint32_t message_len = ntohl(raw_len);  // "network to host long"

    std::vector<uint8_t> message_bytes = recv_exact(conn_fd, message_len);
    if (message_bytes.size() != message_len) {
        std::cerr << "Failed to read full message!" << std::endl;
        return 1;
    }
    protobridge::UserInfo user;
    if (!user.ParseFromArray(message_bytes.data(), message_bytes.size())) {
        std::cerr << "Parsing from array failed!" << std::endl;
        return 1;
    }

    std::cout << "id: " << user.id() << std::endl;
    std::cout << "username: " << user.username() << std::endl;
    std::cout << "email: " << user.email() << std::endl;
    std::cout << "age: " << user.age() << std::endl;
    std::cout << "is_active: " << user.is_active() << std::endl;
    std::cout << "timestamp: " << user.timestamp() << std::endl;

    std::cout << "roles: ";
    for (int i = 0; i < user.roles_size(); ++i) {
        std::cout << protobridge::Role_Name(user.roles(i)) << " ";
    }
    std::cout << std::endl;

    close(conn_fd);
    close(server_fd);

    return 0;
}
