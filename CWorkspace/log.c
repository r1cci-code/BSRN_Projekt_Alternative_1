#include <stdio.h>
#include <stdlib.h>
#include <time.h>

int log(int x, FILE *datei){
    fprintf(datei,"%d\n",x);
    return 0;
 }