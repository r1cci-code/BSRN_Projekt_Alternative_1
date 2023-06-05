#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define USEC_PER_SEC 1000000
#define NUM_SAMPLES_PER_SEC 1000

volatile sig_atomic_t flag = 0;

void handler(int sig) {
    flag = 1;
}

int pipe1[2], pipe2[2], pipe3[2], pipe4[2];
pid_t p1, p2, p3, p4;
FILE *logFile = NULL;

void cleanup() {
    close(pipe1[0]);
    close(pipe1[1]);
    close(pipe2[0]);
    close(pipe2[1]);
    close(pipe3[0]);
    close(pipe3[1]);
    close(pipe4[0]);
    close(pipe4[1]);
    if (logFile != NULL) {
        fclose(logFile);
    }
}

// 8-bit 5V A/D converter generates 256 different possible values
int conv() {
    int num = rand() % 256; 
    return num;
}

void doConvProcess() {
    close(pipe1[0]); 
    while(!flag) {
        int num = conv();
        write(pipe1[1], &num, sizeof(num));
        usleep(USEC_PER_SEC / NUM_SAMPLES_PER_SEC);  // Pause for 1/1000 sec. corresponds to ~1Hz A/D converter
    }
    close(pipe1[1]); 
    exit(0);
}

void doLogProcess() {
    close(pipe1[1]);
    close(pipe2[0]);
    FILE *file = fopen("log.txt", "w");
    if (file == NULL) {
        printf("Error opening file!\n");
        exit(1);
    }
    int num;
    int count = 0;
    while(read(pipe1[0], &num, sizeof(num)) > 0 && !flag) {
        fprintf(file, "Messwert %d: %d\n", count+1, num);
        count++;
        write(pipe2[1], &num, sizeof(num)); // Write the number to pipe2 for the stat process
    }
    fclose(file);
    close(pipe1[0]);
    close(pipe2[1]);
    exit(0);
}

void doStatProcess() {
    close(pipe2[1]);
    int num, sum = 0, count = 0, mean;
    while (!flag) {
        if (read(pipe2[0], &num, sizeof(num)) > 0) {
            sum += num;
            count++;
            if (count % 1000 == 0) { // Ausgabe nur alle 1000 Werte.
                mean = sum / count;

                write(pipe3[1], &sum, sizeof(sum));
                write(pipe3[1], &mean, sizeof(mean));
            }
        }
    }
    close(pipe2[0]);
    exit(0);
}

void doReportProcess() {
    close(pipe3[1]);
    int sum, mean;
    while (!flag) {
        if (read(pipe3[0], &sum, sizeof(sum)) > 0 && read(pipe3[0], &mean, sizeof(mean)) > 0) {
            printf("Summe: %d, Mittelwert: %d\n", sum, mean);
        }
    }
    close(pipe3[0]);
    exit(0);
}

int main() {
    signal(SIGINT, handler);

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1 || pipe(pipe3) == -1) {
        fprintf(stderr, "Pipe Failed");
        return 1;
    }

    // Create child processes
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