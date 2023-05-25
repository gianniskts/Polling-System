#include <iostream>
#include <fstream>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <map>
#include <string>
#include <signal.h>
#include <stdlib.h>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

using namespace std;

map<string, int> partyVotes;
map<string, string> voterRecords;
vector<int> connectionBuffer;
mutex mtx;
condition_variable cond_var;
string poll_stats_file;

void handle_sigint(int sig) { // handle Ctrl+C
    ofstream poll_stats; // write poll stats to file
    poll_stats.open("../results/" + poll_stats_file); // open poll stats file
    if (!poll_stats) {
        cerr << "Unable to open file: " << poll_stats_file << "\n";
        exit(-6);
    }
    int total_votes = 0; // total number of votes
    for (const auto& pair : partyVotes) { // iterate over partyVotes map
        poll_stats << pair.first << " " << pair.second << "\n"; // write party and number of votes to file
        total_votes += pair.second; // increment total number of votes
    }
    poll_stats << "TOTAL " << total_votes << "\n";
    poll_stats.close();
    exit(0);
}


string trim(const string& str) {
    size_t first = str.find_first_not_of("\n\r");
    if (string::npos == first)
        return str;
    size_t last = str.find_last_not_of("\n\r");
    return str.substr(first, (last - first + 1));
}

void workerThread(string poll_log_file) {
    char buffer[256];
    while (true) {
        int conn_fd;
        {
            unique_lock<mutex> lock(mtx);
            while (connectionBuffer.empty())
                cond_var.wait(lock);
            conn_fd = connectionBuffer.back();
            connectionBuffer.pop_back();
        }
        cout << "Connection accepted\n";
        cout << "----------------------------------------\n";
        memset(buffer, 0, 256);
        string request = "Please Enter Your Name followed by Your Vote (separated by a space): ";
        write(conn_fd, request.c_str(), request.size());

        if (read(conn_fd, buffer, 255) < 0)
            continue;
        string name_vote = trim(string(buffer));
        size_t pos = name_vote.find(' ');
        if (pos == string::npos)
            continue;
        string name = name_vote.substr(0, pos);
        string party = name_vote.substr(pos + 1);
        memset(buffer, 0, 256);
        string response;
        {
            unique_lock<mutex> lock(mtx);
            if (voterRecords.find(name) != voterRecords.end()) {
                response = "Already Voted\n";
            }
            else {
                voterRecords[name] = party;
                partyVotes[party]++;
                response = "\nVote for Party " + party + " recorded\n";
            }
        }
        ofstream poll_log;
        poll_log.open("../logs/" + poll_log_file, ios_base::app);

        if (!poll_log) {
            cerr << "Unable to Open File: " << poll_log_file << "\n";
            exit(1);
        }
        poll_log << name << " " << party << "\n";
        poll_log.close();
        write(conn_fd, response.c_str(), response.size());
        close(conn_fd);
    }
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        cerr << "Usage: " << argv[0] << " [portnum] [numWorkerthreads] [bufferSize] [poll-log] [poll-stats]" << endl;
        exit(-1);
    }
    int port_num = atoi(argv[1]);
    if (port_num <= 0) {
        cerr << "Port number must be greater than 0." << endl;
        exit(-2);
    }
    int num_worker_threads = atoi(argv[2]);
    if (num_worker_threads <= 0) {
        cerr << "Number of worker threads must be greater than 0." << endl;
        exit(-3);
    }
    int buffer_size = atoi(argv[3]);
    if (buffer_size <= 0) {
        cerr << "Buffer size must be greater than 0." << endl;
        exit(-4);
    }
    string poll_log_file = argv[4];
    poll_stats_file = argv[5];

    signal(SIGINT, handle_sigint); // stop when Ctrl+C is pressed(Needs double Ctrl+C)

    cout << "Starting polling server...\n";
    cout << "Port number: " << port_num << "\n";
    cout << "Number of worker threads: " << num_worker_threads << "\n";
    cout << "Buffer size: " << buffer_size << "\n";
    cout << "Poll log file: " << poll_log_file << "\n";
    cout << "Poll stats file: " << poll_stats_file << "\n";
    cout << "Press Ctrl+C twice to exit.\n";
    cout << "Waiting for connections...\n";
    cout << "----------------------------------------\n";

    vector<thread> workers; // worker threads to handle connections concurrently 
    for (int i = 0; i < num_worker_threads; i++)
        workers.push_back(thread(workerThread, poll_log_file)); // create worker threads and push them to the vector of threads

    int server_fd, new_socket; // server_fd: socket file descriptor, new_socket: new socket file descriptor for each connection
    sockaddr_in address; // address of the server
    int addrlen = sizeof(address); // length of the address, required by accept

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) { // create socket 
        perror("socket failed");
        exit(-5);
    }
    address.sin_family = AF_INET; // IPv4 address family, required by bind
    address.sin_addr.s_addr = INADDR_ANY; // localhost, required by bind
    address.sin_port = htons(port_num); // port number, required by bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) { // bind socket to the address
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 4) < 0) { // listen for connections, 4 is the maximum number of connections in the queue
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    while (true) {
        int conn_fd; // socket file descriptor for each connection
        if ((conn_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) { // accept connection, blocks until a connection is established
            perror("accept failed");
            continue;
        }
        {
            unique_lock<mutex> lock(mtx); // lock the mutex, so that only one thread can access the buffer at a time
            while (connectionBuffer.size() >= buffer_size) // wait until the buffer is not full
                cond_var.wait(lock); // wait until notified
            connectionBuffer.push_back(conn_fd); // push the connection to the buffer
            cond_var.notify_one(); // notify one thread
        }
    }

    close(server_fd); // close the socket

    for (auto &worker : workers) // wait for all worker threads to finish
        worker.join(); // join the thread

    return 0;
}