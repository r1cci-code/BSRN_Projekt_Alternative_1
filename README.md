# BSRN_Projekt_Alternative_1

## Lösungsansatz

Die Aufgabe besteht darin, ein Echtzeitsystem zu entwickeln, das aus vier Prozessen besteht. Der erste Prozess (Conv) generiert zufällige Messwerte und prüft sie auf Plausibilität. Die nächsten beiden Prozesse (Log und Stat) lesen die Messwerte von Conv aus und führen verschiedene Operationen durch. Schließlich gibt der vierte Prozess (Report) die statistischen Daten aus. Die Synchronisationsbedingungen zwischen den Prozessen sind zu beachten.

## Struktur und methodisches Vorgehen

Um die geforderte Lösung zu implementieren, wird der folgende Ansatz verfolgt:

1. Identifizierung der benötigten Bibliotheken und Systemaufrufe.
2. Implementierung des **Conv-Prozesses**, der zufällige Messwerte generiert und auf Plausibilität prüft.
3. Implementierung des **Log-Prozesses**, der die Messwerte von Conv ausliest und in eine lokale Datei schreibt.
4. Implementierung des **Stat-Prozesses**, der die Messwerte von Conv ausliest und statistische Daten berechnet (Mittelwert und Summe).
5. Implementierung des **Report-Prozesses**, der auf die Ergebnisse von Stat zugreift und die statistischen Daten in der Shell ausgibt.
6. **Implementierung der Synchronisationsbedingungen** zwischen den Prozessen.
7. Implementierung der vier Implementierungsvarianten des Programms, wobei der **Datenaustausch** zwischen den Prozessen einmal mit Pipes, Message Queues, Shared Memory mit Semaphore und via Sockets realisiert wird.
8. **Testen** der Implementierung auf Fehler und Optimierung des Codes.

## Skizzen und Diagramme für Lösungsansatz und Implementation

### Strukturdiagramm

Das folgende Strukturdiagramm zeigt die vier Prozesse des Echtzeitsystems und den Datenaustausch zwischen ihnen.

```
                 +-----+      +-----+
                 | Conv|----->| Log |
                 +-----+      +-----+
                    ^            ^
                    |            |
                    |            |
                 +-----+      +-----+
                 | Stat|----->|Rep. |
                 +-----+      +-----+
```

### Datenflussdiagramm

Das folgende Datenflussdiagramm zeigt den Datenaustausch zwischen den vier Prozessen.

```
        +-----+            +-----+             +-----+           +-----+
        |Conv |            | Log |             |Stat |           |Rep. |
        +-----+            +-----+             +-----+           +-----+
            |                  |                   |                 |
            |                  |                   |                 |
            |----------------->|------------------>|---------------->|
            |     Messwert     | Messwert und Zeit |  Messwert und   |
            |                  |                   |   statistische  |
            |                  |                   |   Daten         |
            |                  |                   |<----------------|
            |                  |<------------------|<----------------|
            |<-----------------|  Bestätigung      |                 |
            |     Bestätigung  |                   |                 | 
        +-----+            +-----+             +-----+           +-----+
        | RNG |            |File |             |Calc.|           |Shell|
        +-----+            +-----+             +-----+           +-----+
```
## Implementierungsvarianten
Wir werden das System mit vier verschiedenen Implementierungsvarianten erstellen, die jeweils den Datenaustausch zwischen den Prozessen mit **Pipes**, **Message Queues**, **Shared Memory mit Semaphore** und **Sockets** realisieren.

Die Implementierung der verschiedenen Varianten erfordert unterschiedliche Bibliotheken und Funktionen, die für den Datenaustausch und die Synchronisation der Prozesse verwendet werden.

### Sockets
1. Erstellen Sie den Server-Socket im Conv-Prozess. Dieser wird verwendet, um die Verbindung mit den Client-Sockets herzustellen. Die Funktion socket() wird verwendet, um den Socket zu erstellen, und bind() wird verwendet, um ihn an eine bestimmte Adresse zu binden (z. B. "localhost" oder eine andere lokale IP-Adresse). Danach wird listen() aufgerufen, um auf eingehende Verbindungen zu warten.

2. Der Log-Prozess erstellt einen Client-Socket, der mit dem Server-Socket des Conv-Prozesses verbunden wird. Die Funktion socket() wird verwendet, um den Socket zu erstellen, und connect() wird verwendet, um eine Verbindung zum Server-Socket herzustellen.

3. Der Stat-Prozess erstellt ebenfalls einen Client-Socket und verbindet sich mit dem Server-Socket des Conv-Prozesses.

4. Der Report-Prozess erstellt auch einen Client-Socket und verbindet sich mit dem Server-Socket des Stat-Prozesses.

5. Der Conv-Prozess generiert Zufallszahlen und sendet sie über den Socket an den Log-Prozess. Hierfür wird send() verwendet, um die Daten an den Client-Socket zu senden.

6. Der Log-Prozess empfängt die Daten vom Socket mit recv() und schreibt sie in eine lokale Datei. Danach sendet er die Daten über einen anderen Socket an den Stat-Prozess.

7. Der Stat-Prozess empfängt die Daten vom Socket, berechnet statistische Daten und sendet die Ergebnisse über einen anderen Socket an den Report-Prozess.

8. Der Report-Prozess empfängt die Ergebnisse vom Socket mit recv() und gibt sie in der Shell aus.

Es gibt noch weitere Details, die bei der Implementierung zu beachten sind, wie beispielsweise die Verwendung von select() oder Threads, um eingehende Verbindungen und Datenverkehr auf den Sockets zu verwalten.

### Pipes
Pipes werden für die einfachste Implementierungsvariante verwendet. Wir werden die Standard-Bibliotheksfunktionen **pipe()**, **fork()**, **write()** und **read()** verwenden, um die Kommunikation zwischen den Prozessen zu realisieren.

### Message Queues
Für die Implementierung mit Message Queues werden wir die Funktionen **msgget()**, **msgsnd()**, **msgrcv()** und **msgctl()** verwenden, die in der Bibliothek **<sys/msg.h**> definiert sind.

### Shared Memory mit Semaphore
Bei der Implementierung mit Shared Memory und Semaphore wird die gemeinsam genutzte Speicherregion als Shared Memory angelegt. Zur Synchronisation wird ein Semaphore verwendet, der den Zugriff auf die Speicherregion steuert.

#### Lösungsansatz
1. Die Prozesse Conv, Log und Stat legen zunächst den Shared Memory an, auf den sie zugreifen werden, und initialisieren ihn mit den erforderlichen Daten.
2. Der Semaphore wird initialisiert und auf den Wert 1 gesetzt.
3. Conv erzeugt Zufallszahlen und schreibt sie in den Shared Memory. Bevor er schreibt, wartet er darauf, dass der Semaphore den Wert 1 hat. Wenn der Semaphore den     Wert 0 hat, wartet Conv darauf, dass der Semaphore wieder den Wert 1 hat.
4. Log und Stat lesen die Daten aus dem Shared Memory, indem sie auf den Semaphore warten. Wenn der Semaphore den Wert 1 hat, lesen sie die Daten und setzen den Semaphore wieder auf 1.
5. Stat berechnet die statistischen Daten und schreibt sie in den Shared Memory. Bevor er schreibt, wartet er darauf, dass der Semaphore den Wert 1 hat. Wenn der Semaphore den Wert 0 hat, wartet Stat darauf, dass der Semaphore wieder den Wert 1 hat.
6. Report liest die statistischen Daten aus dem Shared Memory, indem er auf den Semaphore wartet. Wenn der Semaphore den Wert 1 hat, liest Report die Daten und gibt sie in der Shell aus. Danach setzt er den Semaphore wieder auf 1.
