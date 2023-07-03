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

//zurzeit wird nur alle 1000 Messwerte mit dem Stat Prozess berechnet und ausgegeben.
#define USEC_PER_SEC 1000000
#define NUM_SAMPLES_PER_SEC 1000


volatile sig_atomic_t flag = 0;

void handler(int sig) {
    flag = 1;
}

pid_t p1, p2, p3, p4;
FILE *logFile = NULL;

//SharedData definiert
typedef struct {
    int num;
    int count;
    int sum;
    int mean;
} SharedData;

// shared memory key
#define SHM_KEY 12345
// semaphore key
#define SEM_KEY 54321

int shmid; // shared memory ID
SharedData* shm_data; // pointer to shared memory data structure

int semid; // semaphore ID



// Prozesse ordnungsgemäß  beenden
void cleanup() {
    shmdt(shm_data);
    shmctl(shmid, IPC_RMID, NULL);
    // Remove semaphore
    semctl(semid, 0, IPC_RMID);
}


    // Semaphore operations
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



// 8-bit 5V A/D converter generates 256 different possible values
int conv() {
    int num = rand() % 256; 
    return num;
}

void doConvProcess() {
        // Generate random numbers and store them in shared memory
            while (!flag) {
                int num = conv();
                P();
                shm_data->num = num;
                shm_data->count++;
                shm_data->sum += num;

                V();
                usleep(USEC_PER_SEC / NUM_SAMPLES_PER_SEC);
            }
            break;
        default:
            fprintf(stderr, "Unsupported communication method\n");
            exit(1);
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

            break;
        default:
            fprintf(stderr, "Unsupported communication method\n");
            exit(1);
    }

void doStatProcess() {
            while (!flag) {
                sem_wait(semid);
                int num = shm_data->num;
                shm_data->sum += num;
                shm_data->count++;
                if (shm_data->count % 1000 == 0) {
                    shm_data->mean = shm_data->sum / shm_data->count;
                    int sum = shm_data->sum;
                    int mean = shm_data->mean;
                    sem_post(semid);
                    write(pipe3[1], &sum, sizeof(sum));
                    write(pipe3[1], &mean, sizeof(mean));
                }
                else {
                    sem_post(semid);
                }
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

            while (fscanf(file, "Messwert %*d: %d", &num) == 1) {
                count++;
                sum += num;
                if (num < min)
                    min = num;
                if (num > max)
                    max = num;
            }
            fclose(file);

            printf("Anzahl der Messwerte: %d\n", count);
            printf("Summe der Messwerte: %d\n", sum);
            printf("Minimum der Messwerte: %d\n", min);
            printf("Maximum der Messwerte: %d\n", max);
            printf("Durchschnitt der Messwerte: %.2f\n", (float)sum / count);
            exit(0);
}
        

int main() {
    signal(SIGINT, handler);
        shmid = shmget(SHM_KEY, sizeof(SharedData), IPC_CREAT | 0666);
         if (shmid == -1) {
            perror("shmget");
            exit(1);
        }

        // Attach shared memory
        shm_data = (SharedData*)shmat(shmid, NULL, 0);
        if (shm_data == (void*)-1) {
            perror("shmat");
            exit(1);
        }
        // Initialize semaphore value
        semctl(semid, 0, SETVAL, 1);


    // hier werden durch fork() Elternprozesse dupliziert und so die Kindprozesse erstellt. Sie erben die funktionalität
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

    // Close the write end of pipe3 and the read end of pipe2 in the parent process
    close(pipe3[1]);
    close(pipe2[0]);

    while(!flag);

    cleanup();
    printf("Programm wird durch CTRL+C beendet\n");

    return 0;
}