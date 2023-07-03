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
#include <string.h>

// der Port auf dem die Sockets kommunizieren
#define Port1 50001
#define Port2 50002
#define Port3 50003
#define Port4 50004

//zurzeit wird nur alle 1000 Messwerte mit dem Stat Prozess berechnet und ausgegeben.
#define USEC_PER_SEC 1000000
#define NUM_SAMPLES_PER_SEC 1000

volatile sig_atomic_t flag = 0;

void handler(int sig) {
    flag = 1;
}
// sockets werden Discriptoren
int socket1[2], socket2[2], socket3[2], socket4[2];


pid_t p1, p2, p3, p4;
FILE *logFile = NULL;


// es ist eventuell nötig auch ein case handling für cleanup() zu verwenden um alle möglichen
// Kommunikationsmethoden ordnungsgemäß zu beenden
void cleanup() {
    close(socket1[0]);
    close(socket1[1]);
    close(socket2[0]);
    close(socket2[1]);
    close(socket3[0]);
    close(socket3[1]);
    close(socket4[0]);
    close(socket4[1]);
}

// 8-bit 5V A/D converter generates 256 different possible values
int conv() {
    int num = rand() % 256; 
    return num;
}

void doConvProcess() {
            struct sockaddr_in address;
            int addrlen = sizeof(address);
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons( Port1 );

            bind(socket1[1], (struct sockaddr *)&address, sizeof(address));
            listen(socket1[1], 3);
            int new_socket = accept(socket1[1], (struct sockaddr *)&address, (socklen_t*)&addrlen);

            while(!flag) {
                int num = conv();
                send(new_socket, &num, sizeof(num), 0);
                usleep(USEC_PER_SEC / NUM_SAMPLES_PER_SEC);  
            }
            close(socket1[1]);
            close(new_socket);
            exit(0);
}

void doLogProcess() {
            struct sockaddr_in serv_addr;
            int valread;

            // Konfiguration des Socket
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons( Port1 );

            // Convert IPv4 and IPv6 addresses from text to binary form
            if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
                printf("\nInvalid address/ Address not supported \n");
                return;
            }

            if (connect(socket1[0], (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                printf("\nConnection Failed \n");
                return;
            }

            // Eröffnen der Log-Datei
            FILE *file = fopen("log.txt", "w");
            if (file == NULL) {
                printf("Error opening file!\n");
                exit(1);
            }

            int num;
            int count = 0;
            while (!flag) {
                valread = read(socket1[0], &num, sizeof(num));
                if(valread > 0) {
                    fprintf(file, "Messwert %d: %d\n", count+1, num);
                    count++;
                    write(socket2[1], &num, sizeof(num)); // Write the number to socket2 for the stat process
                }
            }

            fclose(file);
            close(socket1[0]);
            close(socket2[1]);
            exit(0);
}

void doStatProcess() {
            struct sockaddr_in address;
            int valread, addrlen = sizeof(address);
            
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons( Port1 );

            bind(socket2[0], (struct sockaddr *)&address, sizeof(address));
            listen(socket2[0], 3);
            int new_socket = accept(socket2[0], (struct sockaddr *)&address, (socklen_t*)&addrlen);

            int num, sum = 0, count = 0, mean;
            while (!flag) {
                valread = read(new_socket, &num, sizeof(num));
                if (valread > 0) {
                    sum += num;
                    count++;
                    if (count % 1000 == 0) {
                        mean = sum / count;
                        write(socket3[1], &sum, sizeof(sum));
                        write(socket3[1], &mean, sizeof(mean));
                    }
                }
            }
            close(socket2[0]);
            close(new_socket);
            exit(0);
}
       

void doReportProcess() {
                struct sockaddr_in address;
                int sock = 0, valread;
                struct sockaddr_in serv_addr;
                int addrlen = sizeof(serv_addr);
                char buffer[1024] = {0};

                if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
                    printf("\n Socket creation error \n");
                    exit(EXIT_FAILURE);
                }

                memset(&serv_addr, '0', sizeof(serv_addr));

                serv_addr.sin_family = AF_INET;
                serv_addr.sin_port = htons( Port2 ); // PORT4 is for the report process

                // Convert IPv4 and IPv6 addresses from text to binary form
                if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
                    printf("\nInvalid address/ Address not supported \n");
                    exit(EXIT_FAILURE);
                }

                if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                    printf("\nConnection Failed \n");
                    exit(EXIT_FAILURE);
                }

                int sum, mean;
                while (!flag) {
                    if (read(sock, &sum, sizeof(sum)) > 0 && read(sock, &mean, sizeof(mean)) > 0) {
                        printf("Summe: %d, Mittelwert: %d\n", sum, mean);
                    }
                }
                close(sock);
                exit(0);
}

int main() {
    signal(SIGINT, handler);
            for (int i = 0; i < 2; i++) {
                socket1[i] = socket(AF_INET, SOCK_STREAM, 0);
                socket2[i] = socket(AF_INET, SOCK_STREAM, 0);
                socket3[i] = socket(AF_INET, SOCK_STREAM, 0);
                socket4[i] = socket(AF_INET, SOCK_STREAM, 0);
            }
    

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


    while(!flag);

    cleanup();
    printf("Programm wird durch CTRL+C beendet\n");

    return 0;
}