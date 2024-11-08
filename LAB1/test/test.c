#include <lib.h>       
#include <unistd.h>   
#include <stdio.h>     


int get_maxchildren(int *pid, int *children_count) {
    message m;
    int result;
    result = _syscall(MM, 78, &m); /* 78 -> GETMAXCHILDREN*/
    *pid = m.m1_i1;
    *children_count = m.m1_i2;
    return 0;
}

int main() {
    int pid, children_count;
    int result;
    result = get_maxchildren(&pid, &children_count);
    printf("PID -> %d\n", pid);
    printf("Children count -> %d\n", children_count);
    return 0;
}

