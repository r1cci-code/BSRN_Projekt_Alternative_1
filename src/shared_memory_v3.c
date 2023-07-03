#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <limits.h>

#define USEC_PER_SEC 1000000
#define NUM_SAMPLES_PER_SEC 1000

volatile sig_atomic_t flag = 0;

void handler(int sig) {
    flag = 1;
}

pid_t p1, p2, p3, p4;
FILE* logFile = NULL;

typedef struct {
    int num;
    int count;
    int sum;
    int mean;
} SharedData;

#define SHM_KEY 12345
#define SEM_KEY 54321

int shmid;
SharedData* shm_data;

int semid;

void cleanup() {
    wait(NULL); // Warten auf die Beendigung der Kindprozesse
    shmdt(shm_data);
    shmctl(shmid, IPC_RMID, NULL);
    semctl(semid, 0, IPC_RMID);
}

void P() {
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = -1;
    sb.sem_flg = 0;
    if (semop(semid, &sb, 1) == -1) {
        perror("semop");
        exit(1);
    }
}

void V() {
    struct sembuf sb;
    sb.sem_num = 0;
    sb.sem_op = 1;
    sb.sem_flg = 0;
    if (semop(semid, &sb, 1) == -1) {
        perror("semop");
        exit(1);
    }
}

int conv() {
    int num = rand() % 256;
    return num;
}

void doConvProcess() {
    while (!flag) {
        int num = conv();
        P();
        shm_data->num = num;
        shm_data->count++;
        shm_data->sum += num;
        V();
        usleep(USEC_PER_SEC / NUM_SAMPLES_PER_SEC);
    }
    exit(0);
}

void doLogProcess() {
    FILE* file = fopen("log.txt", "w");
    if (file == NULL) {
        printf("Error opening file!\n");
        exit(1);
    }
    int num;
    int count = 0;
    while (!flag) {
        P();
        num = shm_data->num;
        V();
        fprintf(file, "Messwert %d: %d\n", count + 1, num);
        count++;
        P();
        shm_data->count++;
        shm_data->sum += num;
        V();
        usleep(USEC_PER_SEC / NUM_SAMPLES_PER_SEC);
    }
    fclose(file);
    exit(0);
}

void doStatProcess() {
    while (!flag) {
        P();
        int num = shm_data->num;
        shm_data->sum += num;
        shm_data->count++;
        if (shm_data->count % 1000 == 0) {
            shm_data->mean = shm_data->sum / shm_data->count;
        }
        V();
    }
    exit(0);
}

void doReportProcess() {
    FILE* file = fopen("log.txt", "r");
    if (file == NULL) {
        printf("Error opening file!\n");
        exit(1);
    }
    int count = 0;
    int sum = 0;
    int min = INT_MAX;
    int max = INT_MIN;
    int num;

    while (fscanf(file, "Messwert %*d: %d\n", &num) == 1) {
        count++;
        sum += num;
        if (num < min)
            min = num;
        if (num > max)
            max = num;
    }
    fclose(file);

    printf("Anzahl der Messwerte: %d\n", count + shm_data->count);
    printf("Summe der Messwerte: %d\n", sum + shm_data->sum);
    printf("Minimum der Messwerte: %d\n", min);
    printf("Maximum der Messwerte: %d\n", max);
    printf("Durchschnitt der Messwerte: %.2f\n", (float)(sum + shm_data->sum) / (count + shm_data->count));
    exit(0);
}

int main() {
    signal(SIGINT, handler);
    shmid = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
    if (shmid == -1) {
        perror("shmget");
        exit(1);
    }
    shm_data = (SharedData*)shmat(shmid, NULL, 0);
    if (shm_data == (void*)-1) {
        perror("shmat");
        exit(1);
    }
    semid = semget(SEM_KEY, 1, IPC_CREAT | 0666);
    if (semid == -1) {
        perror("semget");
        exit(1);
    }
    semctl(semid, 0, SETVAL, 1);

    p1 = fork();
    if (p1 < 0) {
        fprintf(stderr, "fork Failed");
        return 1;
    }
    if (p1 == 0) {
        doConvProcess();
    }

    p2 = fork();
    if (p2 < 0) {
        fprintf(stderr, "fork Failed");
        return 1;
    }
    if (p2 == 0) {
        doLogProcess();
    }

    p3 = fork();
    if (p3 < 0) {
        fprintf(stderr, "fork Failed");
        return 1;
    }
    if (p3 == 0) {
        doStatProcess();
    }

    p4 = fork();
    if (p4 < 0) {
        fprintf(stderr, "fork Failed");
        return 1;
    }
    if (p4 == 0) {
        doReportProcess();
    }

    while (!flag)
        usleep(100000);

    cleanup();
    return 0;
}