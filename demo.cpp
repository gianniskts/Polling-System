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

void handle_sigint(int sig) {
    ofstream poll_stats;
    poll_stats.open(poll_stats_file);
    if (!poll_stats) {
        cerr << "Unable to open file: " << poll_stats_file << "\n";
        exit(1);
    }
    int total_votes = 0;
    for (const auto& pair : partyVotes) {
        poll_stats << pair.first << " " << pair.second << "\n";
        total_votes += pair.second;
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
        string request = "Please Enter Your Name: ";
        write(conn_fd, request.c_str(), request.size());

        string name;
        if (read(conn_fd, buffer, 255) < 0)
            continue;
        name = trim(string(buffer));
        memset(buffer, 0, 256);
        string response;
        {
            unique_lock<mutex> lock(mtx);
            if (voterRecords.find(name) != voterRecords.end()) {
                response = "Already Voted\n";
                write(conn_fd, response.c_str(), response.size());
                close(conn_fd);
                continue;
            }
            else {
                response = "Please Enter Your Vote: ";
                write(conn_fd, response.c_str(), response.size());
            }
        }
        string party;
        if (read(conn_fd, buffer, 255) < 0)
            continue;
        party = trim(string(buffer));
        memset(buffer, 0, 256);
        {
            unique_lock<mutex> lock(mtx);
            voterRecords[name] = party;
            partyVotes[party]++;
        }
        ofstream poll_log;
        poll_log.open(poll_log_file, ios_base::app);
        if (!poll_log) {
            cerr << "Unable to Open File: " << poll_log_file << "\n";
            exit(1);
        }
        poll_log << name << " " << party << "\n";
        poll_log.close();
        response = "\nVote for Party " + party + " recorded\n";
        write(conn_fd, response.c_str(), response.size());
        close(conn_fd);
    }
}



int main(int argc, char *argv[]) {
    if (argc != 6) {
        cerr << "Usage: " << argv[0] << " [portnum] [numWorkerthreads] [bufferSize] [poll-log] [poll-stats]\n";
        exit(1);
    }
    int port_num = atoi(argv[1]);
    if (port_num <= 0) {
        cerr << "Port number must be greater than 0.\n";
        exit(1);
    }
    int num_worker_threads = atoi(argv[2]);
    if (num_worker_threads <= 0) {
        cerr << "Number of worker threads must be greater than 0.\n";
        exit(1);
    }
    int buffer_size = atoi(argv[3]);
    if (buffer_size <= 0) {
        cerr << "Buffer size must be greater than 0.\n";
        exit(1);
    }
    string poll_log_file = argv[4];
    poll_stats_file = argv[5];

    signal(SIGINT, handle_sigint);

    cout << "Starting polling server...\n";
    cout << "Port number: " << port_num << "\n";
    cout << "Number of worker threads: " << num_worker_threads << "\n";
    cout << "Buffer size: " << buffer_size << "\n";
    cout << "Poll log file: " << poll_log_file << "\n";
    cout << "Poll stats file: " << poll_stats_file << "\n";
    cout << "Press Ctrl+C twice to exit.\n";
    cout << "Waiting for connections...\n";
    cout << "----------------------------------------\n";

    vector<thread> workers;
    for (int i = 0; i < num_worker_threads; i++)
        workers.push_back(thread(workerThread, poll_log_file));

    int server_fd, new_socket;
    sockaddr_in address;
    int addrlen = sizeof(address);

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port_num);
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    if (listen(server_fd, 3) < 0) {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    while (1) {
        int conn_fd;
        if ((conn_fd = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept failed");
            continue;
        }
        {
            unique_lock<mutex> lock(mtx);
            while (connectionBuffer.size() >= buffer_size)
                cond_var.wait(lock);
            connectionBuffer.push_back(conn_fd);
            cond_var.notify_one();
        }
    }

    close(server_fd);

    for (auto &worker : workers)
        worker.join();

    return 0;
}
