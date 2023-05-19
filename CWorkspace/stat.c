#include <stdio.h>
#include <stdlib.h>
#include <time.h>

void stat(int zahlen[]){
int summe = 0;
int mittelwert;
    for(int i = 0;i<10;i++){
        summe +=zahlen[i];
    } 
    printf("Die Summe beträgt: %d\n",summe);
    mittelwert=summe/10;
    printf("Der Mittelwert beträgt: %d\n",mittelwert);
    return 0;
}