# BSRN_Projekt_Alternative_1
## Anforderungen aus Campuas:
Hier geben Sie eine erste Lösungsskizze für Werkstück A ab, bestehend aus:

1. Lösungsansatz des Problems (kurze Beschreibung 1-2 DIN A4 Seite)
2. Methodisches Vorgehen zur Lösungsfindung (bisherig und noch geplant)
3. Skizzen zum Lösungsansatz (Diagramme, Zeichnungen, Programmschnipsel, etc.)

## Gruppenmitglieder:
Daniel Riccius 1459702

Etienne Garcia Vihola 1443543

Tolgahan Sahin 1405743
## Lösungsansatz
Unsere Aufgabe ist es ein Echtzeitsystem zu erstellen, das aus vier Prozessen besteht. 

Der erste Prozess (Conv) generiert zufällige Messwerte.  

Der zweite Prozess (Log) liest die Messwerte vom ersten Prozess ein und speichert diese in einer lokalen Datei.  

Der dritte Prozess (Stat) liest die Werte von dem ersten Prozess aus und berechnet mit diesen statistische Daten z.B. die Summe oder den Mittelwert. Hierbei muss beachtet werden das Conv zuerst die Messwerte schreiben muss bevor Log und Stat diese auslesen können.  

Zu guter Letzt gibt es noch den vierten Prozess (Report). Dieser greift direkt auf die Ergebnisse des dritten Prozesses zu und gibt die Daten schließlich in der Shell aus. Hierbei ist auch wieder zu beachten das Stat die Daten erst schreiben muss bevor Report auf sie zugreifen kann. 

Außerdem soll für die Prozesse ein Gerüst erstellt werden,  womit man die Endlosprozesse, mit dem Systemaufruf fork, starten und mit Kommandos wie top, ps und pstree überwachen kann. 

Des Weiteren soll man mit dem Befehl Ctrl-C das Programm beenden können, indem man dafür das Signal SIGINT implementiert. 

## Struktur und methodisches Vorgehen
## Skizzen und Diagramme für Lösungsansatz und Implementation
### Strukturdiagramm
<img width="600" alt="Strukturdiagramm - Datenaustausch v3" src="https://user-images.githubusercontent.com/112110296/235207265-50e32d1b-91f9-4f0b-aaf1-37f82438c5f0.png">

## Implementierungsvarianten
### Pipes
Pipes werden für die einfachste Implementierungsvariante verwendet. Wir werden die Standard-Bibliotheksfunktionen pipe(), fork(), write() und read() verwenden, um die Kommunikation zwischen den Prozessen zu realisieren.

### Sockets
Zuerst muss der Server-Socket im Conv-Prozess erstellt werden.  

Danach müssen der Log-Prozess und der Stat-Prozess einen Client-Socket erstellen die mit dem Server-Socket des Conv-Prozesses verbunden sind. 

Der Report-Prozess erstellt ebenfalls einen Client-Socket jedoch verbindet sich dieser mit dem Server-Socket des Stat-Prozesses. 

Jetzt kann der Conv-Prozess zahlen generieren und diese per Socket an den Log-Prozess schicken. Der Log-Prozess speichert die Zahlen in einer Datei und sendet diese per Socket an den Stat-Prozess weiter. Dieser rechnet und übermittelt das Ergebnis per Socket weiter an den Report-Prozess, welcher die Ergebnisse schließlich in der Shell ausgibt.

### Message Queues
Für die Implementierung mit Message Queues werden wir die Funktionen msgget(), msgsnd(), msgrcv() und msgctl() verwenden, die in der Bibliothek <sys/msg.h> definiert sind.

### Shared Memory mit Semaphore
Bei der Implementierung mit Shared Memory und Semaphore wird die gemeinsam genutzte Speicherregion als Shared Memory angelegt. Zur Synchronisation wird ein Semaphore verwendet, der den Zugriff auf die Speicherregion steuert.

