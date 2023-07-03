#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <signal.h>
#include <time.h>

// die Variablen, die die Menge der Messwerte definieren
//zurzeit wird nur alle 1000 Messwerte mit dem Stat Prozess berechnet und ausgegeben.
#define USEC_PER_SEC 1000000
#define NUM_SAMPLES_PER_SEC 1000

#define MESSAGE_QUEUE_ID 1234
// handler um mit STRG+C das Programm zu beenden
volatile sig_atomic_t flag = 0;

void handler(int sig) {
    flag = 1;
}
// definieren der struktur der message queue
struct message {
    long msg_type;
    int msg_value;
};

int msgid;

// sorgt dafür das die Message Que geschlossen wird.
void cleanup() {
    msgctl(msgid, IPC_RMID, NULL);
}

// emulation eines 8-bit 5V A/D converter generiert zufällige Werte von 0 - 256. 
int conv() {
    int num = rand() % 256; 
    return num;
}

// conv wird in die message geschrieben und der Prozess ruht danach. Es entstehen ca 1000 Werte pro Sekunde um den
// Converter ordnungsgemäß zu emulieren 
void convProcess() {
    struct message msg = {1, 0};
    while(!flag) {
        msg.msg_value = conv();
        msgsnd(msgid, &msg, sizeof(msg), 0); // jeder Wert wird gesendet mit er durch die anderen Prozesse ausgelesen werden kann
        usleep(USEC_PER_SEC / NUM_SAMPLES_PER_SEC);
    }
    exit(0);
}

// Wenn eine message von convProcess empfangen wird wird mit der Inhalt in das log.txt file geschrieben
void logProcess() {
    struct message msg;
    int count = 0;
    FILE *file = fopen("log.txt", "w");
    if (file == NULL) {
        printf("Error opening file!\n");
        exit(1);
    }
    while (!flag) {
        msgrcv(msgid, &msg, sizeof(msg), 1, 0);
        fprintf(file, "Messwert %d: %d\n", count+1, msg.msg_value);
        count++;
    }
    fclose(file);
    exit(0);
}

// statProcess() empfängt durch die message queue die Daten von convProcess und
// erreichnet Mittelwert und Summe der Werte
void statProcess() {
    struct message msg;
    int num, sum = 0, count = 0, mean;
    while (!flag) {
        msgrcv(msgid, &msg, sizeof(msg), 1, 0);
        num = msg.msg_value;
        sum += num;
        count++;
        if (count % 1000 == 0) { // Ausgabe nur alle 1000 Werte.
            mean = sum / count;
            msg.msg_type = 2;
            msg.msg_value = sum;
            msgsnd(msgid, &msg, sizeof(msg), 0);
            msg.msg_value = mean;
            msgsnd(msgid, &msg, sizeof(msg), 0);
            msg.msg_type = 1;
        }
    }
    exit(0);
}
// reportProcess empfängt Daten (Mittelwert und Summe der Messwerte) aus stat und gibt sie in der Konsole aus
void reportProcess() {
    struct message msg;
    int sum, mean;
    while (!flag) {
        msgrcv(msgid, &msg, sizeof(msg), 2, 0);
        sum = msg.msg_value;
        msgrcv(msgid, &msg, sizeof(msg), 2, 0);
        mean = msg.msg_value;
        printf("Summe: %d, Mittelwert: %d\n", sum, mean);
    }
    exit(0);
}

int main() {
    signal(SIGINT, handler);

    msgid = msgget(MESSAGE_QUEUE_ID, 0666 | IPC_CREAT);
    if (msgid == -1) {
        fprintf(stderr, "msgget failed\n");
        return 1;
    }

    pid_t p1, p2, p3, p4;

    p1 = fork();
    if (p1 < 0) {
        fprintf(stderr, "fork Failed");
        return 1;
    }
    // Wenn p1 == 0 ist, ist p1 als Kindprozess erstellt und
    // kann die zugehörige funktion aufrufen
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
    while(!flag);

    cleanup();
    printf("Programm wird durch CTRL+C beendet\n");

    return 0;
}
