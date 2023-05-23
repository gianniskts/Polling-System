
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>

// Define the mutex and condition variables
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;

// Shared buffer to store client sockets
int *buffer;
int bufferIndex = 0;

// Array to store client votes
char **votes;
char **names;
int *voteCounts;

FILE *pollLog, *pollStats;

int numParties = 0;

void shutdown_handler(int sig) {
    for(int i = 0; i < numParties; i++) {
        fprintf(pollStats, "%s %d\n", names[i], voteCounts[i]);
    }
    fclose(pollLog);
    fclose(pollStats);
    exit(0);
}

void* handle_client(void* arg) {
    int client_sock = *(int*)arg;
    char name[256] = {0};
    char party[256] = {0};

    // Ask for the client's name
    write(client_sock, "SEND NAME PLEASE", 16);
    for (int i=0; i<256; i++) {
        printf("%c", name[i]);
    }
    // Read the client's name
    ssize_t len = read(client_sock, name, sizeof(name) - 1);
    // name[strcspn(name, "\n")] = 0; // Replace newline at any position
    size_t lenn = strlen(name);
    
    printf("Size of name: %ld\n", len);
    printf("Name: %s", name);

    // Lock the mutex before checking if the client has already voted
    pthread_mutex_lock(&mutex);

    int found = 0;
    for(int i = 0; i < numParties; i++) {
        if(strcmp(names[i], name) == 0) {
            found = 1;
            break;
        }
    }

    if(found) {
        write(client_sock, "ALREADY VOTED", 13);
    } else {
        // Ask for the client's vote
        write(client_sock, "SEND VOTE PLEASE", 16);
        // Read the client's vote
        len = read(client_sock, party, sizeof(party) - 1);
        // if (len > 0 && party[len - 1] == '\n') party[len - 1] = '\0'; // Remove newline if it's there
        printf("Size of party: %ld\n", len);
        for (ssize_t i=0; i<len; i++) {
            printf("%c", party[i]);
        }

        names[numParties] = strdup(name);
        votes[numParties] = strdup(party);
        voteCounts[numParties] = 1;
        numParties++;

        // Write the vote to the poll log file
        fprintf(pollLog, "%s %s\n", name, party);

        char msg[500];
        snprintf(msg, sizeof(msg), "VOTE for Party %s RECORDED", party);
        write(client_sock, msg, strlen(msg));
    }

    // Unlock the mutex
    pthread_mutex_unlock(&mutex);

    close(client_sock);

    pthread_exit(NULL);
}



void* worker(void* arg) {
    while (1) {
        int client_sock;

        // Lock the mutex before checking the buffer
        pthread_mutex_lock(&mutex);

        while (bufferIndex == 0) {
            // Wait until the buffer is not empty
            pthread_cond_wait(&cond, &mutex);
        }

        // Remove a client socket from the buffer
        client_sock = buffer[--bufferIndex];

        // Unlock the mutex
        pthread_mutex_unlock(&mutex);

        // Handle the client
        handle_client(&client_sock);
    }

    pthread_exit(NULL);
}

int main(int argc, char *argv[]) {
    if (argc != 6) {
        printf("Usage: ./poller [portnum] [numWorkerthreads] [bufferSize] [poll-log] [poll-stats]\n");
        exit(1);
    }

    signal(SIGINT, shutdown_handler);

    int portNum = atoi(argv[1]);
    int numWorkerThreads = atoi(argv[2]);
    int bufferSize = atoi(argv[3]);

    buffer = (int*) malloc(bufferSize * sizeof(int));
    votes = (char**) malloc(bufferSize * sizeof(char*));
    names = (char**) malloc(bufferSize * sizeof(char*));
    voteCounts = (int*) malloc(bufferSize * sizeof(int));

    for (int i = 0; i < bufferSize; i++) {
        votes[i] = (char*) malloc(256 * sizeof(char));
        names[i] = (char*) malloc(256 * sizeof(char));
    }

    pollLog = fopen(argv[4], "w");
    pollStats = fopen(argv[5], "w");

    pthread_t *workers = (pthread_t*) malloc(numWorkerThreads * sizeof(pthread_t));

    // Start the worker threads
    for (int i = 0; i < numWorkerThreads; i++) {
        pthread_create(&workers[i], NULL, worker, NULL);
    }

    // Create the socket
    int server_sock = socket(AF_INET, SOCK_STREAM, 0);

    // Define the server address
    struct sockaddr_in server_addr;
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(portNum);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    // Bind the socket
    bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr));

    // Start listening
    listen(server_sock, 10);

    printf("Server listening on port %d...\n", portNum);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        // Accept a new client
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &client_len);

        // Lock the mutex before adding the client socket to the buffer
        pthread_mutex_lock(&mutex);

        // Add the client socket to the buffer
        buffer[bufferIndex++] = client_sock;

        // Signal a worker thread that the buffer is not empty
        pthread_cond_signal(&cond);

        // Unlock the mutex
        pthread_mutex_unlock(&mutex);
    }

    return 0;
}
