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

// der Port auf dem die Sockets kommunizieren
#define Port1 50001
#define Port2 50002
#define Port3 50003
#define Port4 50004

//zurzeit wird nur alle 1000 Messwerte mit dem Stat Prozess berechnet und ausgegeben.
#define USEC_PER_SEC 1000000
#define NUM_SAMPLES_PER_SEC 1000
//DIE KOMMUNIKATIONSMETHODE WIRD NUR IN main() GEÄNDERT
//define all possible comunication options
enum CommunicationMethod {
    PIPES,
    MESSAGE_QUEUE,
    SOCKETS,
    SHARED_MEMORY
};

// Global variable to hold the selected communication method
enum CommunicationMethod communication_method;

volatile sig_atomic_t flag = 0;

void handler(int sig) {
    flag = 1;
}
// sockets werden definiert
int socket1[2], socket2[2], socket3[2], socket4[2];

// die Pipes werden definiert, Prozess Identification wird mit pid_t datentyp erstellt, File wird auf NULL gesetzt
int pipe1[2], pipe2[2], pipe3[2], pipe4[2];
pid_t p1, p2, p3, p4;
FILE *logFile = NULL;


// es ist eventuell nötig auch ein case handling für cleanup() zu verwenden um alle möglichen
// Kommunikationsmethoden ordnungsgemäß zu beenden
void cleanup() {
    switch (communication_method) {
        case PIPES:
            // Implement the functionality using pipes here
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
            break;
        case MESSAGE_QUEUE:
            // Implement the functionality using message queues here
            break;
        case SOCKETS:
            // Implement the functionality using sockets here
            close(socket1[0]);
            close(socket1[1]);
            close(socket2[0]);
            close(socket2[1]);
            close(socket3[0]);
            close(socket3[1]);
            close(socket4[0]);
            close(socket4[1]);
            break;
        case SHARED_MEMORY:
            // Implement the functionality using shared memory here
            break;
        default:
            fprintf(stderr, "Unsupported communication method\n");
            exit(1);
        }
    
}

// 8-bit 5V A/D converter generates 256 different possible values
int conv() {
    int num = rand() % 256; 
    return num;
}

void doConvProcess() {
    switch (communication_method) {
        case PIPES:
            // Implement the functionality using pipes here
            close(pipe1[0]); 
            while(!flag) {
                int num = conv();
                write(pipe1[1], &num, sizeof(num));
                usleep(USEC_PER_SEC / NUM_SAMPLES_PER_SEC);  // Pause for 1/1000 sec. corresponds to ~1Hz A/D converter
            }
            close(pipe1[1]); 
            exit(0);
            break;
        case MESSAGE_QUEUE:
            // Implement the functionality using message queues here
            break;
        case SOCKETS:
            // Implement the functionality using sockets here
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
            break;
        case SHARED_MEMORY:
            // Implement the functionality using shared memory here
            break;
        default:
            fprintf(stderr, "Unsupported communication method\n");
            exit(1);
        }
    
    
}

void doLogProcess() {
    switch (communication_method) {
        case PIPES:
            // Implement the functionality using pipes here
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
            break;
        case MESSAGE_QUEUE:
            // Implement the functionality using message queues here
            break;
        case SOCKETS:
            // Implement the functionality using sockets here
            struct sockaddr_in serv_addr;
            int valread;

            // Konfiguration des Socket
            serv_addr.sin_family = AF_INET;
            serv_addr.sin_port = htons( Port2 );

            // Convert IPv4 and IPv6 addresses from text to binary form
            if(inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
                printf("\nInvalid address/ Address not supported \n");
                return -1;
            }

            if (connect(socket1[0], (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
                printf("\nConnection Failed \n");
                return -1;
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
            break;
        case SHARED_MEMORY:
            // Implement the functionality using shared memory here
            break;
        default:
            fprintf(stderr, "Unsupported communication method\n");
            exit(1);
    }
}

void doStatProcess() {
    switch (communication_method) {
        case PIPES:
            // Implement the functionality using pipes here
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

            break;
        case MESSAGE_QUEUE:
            // Implement the functionality using message queues here
            break;
        case SOCKETS:
            // Implement the functionality using sockets here
            struct sockaddr_in address;
            int valread, addrlen = sizeof(address);
            
            address.sin_family = AF_INET;
            address.sin_addr.s_addr = INADDR_ANY;
            address.sin_port = htons( Port3 );

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
            break;
        case SHARED_MEMORY:
            // Implement the functionality using shared memory here
            break;
        default:
            fprintf(stderr, "Unsupported communication method\n");
            exit(1);
    }
}

void doReportProcess() {
    switch (communication_method) {
        case PIPES:
            // Implement the functionality using pipes here
            close(pipe3[1]);
            int sum, mean;
            while (!flag) {
            if (read(pipe3[0], &sum, sizeof(sum)) > 0 && read(pipe3[0], &mean, sizeof(mean)) > 0) {
                printf("Summe: %d, Mittelwert: %d\n", sum, mean);
                    }
                }
                close(pipe3[0]);
                exit(0);
            break;
        case MESSAGE_QUEUE:
            // Implement the functionality using message queues here
            break;
        case SOCKETS:
            // Implement the functionality using sockets here
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
                serv_addr.sin_port = htons(Port4); // PORT4 is for the report process

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
                break;
        case SHARED_MEMORY:
            // Implement the functionality using shared memory here
            break;
        default:
            fprintf(stderr, "Unsupported communication method\n");
            exit(1);
    }
}

int main() {
    signal(SIGINT, handler);
    
    // Set the communication method here
    communication_method = SOCKETS; 

    // die jeweilige intialisierung der Kommunikationsmethode
    switch (communication_method) {
        case PIPES:
            // Implement the functionality using pipes here
            if (pipe(pipe1) == -1 || pipe(pipe2) == -1 || pipe(pipe3) == -1) {
                fprintf(stderr, "Pipe Failed");
                return 1;
            }
            break;
        case MESSAGE_QUEUE:
            // Implement the functionality using message queues here
            break;
        case SOCKETS:
            // Implement the functionality using sockets here
            for (int i = 0; i < 2; i++) {
                socket1[i] = socket(AF_INET, SOCK_STREAM, 0);
                socket2[i] = socket(AF_INET, SOCK_STREAM, 0);
                socket3[i] = socket(AF_INET, SOCK_STREAM, 0);
                socket4[i] = socket(AF_INET, SOCK_STREAM, 0);
            }
            break;
        case SHARED_MEMORY:
            // Implement the functionality using shared memory here
            break;
        default:
            fprintf(stderr, "Unsupported communication method\n");
            return(1);
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

    // Close the write end of pipe3 and the read end of pipe2 in the parent process
    close(pipe3[1]);
    close(pipe2[0]);

    while(!flag);

    cleanup();
    printf("Programm wird durch CTRL+C beendet\n");

    return 0;
}