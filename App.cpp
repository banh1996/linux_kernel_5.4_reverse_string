#include <iostream>
#include <fstream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <unistd.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 6789
#define BUFFER_SIZE 1024

int main(int argc, char *argv[]) {
    std::string line;
    char buffer[BUFFER_SIZE];
    struct sockaddr_in serverAddr{};
    socklen_t serverAddrLen = sizeof(serverAddr);
    ssize_t receivedBytes;
    ssize_t sentBytes;
    std::string filename;
    int sockfd;

    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <filename>" << std::endl;
        return 1;
    }

    filename = argv[1];
    std::ifstream file(filename);

    if (!file) {
        std::cerr << "Failed to open file: " << filename << std::endl;
        return 1;
    }

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        std::cerr << "Failed to create socket" << std::endl;
        return 1;
    }

    serverAddr.sin_family = AF_INET;
    serverAddr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &(serverAddr.sin_addr)) <= 0) {
        std::cerr << "Failed to set server address" << std::endl;
        close(sockfd);
        return 1;
    }

    while (std::getline(file, line)) {
        sentBytes = sendto(sockfd, line.c_str(), line.length(), 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr));
        if (sentBytes < 0) {
            std::cerr << "Failed to send message" << std::endl;
            close(sockfd);
            return 1;
        }

        receivedBytes = recvfrom(sockfd, buffer, BUFFER_SIZE - 1, 0, (struct sockaddr *)&serverAddr, &serverAddrLen);
        if (receivedBytes < 0) {
            std::cerr << "Failed to receive message" << std::endl;
            close(sockfd);
            return 1;
        }

        buffer[receivedBytes] = '\0';
        std::cout <<buffer << std::endl;
    }

    //std::cout << "Messages sent to and received from kernel module" << std::endl;

    close(sockfd);

    return 0;
}