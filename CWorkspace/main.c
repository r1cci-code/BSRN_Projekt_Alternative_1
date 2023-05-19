#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "conv.c"
#include "log.c"
#include "stat.c"




int main(){
   int zahlen[10];
   int zahl;
    FILE *datei;
    datei = fopen("test.txt","w");
   for(int i =0;i<10;i++){
    zahl = conv();
    zahlen[i]=zahl;
    printf(" %d\n",zahl);
    log(zahl,datei);
   }
   stat(zahlen);
   fclose (datei);

   return 0;
}
 