#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

// die Variablen, die die Menge der Messwerte definieren
//zurzeit wird nur alle 1000 Messwerte mit dem Stat Prozess berechnet und ausgegeben.
#define USEC_PER_SEC 1000000
#define NUM_SAMPLES_PER_SEC 1000

volatile sig_atomic_t flag = 0;
// handler um mit STRG+C das Programm zu beenden
void handler(int sig) {
    flag = 1;
}

// die Pipes werden definiert, Prozess Identification wird mit pid_t datentyp erstellt,
// File wird auf NULL gesetzt damit es wieder neu beschrieben werden kann
int pipe1[2], pipe2[2], pipe3[2];
pid_t p1, p2, p3, p4;
FILE *logFile = NULL;

// funktion um das ordnungsgemäße Beenden der Prozesse zu garantieren.
void cleanup() {
            close(pipe1[1]);
            close(pipe2[0]);
            close(pipe2[1]);
            close(pipe3[0]);
            close(pipe3[1]);
            if (logFile != NULL) {
                fclose(logFile);
            }
}


int conv() {
    int num = rand() % 256; 
    return num;
}

// conv wird in variable gespeichert, danach ruht der Prozess.
// Es entstehen ca. 1000 Werte pro Sekunde um einen 
// 8-bit 5V A/D Converter zu emulieren 
void convProcess() {
            close(pipe1[0]); 
            while(!flag) {
                int num = conv();
                write(pipe1[1], &num, sizeof(num));
                usleep(USEC_PER_SEC / NUM_SAMPLES_PER_SEC);  // Pause for 1/1000 sec. corresponds to ~1Hz A/D converter
            }
            close(pipe1[1]); 
            exit(0);
}

// wenn Daten von convProcess() empfangen werden wird der
// Inhalt in das log.txt file geschrieben
void logProcess() {
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
                write(pipe2[1], &num, sizeof(num));
            }
            fclose(file);
            close(pipe1[0]);
            close(pipe2[1]);
            exit(0);
}
// statProcess() empfängt durch die message queue die Daten von convProcess und
// erreichnet Mittelwert und Summe der Werte
void statProcess() {
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
// reportProcess empfängt Daten (Mittelwert und Summe der Messwerte) 
// aus statProcess() und gibt sie in der Konsole aus
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

    if (pipe(pipe1) == -1 || pipe(pipe2) == -1 || pipe(pipe3) == -1) {
        fprintf(stderr, "Pipe Failed");
            return 1;
    }
    
    // hier werden durch fork() Elternprozesse dupliziert und so die Kindprozesse erstellt.
    // Sie erben die funktionalität
    p1 = fork();
    if (p1 < 0) {
        fprintf(stderr, "fork Failed");
        return 1;
    }
    // Wenn p1 == 0 ist p1 als Kindprozess erstellt und kann die zugehörige funktion aufrufen
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

    // die cleanup funktion wird aufgerufen um 
    // ein ordnugsgemäßes Beenden des Prozess zu garantieren
    
    close(pipe3[1]);
    close(pipe2[0]);
    
    // die cleanup funktion wird aufgerufen um 
    // ein ordnugsgemäßes Beenden des Prozess zu garantieren
    while(!flag);

    cleanup();
    printf("Programm wird durch CTRL+C beendet\n");

    return 0;
}