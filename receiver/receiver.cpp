#include <iostream>      // std::cout, std::cerr — вывод в консоль
#include <cstring>       // memset() — обнулить структуру sockaddr_in

#include <unistd.h>      // close() — закрыть файловый дескриптор
#include <arpa/inet.h>   // htons(), INADDR_ANY, inet_pton() — работа с сетевыми адресами
#include <sys/socket.h>  // socket(), bind(), listen(), accept() — сами системные вызовы
#include <netinet/in.h>  // struct sockaddr_in — структура адреса

int main()
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
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

    if (bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1) {
        std::cerr << "Bind failed!" << std::endl;
        return 1;
    }

    listen(sockfd, 1);
    std::cout << "Listening on port 5000!" << std::endl;

    int client_sock = accept(sockfd, nullptr, nullptr);
    if (client_sock == -1) {
        std::cerr << "Accept failed!" << std::endl;
        return 1;
    }
    std::cout << "Client connected!" << std::endl;

    close(client_sock);
    close(sockfd);

    return 0;
}
