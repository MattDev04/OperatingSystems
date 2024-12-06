#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <semaphore.h>
#include <random>

int getRandomNumber(int min, int max) {
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(min, max);
    return dis(gen);
}

#define NUM_QUEUES 4
#define N 10

struct Queues {
    int queues[NUM_QUEUES][N];
    int sizes[NUM_QUEUES];
};

struct Semaphores {
    sem_t full[NUM_QUEUES];
    sem_t empty[NUM_QUEUES];
    sem_t mutex[NUM_QUEUES];
};

struct Queues* getQueues() {
    static int shmid = 0;
    if (shmid == 0) {
        shmid = shmget(IPC_PRIVATE, sizeof(struct Queues), IPC_CREAT | 0666);
        if (shmid == -1) {
            perror("shmget Queues");
            exit(1);
        }
    }
    return (struct Queues*)shmat(shmid, NULL, 0);
}

struct Semaphores* getSemaphores() {
    static int shmid = 0;
    if (shmid == 0) {
        shmid = shmget(IPC_PRIVATE, sizeof(struct Semaphores), IPC_CREAT | 0666);
        if (shmid == -1) {
            perror("shmget semaphores");
            exit(1);
        }
    }
    return (struct Semaphores*)shmat(shmid, NULL, 0);
}

void initSystem() {
    struct Queues* Queues = getQueues();
    struct Semaphores* semaphores = getSemaphores();
    for (int i = 0; i < NUM_QUEUES; i++) {
        Queues->sizes[i] = 0;
        sem_init(&semaphores->full[i], 1, 0);
        sem_init(&semaphores->empty[i], 1, N);
        sem_init(&semaphores->mutex[i], 1, 1);
    }
}

void producer(int producerId, int queueIndex) {
    struct Queues* Queues = getQueues();
    struct Semaphores* semaphores = getSemaphores();
    while (1) {
        sem_wait(&semaphores->empty[queueIndex]);
        sem_wait(&semaphores->mutex[queueIndex]);
        int data = getRandomNumber((queueIndex + 1) * 10, (queueIndex + 2) * 10);
        Queues->queues[queueIndex][Queues->sizes[queueIndex]] = data;
        Queues->sizes[queueIndex]++;
        printf("Producer %d added '%d' to queue %d\n", producerId, data, queueIndex + 1);
        sem_post(&semaphores->mutex[queueIndex]);
        sem_post(&semaphores->full[queueIndex]);
        usleep(500000);
    }
}

void superProducer() {
    struct Queues* Queues = getQueues();
    struct Semaphores* semaphores = getSemaphores();
    while (1) {
        for (int i = 0; i < NUM_QUEUES; i++) {
            sem_wait(&semaphores->empty[i]);
            sem_wait(&semaphores->mutex[i]);
            int data = getRandomNumber((i + 1) * 10, (i + 2) * 10);
            Queues->queues[i][Queues->sizes[i]] = data;
            Queues->sizes[i]++;
            printf("SuperProducer added '%d' to queue %d\n", data, i + 1);
            sem_post(&semaphores->mutex[i]);
            sem_post(&semaphores->full[i]);
        }
        usleep(1000000);
    }
}

void consumer(int consumerId, int queueIndex) {
    struct Queues* Queues = getQueues();
    struct Semaphores* semaphores = getSemaphores();
    while (1) {
        sem_wait(&semaphores->full[queueIndex]);
        sem_wait(&semaphores->mutex[queueIndex]);
        int data = Queues->queues[queueIndex][0];
        for (int i = 0; i < Queues->sizes[queueIndex] - 1; i++) {
            Queues->queues[queueIndex][i] = Queues->queues[queueIndex][i + 1];
        }
        Queues->sizes[queueIndex]--;
        printf("Consumer %d took '%d' from queue %d\n", consumerId, data, queueIndex + 1);
        sem_post(&semaphores->mutex[queueIndex]);
        sem_post(&semaphores->empty[queueIndex]);
        usleep(700000);
    }
}

void createProducer(int producerId, int queueIndex) {
    if (fork() == 0) {
        producer(producerId, queueIndex);
        exit(0);
    }
}

void createConsumer(int consumerId, int queueIndex) {
    if (fork() == 0) {
        consumer(consumerId, queueIndex);
        exit(0);
    }
}

int main() {
    initSystem();
    createProducer(1, 0);
    createProducer(2, 1);
    for (int i = 0; i < NUM_QUEUES; i++) {
        createConsumer(i + 1, i);
    }
    superProducer();
    return 0;
}
