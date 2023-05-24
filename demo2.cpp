#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>

using namespace std;

void sendVote(const string &serverName, int portNum, const string &name_vote) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Error creating socket");
        return;
    }

    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portNum);
    if (inet_pton(AF_INET, serverName.c_str(), &serv_addr.sin_addr) <= 0) {
        perror("Error on inet_pton");
        close(sock);
        return;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error connecting to server");
        close(sock);
        return;
    }

    if (send(sock, name_vote.c_str(), name_vote.size(), 0) < 0) {
        perror("Error sending vote");
    }

    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    if (read(sock, buffer, sizeof(buffer) - 1) < 0) {
        perror("Error reading confirmation from server");
    } else {
        cout << "Server confirmation: " << buffer << "\n";
    }

    close(sock);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " [serverName] [portNum] [inputFile.txt]\n";
        exit(1);
    }

    string serverName = argv[1];
    int portNum = atoi(argv[2]);
    string inputFile = argv[3];

    ifstream inFile(inputFile);
    if (!inFile) {
        cerr << "Unable to open input file: " << inputFile << "\n";
        exit(1);
    }

    string line;
    vector<thread> workers;
    while (getline(inFile, line)) {
        workers.push_back(thread(sendVote, serverName, portNum, line));
        // Wait for current thread to complete before moving to the next one.
        workers.back().join();
    }

    return 0;
}
