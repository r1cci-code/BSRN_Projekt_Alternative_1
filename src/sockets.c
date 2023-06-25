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

//zurzeit wird nur alle 1000 Messwerte mit dem Stat Prozess berechnet und ausgegeben.
#define USEC_PER_SEC 1000000
#define NUM_SAMPLES_PER_SEC 1000

volatile sig_atomic_t flag = 0;

void handler(int sig) {
    flag = 1;
}

pid_t p1, p2, p3, p4;
FILE *logFile = NULL;

void cleanup() {
}

// Socket variables
int server_fd, new_socket;

void handle_sigint(int sig) {
    close(server_fd);
    exit(0);
}

void setup_socket(PORT) {
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }
    
    // Forcefully attaching socket to the port 8080
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }
    
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons( PORT );
    
    // Forcefully attaching socket to the port 8080
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }
    
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }
    
    if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }
}

// 8-bit 5V A/D converter generates 256 different possible values
int conv() {
    int num = rand() % 256; 
    return num;
}

void convProcess() {
//Socket erstellen

            while(!flag) {
                int num = conv();
                //write
                usleep(USEC_PER_SEC / NUM_SAMPLES_PER_SEC);  // Pause for 1/1000 sec. corresponds to ~1Hz A/D converter
            }
            exit(0);
}

void logProcess() {
            
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

void statProcess() {
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
        exit(0);
}

void reportProcess() {
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
    
    // hier werden durch fork() Elternprozesse dupliziert und so die Kindprozesse erstellt. Sie erben die funktionalität
    p1 = fork();
    if (p1 < 0) {
        fprintf(stderr, "fork Failed");
        return 1;
    }

    if (p1 == 0) {
        convProcess();
    } 

    p2 = fork();
    if (p2 < 0) {
        fprintf(stderr, "fork Failed");
        return 1;
    }

    if (p2 == 0) {
        logProcess();
    }

    p3 = fork();
    if (p3 < 0) {
        fprintf(stderr, "fork Failed");
        return 1;
    }

    if (p3 == 0) {
        statProcess();
    }

    p4 = fork();
    if (p4 < 0) {
        fprintf(stderr, "fork Failed");
        return 1;
    }

    if (p4 == 0) {
        reportProcess();
    }

    while(!flag);

    cleanup();
    printf("Programm wird durch CTRL+C beendet\n");

    return 0;
}