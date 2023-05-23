#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>

#define MAX_BUFFER_SIZE 1024
#define MAX_PARTY_NAME_SIZE 256
#define MAX_NAME_SIZE 256
#define MAX_QUEUE_SIZE 50

typedef struct {
    char voter_name[MAX_NAME_SIZE];
    char party_name[MAX_PARTY_NAME_SIZE];
} Vote;

typedef struct {
    int sock;
    struct sockaddr_in addr;
} Connection;

typedef struct {
    Vote votes[MAX_QUEUE_SIZE];
    int front;
    int rear;
    int size;
    pthread_mutex_t mutex;
    pthread_cond_t cond;
} VoteQueue;

int listenfd;
pthread_t *worker_threads;
int num_worker_threads;
int stop = 0;

VoteQueue vote_queue;

void write_poll_stats();

void signal_handler(int sig) {
    printf("Received signal: %d\n", sig);

    if (sig == SIGINT) {
        stop = 1;
        shutdown(listenfd, SHUT_RDWR);
        close(listenfd);

        for (int i = 0; i < num_worker_threads; i++) {
            pthread_cancel(worker_threads[i]);
            pthread_join(worker_threads[i], NULL);
        }

        pthread_mutex_lock(&vote_queue.mutex);
        write_poll_stats();
        pthread_mutex_unlock(&vote_queue.mutex);

        exit(0);
    }
}

void* worker(void* arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    char buffer[MAX_BUFFER_SIZE];

    while (1) {
        pthread_mutex_lock(&vote_queue.mutex);
        while (vote_queue.size == 0)
            pthread_cond_wait(&vote_queue.cond, &vote_queue.mutex);

        Vote vote = vote_queue.votes[vote_queue.front];
        vote_queue.front = (vote_queue.front + 1) % MAX_QUEUE_SIZE;
        vote_queue.size--;

        pthread_mutex_unlock(&vote_queue.mutex);

        Connection conn = *(Connection*)arg;
        int sock = conn.sock;

        ssize_t n = write(sock, "SEND NAME PLEASE\n", 17);
        if (n == -1) {
            perror("write");
            return NULL;
        }

        n = read(sock, buffer, sizeof(buffer));
        if (buffer[n-1] == '\n') {
            buffer[n-1] = '\0';
            n--;
        }

        printf("Raw bytes: ");
        for (ssize_t i = 0; i < n; i++) {
            printf("%c", buffer[i]);
        }
        printf("\n");
        printf("Received: %s\n", buffer);

        if (n == -1) {
            perror("read");
            return NULL;
        }

        strncpy(vote.voter_name, buffer, n);
        vote.voter_name[n] = '\0';

        printf("Voter name: %s\n", vote.voter_name);

        memset(buffer, 0, sizeof(buffer));

        pthread_mutex_lock(&vote_queue.mutex);
        int already_voted = 0;
        for (int i = 0; i < vote_queue.size; i++) {
            if (strcmp(vote.voter_name, vote_queue.votes[(vote_queue.front + i) % MAX_QUEUE_SIZE].voter_name) == 0) {
                already_voted = 1;
                break;
            }
        }

        if (already_voted) {
            write(sock, "ALREADY VOTED\n", 14);
            close(sock);
            pthread_mutex_unlock(&vote_queue.mutex);
            continue;
        }

        ssize_t m = write(sock, "SEND VOTE PLEASE\n", 17);
        if (m == -1) {
            perror("write");
            return NULL;
        }

        m = read(sock, buffer, sizeof(buffer));
        if (buffer[m-1] == '\n') {
            buffer[m-1] = '\0';
            m--;
        }
        printf("Received: %s\n", buffer);

        if (m == -1) {
            perror("read");
            return NULL;
        }

        strncpy(vote.party_name, buffer, m);
        vote.party_name[m] = '\0';

        memset(buffer, 0, sizeof(buffer));

        printf("Voter %s voted for %s\n", vote.voter_name, vote.party_name);

        vote_queue.votes[vote_queue.rear] = vote;
        vote_queue.rear = (vote_queue.rear + 1) % MAX_QUEUE_SIZE;
        vote_queue.size++;

        snprintf(buffer, sizeof(buffer), "VOTE for Party %s RECORDED\n", vote.party_name);
        n = write(sock, buffer, strlen(buffer));
        if (n == -1) {
            perror("write");
            return NULL;
        }

        pthread_mutex_unlock(&vote_queue.mutex);
        close(sock);
    }

    return NULL;
}

void write_poll_stats() {
    int total_votes = 0;

    FILE* pollStats = fopen("pollStats.txt", "w");
    if (pollStats == NULL) {
        perror("Failed to open stats file");
        return;
    }

    for (int i = 0; i < vote_queue.size; i++) {
        Vote vote = vote_queue.votes[(vote_queue.front + i) % MAX_QUEUE_SIZE];
        fprintf(pollStats, "%s %s\n", vote.voter_name, vote.party_name);
    }

    fprintf(pollStats, "TOTAL %d\n", vote_queue.size);
    fflush(pollStats);

    fclose(pollStats);
}

void* worker(void* arg) {
    pthread_setcancelstate(PTHREAD_CANCEL_ENABLE, NULL);
    pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS, NULL);

    char buffer[MAX_BUFFER_SIZE];

    while (1) {
        pthread_mutex_lock(&vote_queue.mutex);
        while (vote_queue.size == 0)
            pthread_cond_wait(&vote_queue.cond, &vote_queue.mutex);

        Connection conn = *(Connection*)arg;
        int sock = conn.sock;

        Vote vote;
        strncpy(vote.voter_name, buffer, MAX_NAME_SIZE);
        vote.voter_name[MAX_NAME_SIZE - 1] = '\0';

        ssize_t m = write(sock, "SEND VOTE PLEASE\n", 17);
        if (m == -1) {
            perror("write");
            return NULL;
        }

        m = read(sock, buffer, sizeof(buffer));
        if (buffer[m-1] == '\n') {
            buffer[m-1] = '\0';
            m--;
        }
        printf("Received: %s\n", buffer);

        if (m == -1) {
            perror("read");
            return NULL;
        }

        strncpy(vote.party_name, buffer, MAX_PARTY_NAME_SIZE);
        vote.party_name[MAX_PARTY_NAME_SIZE - 1] = '\0';

        memset(buffer, 0, sizeof(buffer));

        printf("Voter %s voted for %s\n", vote.voter_name, vote.party_name);

        vote_queue.votes[vote_queue.rear] = vote;
        vote_queue.rear = (vote_queue.rear + 1) % MAX_QUEUE_SIZE;
        vote_queue.size++;

        snprintf(buffer, sizeof(buffer), "VOTE for Party %s RECORDED\n", vote.party_name);
        ssize_t n = write(sock, buffer, strlen(buffer));
        if (n == -1) {
            perror("write");
            return NULL;
        }

        pthread_mutex_unlock(&vote_queue.mutex);
        close(sock);
    }

    return NULL;
}

int main(int argc, char* argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s portnum num_worker_threads bufferSize pollLogPath pollStatsPath\n", argv[0]);
        return 1;
    }

    signal(SIGINT, signal_handler);

    int portnum = atoi(argv[1]);
    num_worker_threads = atoi(argv[2]);
    int bufferSize = atoi(argv[3]);
    char* pollLogPath = argv[4];
    char* pollStatsPath = argv[5];

    FILE* pollLog = fopen(pollLogPath, "w");
    if (pollLog == NULL) {
        perror("Failed to open log file");
        return 1;
    }

    vote_queue.front = 0;
    vote_queue.rear = 0;
    vote_queue.size = 0;
    pthread_mutex_init(&vote_queue.mutex, NULL);
    pthread_cond_init(&vote_queue.cond, NULL);

    worker_threads = (pthread_t*)malloc(num_worker_threads * sizeof(pthread_t));
    for (int i = 0; i < num_worker_threads; i++) {
        Connection* conn = malloc(sizeof(Connection));
        conn->sock = -1;
        pthread_create(&worker_threads[i], NULL, worker, conn);
    }

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

    if (listen(listenfd, MAX_QUEUE_SIZE) == -1) {
        perror("listen");
        return 1;
    }

    while (!stop) {
        Connection conn;
        socklen_t addrlen = sizeof(conn.addr);

        conn.sock = accept(listenfd, (struct sockaddr*)&conn.addr, &addrlen);
        if (conn.sock == -1) {
            perror("accept");
            return 1;
        }

        pthread_mutex_lock(&vote_queue.mutex);
        while (vote_queue.size == bufferSize)
            pthread_cond_wait(&vote_queue.cond, &vote_queue.mutex);

        vote_queue.votes[vote_queue.rear] = vote;
        vote_queue.rear = (vote_queue.rear + 1) % MAX_QUEUE_SIZE;
        vote_queue.size++;

        pthread_mutex_unlock(&vote_queue.mutex);
        pthread_cond_signal(&vote_queue.cond);
    }

    return 0;
}
