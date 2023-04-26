#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define MAX_ANALOG_VALUE 1023
#define MAX_DELAY_MS 50

int conv() {
    srand(time(NULL));
    int analog_value;
    int delay_ms;
    while (1) {
        analog_value = rand() % (MAX_ANALOG_VALUE + 1);
        delay_ms = rand() % (MAX_DELAY_MS + 1);
        usleep(delay_ms * 1000); // Sleep for delay_ms milliseconds
        return analog_value;
    }
}

