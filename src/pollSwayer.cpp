#include <iostream>
#include <fstream> 
#include <thread> 
#include <vector>
#include <sys/socket.h> // used for socket
#include <netinet/in.h> // used for sockaddr_in
#include <arpa/inet.h>  // used for inet_pton
#include <unistd.h>     // used for close
#include <cstring>      // used for memset

using namespace std;

// Function to send a vote to the server
void sendVote(const string &serverName, int portNum, const string &name_vote) {
    // Create a socket
    int sock = socket(AF_INET, SOCK_STREAM, 0); // AF_INET: IPv4, SOCK_STREAM: TCP
    if (sock < 0) {
        perror("Error creating socket");
        return;
    }

    // Specify server address
    struct sockaddr_in serv_addr; // IPv4 address
    serv_addr.sin_family = AF_INET; // IPv4 address family
    serv_addr.sin_port = htons(portNum); // port number
    if (inet_pton(AF_INET, serverName.c_str(), &serv_addr.sin_addr) <= 0) { // convert IPv4 address from text to binary form
        perror("Error on inet_pton");
        close(sock);
        return;
    }

    // Connect to the server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) { // connect to the server
        perror("Error connecting to server");
        close(sock);
        return;
    }

    // Send vote to the server
    if (send(sock, name_vote.c_str(), name_vote.size(), 0) < 0) { // send vote to the server
        perror("Error sending vote");
    }

    // Read confirmation from server
    char buffer[256]; // buffer to store server response
    memset(buffer, 0, sizeof(buffer)); // clear buffer
    if (read(sock, buffer, sizeof(buffer) - 1) < 0) { // read confirmation from server
        perror("Error reading confirmation from server");
    } else {
        cout << "Server confirmed" << endl;
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
    vector<thread> workers; // Vector to store threads
    while (getline(inFile, line)) { // Read each line from the input file
        workers.push_back(thread(sendVote, serverName, portNum, line)); // Create a new thread to send the vote

        workers.back().join(); // Wait for the thread to finish
    }

    return 0;
}
