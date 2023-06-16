#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>

#define PORT 8080
#define MAX_BUFFER_SIZE 1024

void handleConvClient(int clientSocket) {
    int i;
    char buffer[MAX_BUFFER_SIZE];

    for (i = 0; i < 10; i++) {
        // Zufallszahl erzeugen (hier vereinfacht)
        int randomValue = rand() % 100;
        sprintf(buffer, "%d", randomValue);

        // Messwert an Client senden
        send(clientSocket, buffer, strlen(buffer), 0);
        sleep(1);  // Kurze Pause zwischen den Messwerten
    }

    // Verbindung schließen
    close(clientSocket);
}

void handleLogClient(int clientSocket) {
    char buffer[MAX_BUFFER_SIZE];
    FILE* file = fopen("test.txt", "w");

    while (1) {
        // Messwert vom Conv-Server empfangen
        int bytesRead = recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0);
        if (bytesRead <= 0) {
            break;  // Verbindung wurde geschlossen
        }

        // Messwert in Datei schreiben
        fwrite(buffer, sizeof(char), bytesRead, file);
    }

    fclose(file);
    close(clientSocket);
}

void handleStatClient(int clientSocket) {
    char buffer[MAX_BUFFER_SIZE];
    int sum = 0;
    int count = 0;

    while (1) {
        // Messwert vom Conv-Server empfangen
        int bytesRead = recv(clientSocket, buffer, MAX_BUFFER_SIZE, 0);
        if (bytesRead <= 0) {
            break;  // Verbindung wurde geschlossen
        }

        // Statistische Daten berechnen
        int wert = atoi(buffer);
        summe += wert;
        count++;
    }

    // Mittelwert berechnen
    double mittelwert = (double)summe / count;

    // Mittelwert und Summe an Report-Server senden
    send(clientSocket, (char*)&mittelwert, sizeof(double), 0);
    send(clientSocket, (char*)&summe, sizeof(int), 0);

    close(clientSocket);
}

void handleReportClient(int clientSocket) {
    double mittelwert;
    int summe;

    // Statistische Daten vom Stat-Server empfangen
    recv(clientSocket, (char*)&mittelwert, sizeof(double), 0);
    recv(clientSocket, (char*)&summe, sizeof(int), 0);

    // Statistische Daten in der Shell ausgeben
    printf("Mittelwert: %.2f\n", mittelwert);
    printf("Summe: %d\n", summe);

    close(clientSocket);
}

int main() {
    int convSocket, logSocket, statSocket, reportSocket;
    struct sockaddr_in serverAddress;

    // Conv-Server
    convSocket = socket(AF_INET, SOCK_STREAM, 0);
    memset(&serverAddress, '0', sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);
    bind(convSocket, (struct sockaddr*)&serverAddress,