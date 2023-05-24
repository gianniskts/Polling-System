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

// Function to send a vote to the server
void sendVote(const string &serverName, int portNum, const string &name_vote) {
    // Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("Error creating socket");
        return;
    }

    // Specify server address
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(portNum);
    if (inet_pton(AF_INET, serverName.c_str(), &serv_addr.sin_addr) <= 0) {
        perror("Error on inet_pton");
        close(sock);
        return;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Error connecting to server");
        close(sock);
        return;
    }

    // Send vote to the server
    if (send(sock, name_vote.c_str(), name_vote.size(), 0) < 0) {
        perror("Error sending vote");
    }

    // Read confirmation from server
    char buffer[256];
    memset(buffer, 0, sizeof(buffer));
    if (read(sock, buffer, sizeof(buffer) - 1) < 0) {
        perror("Error reading confirmation from server");
    } else {
        cout << "Server confirmation: " << buffer << "\n";
    }

    // Close the socket
    close(sock);
}

int main(int argc, char *argv[]) {
    // Check command line arguments
    if (argc != 4) {
        cerr << "Usage: " << argv[0] << " [serverName] [portNum] [inputFile.txt]\n";
        exit(1);
    }

    // Parse command line arguments
    string serverName = argv[1];
    int portNum = atoi(argv[2]);
    string inputFile = argv[3];

    // Open input file
    ifstream inFile("../data/" + inputFile);
    if (!inFile) {
        cerr << "Unable to open input file: " << inputFile << "\n";
        exit(1);
    }

    // For each line in the input file, create a new thread to send the vote
    string line;
    vector<thread> workers;
    while (getline(inFile, line)) {
        workers.push_back(thread(sendVote, serverName, portNum, line));

        // Wait for current thread to complete before moving to the next one.
        // Note: This effectively makes the client single-threaded because it
        // waits for each thread to finish before starting the next one.
        workers.back().join();
    }

    return 0;
}
