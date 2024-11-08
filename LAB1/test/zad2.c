#include <minix/callnr.h>
#include <lib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	int pid_to_skip, max_descendants;
	int result, pid, descendants_count;
	message m;
	if (argc != 2) {
		printf("Correct usage -> ./{program_name} pid\n");
		return 1;
	}
	pid_to_skip = atoi(argv[1]);
	m.m1_i1 = pid_to_skip;
	result = _syscall(MM, 79, &m);
	pid = m.m1_i1;
        descendants_count = m.m1_i2;
        printf("PID -> %d\n", pid);
        printf("Descendants count -> %d\n", descendants_count);
	return 0;
}
