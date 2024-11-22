#include <minix/callnr.h>
#include <lib.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

int main(int argc, char** argv) {
	int pid, priority;
	int set_prio;
	message m;
	if (argc != 2 && argc != 3) {
		printf("Correct usage -> ./{program_name} {pid} {ONLY IF YOU WANT TO SET PRIORITY -> (1-100)}\n");
		return 1;
	}
	pid = atoi(argv[1]);
	m.m1_i1 = pid;
	if (argc == 3){
		set_prio = atoi(argv[2]);
		if (set_prio <= 0 || set_prio > 100) return 1;
		m.m1_i2 = set_prio;
		_syscall(MM, 78, &m); 
	} else {
		_syscall(MM, 79, &m);
		priority = m.m1_i1;
		printf("PID -> %d\n", pid);
		printf("Priority -> %d\n", priority);
	}
	return 0;
}

