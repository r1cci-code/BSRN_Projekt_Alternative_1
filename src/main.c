#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "conv.c"


int main() {
    int value;
    while (1) {
        value = conv();
        printf("Analog value: %d\n", value);
    }
    return 0;
}