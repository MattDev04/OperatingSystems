#include <stdio.h>

void busy_work() {
    volatile long i;
    for (i = 0; i < 1000000000; i++) {
        i = i * i / (i + 1);
    }
}

int main() {
    while (1) {
        busy_work();
    }

    return 0;
}

