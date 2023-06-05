#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <time.h>

#define NUM_SAMPLES 256
//im conv prozess soll ein 8 bit A/D-Converter mit 5V Referenzspannung simuliert werden
int conv() {
    srand(time(NULL));
    int num = rand() % 256; // Zufallszahl zwischen 0 und 256
    return num;
}

int main(void) {
    int pipe1[2], pipe2[2], pipe3[2], pipe4[2];
    pid_t p1, p2, p3, p4;

    // Erzeuge Pipes
    if (pipe(pipe1) == -1 || pipe(pipe2) == -1 || pipe(pipe3) == -1 || pipe(pipe4) == -1) {
        fprintf(stderr, "Pipe Failed");
        return 1;
    }

    // Erzeuge Prozesse
    p1 = fork();
    if (p1 < 0) {
        fprintf(stderr, "fork Failed");
        return 1;
    }

    // Conv Prozess
    if (p1 == 0) {
        close(pipe1[0]); // Schließe Leseende
        for(int i=0; i<NUM_SAMPLES; i++) {
            int num = conv();
            write(pipe1[1], &num, sizeof(num));
        }
        close(pipe1[1]); // Schließe Schreibende
    } else { 
        p2 = fork();
        if (p2 < 0) {
            fprintf(stderr, "fork Failed");
            return 1;
        }

        // Log Prozess
        if (p2 == 0) {
            close(pipe1[1]); // Schließe Schreibende
            int nums[NUM_SAMPLES], num;
            for(int i=0; i<NUM_SAMPLES; i++) {
                read(pipe1[0], &num, sizeof(num));
                nums[i] = num;
            }
            close(pipe1[0]); // Schließe Leseende
            
            // Hier könnten Sie die Zahlen in einer Datei speichern
            FILE *file = fopen("log.txt", "w");
            if (file == NULL)
            {
                printf("Error opening file!\n");
                exit(1);
            }

            for(int i=0; i<NUM_SAMPLES; i++) {
                fprintf(file, "Messwert %d: %d\n", i+1, nums[i]);
            }

            fclose(file);
            
            // Senden Sie die Zahlen an den Stat-Prozess
            close(pipe2[0]); // Schließe Leseende
            write(pipe2[1], nums, sizeof(nums));
            close(pipe2[1]); // Schließe Schreibende
        } else {
            p3 = fork();
            if (p3 < 0) {
                fprintf(stderr, "fork Failed");
                return 1;
            }

            // Stat Prozess
            if (p3 == 0) {
                close(pipe2[1]); // Schließe Schreibende
                int nums[NUM_SAMPLES];
                read(pipe2[0], nums, sizeof(nums));
                close(pipe2[0]); // Schließe Leseende
                
                int sum = 0;
                for(int i=0; i<NUM_SAMPLES; i++) {
                    sum += nums[i];
                }
                int mean = sum / NUM_SAMPLES;
                
                // Senden Sie die Statistik an den Report-Prozess
                close(pipe3[0]); // Schließe Leseende
                write(pipe3[1], &sum, sizeof(sum));
                write(pipe3[1], &mean, sizeof(mean));
                close(pipe3[1]); // Schließe Schreibende
            } else {
                // Report Prozess
                close(pipe3[1]); // Schließe Schreibende
                int sum, mean;
                read(pipe3[0], &sum, sizeof(sum));
                read(pipe3[0], &mean, sizeof(mean));
                close(pipe3[0]); // Schließe Leseende
                
                printf("Summe: %d, Mittelwert: %d\n", sum, mean);
            }
        }
    }

    return 0;
}
