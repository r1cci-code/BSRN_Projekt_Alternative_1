

# Lösungsansatz

Die Aufgabe besteht darin, ein Echtzeitsystem zu entwickeln, das aus vier Prozessen besteht. Der erste Prozess (Conv) generiert zufällige Messwerte und prüft sie auf Plausibilität. Die nächsten beiden Prozesse (Log und Stat) lesen die Messwerte von Conv aus und führen verschiedene Operationen durch. Schließlich gibt der vierte Prozess (Report) die statistischen Daten aus. Die Synchronisationsbedingungen zwischen den Prozessen sind zu beachten.

# Struktur und methodisches Vorgehen

Um die geforderte Lösung zu implementieren, wird der folgende Ansatz verfolgt:

1. Identifizierung der benötigten Bibliotheken und Systemaufrufe.
2. Implementierung des Conv-Prozesses, der zufällige Messwerte generiert und auf Plausibilität prüft.
3. Implementierung des Log-Prozesses, der die Messwerte von Conv ausliest und in eine lokale Datei schreibt.
4. Implementierung des Stat-Prozesses, der die Messwerte von Conv ausliest und statistische Daten berechnet (Mittelwert und Summe).
5. Implementierung des Report-Prozesses, der auf die Ergebnisse von Stat zugreift und die statistischen Daten in der Shell ausgibt.
6. Implementierung der Synchronisationsbedingungen zwischen den Prozessen.
7. Implementierung der vier Implementierungsvarianten des Programms, wobei der Datenaustausch zwischen den Prozessen einmal mit Pipes, Message Queues, Shared Memory mit Semaphore und via Sockets realisiert wird.
8. Testen der Implementierung auf Fehler und Optimierung des Codes.

# Skizzen und Diagramme für Lösungsansatz und Implementation

## Strukturdiagramm

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

## Datenflussdiagramm

Das folgende Datenflussdiagramm zeigt den Datenaustausch zwischen den vier Prozessen.

```
        +-----+            +-----+             +-----+           +-----+
        |Conv |            | Log |             |Stat |           |Rep. |
        +-----+            +-----+             +-----+           +-----+
            |                  |                   |                 |
            |                  |                   |                 |
            |----------------->|------------------>|---------------->|
            |     Messwert    |  Messwert und Zeit  |  Messwert und   |
            |                  |                   |   statistische   |
            |                  |                   |   Daten         |
            |                  |                   |<----------------|
            |                  |<------------------|<----------------|
            |<-----------------|  Bestätigung       |                 |
            |     Bestätigung  |                   |                 |
        +-----+            +-----+             +-----+           +-----+
        |  RNG |            |File |             |Calc.|           |Shell|
        +-----+            +-----+             +-----+           +-----+
```
