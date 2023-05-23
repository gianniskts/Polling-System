#include <iostream>
#include <unordered_map>
#include <queue>
#include <pthread.h>
#include <cstring>
#include <unistd.h>
#include <netinet/in.h>
#include <signal.h>
#include <fstream>
#include <errno.h>

using namespace std;

std::unordered_map<std::string, std::string> votes;
std::unordered_map<std::string, int> party_votes;
std::ofstream pollLog;
std::ofstream pollStats;

int listenfd;
pthread_t *worker_threads;
int num_worker_threads;
int stop = 0;

pthread_mutex_t conn_queue_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t conn_queue_cond = PTHREAD_COND_INITIALIZER;
std::queue<int> conn_queue;
int bufferSize;

void write_poll_stats() {
    int total_votes = 0;

    for (auto& party : party_votes) {
        pollStats << party.first << " " << party.second << "\n";
        total_votes += party.second;
    }

    pollStats << "TOTAL " << total_votes << "\n";
    pollStats.flush();  // Explicitly flush the output
}

void signal_handler(int sig) {
    cout << "Received signal: " << sig << endl;  // Debugging message

    if (sig == SIGINT) {
        stop = 1;
        shutdown(listenfd, SHUT_RDWR);
        close(listenfd);

        for (int i = 0; i < num_worker_threads; i++) {
            pthread_cancel(worker_threads[i]);
            pthread_join(worker_threads[i], NULL);
        }

        pthread_mutex_lock(&conn_queue_mutex);
        write_poll_stats();
        pthread_mutex_unlock(&conn_queue_mutex);

        pollLog.close();
        pollStats.close();

        exit(0);
    }
}

void* worker(void* arg) {
    // pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    // pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    char buffer[1024];

    while (1) {
        pthread_mutex_lock(&conn_queue_mutex);
        while (conn_queue.empty())
            pthread_cond_wait(&conn_queue_cond, &conn_queue_mutex);
        int sock = conn_queue.front();
        conn_queue.pop();
        pthread_mutex_unlock(&conn_queue_mutex);

        ssize_t n = write(sock, "SEND NAME PLEASE\n", 17);
        if (n == -1) {
            perror("write");
            return NULL;
        }

        n = read(sock, buffer, sizeof(buffer));
        // if buffer has a newline, remove it
        if (buffer[n-1] == '\n') {
            buffer[n-1] = '\0';
            n--;
        }
        // Print raw bytes read from the socket
        cout << "Raw bytes: ";
        for (ssize_t i = 0; i < n; i++) {
            cout << buffer[i];
        }
        cout << endl;
        cout << "Received: " << buffer << endl;  // Debugging message
        if (n == -1) {
            perror("read");
            return NULL;
        }
        // string voter_name(buffer);
        // create a char* voter_name in c
        // copy buffer into voter_name
        char voter_name[n+1];
        strncpy(voter_name, buffer, n);
        cout << "Voter name: " << voter_name << endl;  // Debugging message
        memset(buffer, 0, sizeof(buffer));  // Clear the buffer with null characters


        pthread_mutex_lock(&conn_queue_mutex);
        if (votes.find(voter_name) != votes.end()) {
            write(sock, "ALREADY VOTED\n", 14);
            close(sock);
            pthread_mutex_unlock(&conn_queue_mutex);
            continue;
        }

        ssize_t m = write(sock, "SEND VOTE PLEASE\n", 17);
        if (m == -1) {
            perror("write");
            return NULL;
        }

        m = read(sock, buffer, sizeof(buffer));
        // if buffer has a newline, remove it
        if (buffer[m-1] == '\n') {
            buffer[m-1] = '\0';
            m--;
        }
        cout << "Received: " << buffer << endl;  // Debugging message

        if (m == -1) {
            perror("read");
            return NULL;
        }
        std::string party_name(buffer, m);  // Use n to ensure correct string
        cout << "Party name: " << party_name << endl;  // Debugging message
        memset(buffer, 0, sizeof(buffer));  // Clear the buffer with null characters
        cout << "Voter " << voter_name << " voted for " << party_name << endl;  // Debugging message

        votes[voter_name] = party_name;
        party_votes[party_name]++;
        cout << voter_name << ' ' << party_name << "\n";
        pollLog << voter_name << ' ' << party_name << "\n";
        pollLog.flush();  // Explicitly flush the output

        if (pollLog.fail()) {
            cerr << "Failed to write to log file" << endl;
            return NULL;
        }

        snprintf(buffer, sizeof(buffer), "VOTE for Party %s RECORDED\n", party_name.c_str());
        n = write(sock, buffer, strlen(buffer));
        if (n == -1) {
            perror("write");
            return NULL;
        }
        pthread_mutex_unlock(&conn_queue_mutex);
        close(sock);
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        cerr << "Usage: " << argv[0] << " portnum num_worker_threads bufferSize pollLogPath pollStatsPath" << endl;
        return 1;
    }

    signal(SIGINT, signal_handler);

    int portnum = atoi(argv[1]);
    num_worker_threads = atoi(argv[2]);
    bufferSize = atoi(argv[3]);
    char* pollLogPath = argv[4];
    char* pollStatsPath = argv[5];

    pollLog.open(pollLogPath);
    if (!pollLog.is_open()) {
        perror("Failed to open log file"); // Print detailed error message
        return 1;
    }

    pollStats.open(pollStatsPath);
    if (!pollStats.is_open()) {
        perror("Failed to open stats file"); // Print detailed error message
        return 1;
    }

    worker_threads = (pthread_t*)malloc(num_worker_threads * sizeof(pthread_t));
    for(int i = 0; i < num_worker_threads; i++)
        pthread_create(&worker_threads[i], NULL, worker, NULL);

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    if (listenfd == -1) {
        perror("socket");
        return 1;
    }

    struct sockaddr_in servaddr = {0};
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(portnum);

    if (bind(listenfd, (struct sockaddr*)&servaddr, sizeof(servaddr)) == -1) {
        perror("bind");
        return 1;
    }

    if (listen(listenfd, 50) == -1) {
        perror("listen");
        return 1;
    }

    while (!stop) {
        int connfd = accept(listenfd, (struct sockaddr*)NULL, NULL);
        if (connfd == -1) {
            perror("accept");
            return 1;
        }

        pthread_mutex_lock(&conn_queue_mutex);
        while ((int)conn_queue.size() == bufferSize)
            pthread_cond_wait(&conn_queue_cond, &conn_queue_mutex);
        conn_queue.push(connfd);
        pthread_cond_signal(&conn_queue_cond);
        pthread_mutex_unlock(&conn_queue_mutex);
    }

    return 0;
}