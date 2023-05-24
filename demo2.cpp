#include <iostream>
#include <fstream>
#include <string>
#include <unistd.h>
#include <stdexcept>
#include <cstring>
#include <arpa/inet.h>
#include <sys/socket.h>

void sendVote(const std::string& serverName, const int portNum, const std::string& vote) {
    struct sockaddr_in server_addr;
    int sock = -1;
    try {
        // Create socket
        sock = socket(AF_INET , SOCK_STREAM , 0);
        if (sock == -1) {
            throw std::runtime_error("Could not create socket");
        }

        server_addr.sin_addr.s_addr = inet_addr(serverName.c_str());
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(portNum);

        // Connect to remote server
        if (connect(sock , (struct sockaddr *)&server_addr , sizeof(server_addr)) < 0) {
            throw std::runtime_error("Connection failed");
        }

        // Send the vote
        if( send(sock , vote.c_str() , strlen(vote.c_str()) , 0) < 0) {
            throw std::runtime_error("Send failed");
        }

        // TODO: Add any response handling if necessary

    } catch (const std::runtime_error& e) {
        std::cerr << "Exception: " << e.what() << '\n';
    }

    if (sock != -1) {
        close(sock);
    }
}

int main(int argc , char *argv[]) {
    if(argc != 4) {
        std::cerr << "Usage: " << argv[0] << " [serverName] [portNum] [inputFile.txt]\n";
        return 1;
    }

    std::string serverName = argv[1];
    int portNum = std::stoi(argv[2]);
    std::string inputFilePath = argv[3];
    std::ifstream inputFile(inputFilePath);
    std::string line;

    while(std::getline(inputFile, line)) {
        // This assumes that each line in the file is a vote to be sent.
        // Adjust as necessary for your use case.
        sendVote(serverName, portNum, line);
    }

    return 0;
}
