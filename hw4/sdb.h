#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include "elftool.h"

typedef struct {
	elf_handle_t* eh;
	elf_strtab_t* strtab;
	char path[128];
	pid_t pid;
} SDB;

SDB* sdb_create() {
	elf_init();
	SDB* sdb = (SDB*)malloc(sizeof(SDB));
	sdb->eh = NULL;
	sdb->strtab = NULL;
	sdb->path[0] = 0;
	sdb->pid = -1;
	return sdb;
}

void sdb_load(SDB* sdb, char* new_filename) {
	if(sdb->eh) {
		elf_close(sdb->eh);
		sdb->eh = NULL;
	}

	if((sdb->eh = elf_open(new_filename)) == NULL) {
		fprintf(stderr, "** unabel to open '%s'.\n", new_filename);
		return;
	}

	if(elf_load_all(sdb->eh) < 0) {
		fprintf(stderr, "** unable to load '%s.\n", new_filename);
		return;
	}

	for(sdb->strtab = (sdb->eh)->strtab; sdb->strtab != NULL; sdb->strtab = (sdb->strtab)->next) {
		if((sdb->strtab)->id == (sdb->eh)->shstrndx) 
			break;
	}
	if(sdb->strtab == NULL) {
		fprintf(stderr, "** section header string table not found.\n");
		return;
	}

	for(int i = 0; i < (sdb->eh)->shnum; i++) {
		if(strcmp(&(sdb->strtab)->data[(sdb->eh)->shdr[i].name], ".text") == 0) {
			printf("** program '%s' loaded. entry point 0x%lx, "
				"vaddr 0x%llx, offset 0x%llx, size 0x%llx\n",
				new_filename, (sdb->eh)->entrypoint, (sdb->eh)->shdr[i].addr, 
				(sdb->eh)->shdr[i].offset, (sdb->eh)->shdr[i].size);
			break;
		}
	}

	sdb->pid = -1;
	strcpy(sdb->path, new_filename);
}

void print_vm_maps(char* line) {
	char* token = strtok(line, " \n");
	int count = 0;
	while(token != NULL) {
		switch(count) {
		case 0: {
			char *second = strchr(token, '-');
			*second = 0;
			second++;
			unsigned long long begin, end;
			sscanf(token, "%llx", &begin);
			sscanf(second, "%llx", &end);
			printf("%016llx-%016llx ", begin, end);
			break;
		}
		case 1:
			token[3] = 0;
			printf("%s ", token);
			break;
		case 2: {
			unsigned int offset;
			sscanf(token, "%x", &offset);
			printf("%-8x ", offset);
			break;
		}
		case 5:
			printf("%s ", token);
			break;
		default:
			break;
		}
		count++;
		token = strtok(NULL, " \n");
	}
	puts("");
}

void sdb_vmmap(SDB* sdb) {
	if(strcmp(sdb->path, "") == 0) {
		printf("No program loaded\n");
		return;
	} else {
		elf_handle_t *eh = sdb->eh;
		elf_strtab_t *tab = sdb->strtab;
		if(sdb->pid == -1) {
			//not running
			for(int i = 0; i < eh->shnum; i++) {
				if(strcmp(&tab->data[eh->shdr[i].name], ".text") == 0) {
					printf("%016llx-%016llx r-x %llx       %s\n",
						eh->shdr[i].addr, eh->shdr[i].addr+eh->shdr[i].size,
						eh->shdr[i].offset, sdb->path);
					break;
				}
			}
		} else {
			char proc_info_path[64];
			char* line = NULL;
			size_t len = 0;
			sprintf(proc_info_path, "/proc/%d/maps", sdb->pid);
			FILE* fp = fopen(proc_info_path, "r");
			while(getline(&line, &len, fp) != -1) {
				print_vm_maps(line);
			}
			if(line)
				free(line);
			fclose(fp);
		}
	}
}

void sdb_start(SDB* sdb) {
	if(strcmp(sdb->path, "") == 0) {
		printf("No program loaded\n");
		return;
	}

	if((sdb->pid = fork()) < 0) {
		perror("Failed to fork");
		return;
	}
	if(sdb->pid == 0) {
		if(ptrace(PTRACE_TRACEME, 0,0,0) < 0) {
			perror("ptrace failed");
			exit(-1);
		}
		execlp(sdb->path, sdb->path, NULL);
		printf("'%s'\n", sdb->path);
		perror("execlp");
		exit(-1);
	} else {
		int status;
		if(waitpid(sdb->pid, &status, 0) < 0) {
			printf("waitpid failed\n");
			return;
		}
		ptrace(PTRACE_SETOPTIONS, sdb->pid, 0, PTRACE_O_EXITKILL);
		printf("** pid %d\n", sdb->pid);
	}
}

void sdb_get(SDB* sdb, char* reg_name) {
	struct user_regs_struct regs;
	ptrace(PTRACE_GETREGS, sdb->pid, 0, &regs);
	printf("%s = %llu (0x%llx)\n", reg_name, regs.rip, regs.rip);
}

void sdb_getregs(SDB* sdb) {
	struct user_regs_struct regs;
	ptrace(PTRACE_GETREGS, sdb->pid, 0, &regs);
	printf("RAX %-18llxRBX %-18llxRCX %-18llxRDX %-18llx\n", regs.rax, regs.rbx, regs.rcx, regs.rdx);
	printf("R8  %-18llxR9  %-18llxR10 %-18llxR11 %-18llx\n", regs.r8, regs.r9, regs.r10, regs.r11);
	printf("R12 %-18llxR13 %-18llxR14 %-18llxR15 %-18llx\n", regs.r12, regs.r13, regs.r14, regs.r15);
	printf("RDI %-18llxRSI %-18llxRBP %-18llxRSP %-18llx\n", regs.rdi, regs.rsi, regs.rbp, regs.rsp);
	printf("RIP %-18llxFLAGS %016llx\n", regs.rip, regs.eflags);
}

void sdb_run(SDB* sdb) {
	if(strcmp(sdb->path, "") == 0) {
		printf("No program loaded\n");
		return;
	}

	int status;
	if(sdb->pid == -1) {
		if((sdb->pid = fork()) < 0) {
			perror("Failed to fork");
			return;
		}
		if(sdb->pid == 0) {
			if(ptrace(PTRACE_TRACEME, 0,0,0) < 0) {
				perror("ptrace failed");
				exit(-1);
			}
			execlp(sdb->path, sdb->path, NULL);
			printf("'%s'\n", sdb->path);
			perror("execlp");
			exit(-1);
		} else {
			if(waitpid(sdb->pid, &status, 0) < 0) {
				printf("waitpid failed\n");
				return;
			}
			ptrace(PTRACE_SETOPTIONS, sdb->pid, 0, PTRACE_O_EXITKILL);
		}
	} else {
		printf("** program %s is already running.\n", sdb->path);
	}

	ptrace(PTRACE_CONT, sdb->pid, 0, 0);
	waitpid(sdb->pid, &status, 0);
	if(status == 0)
		printf("** child process %d terminated normally (code %d)\n", sdb->pid, status);
	else
		printf("** child process %d exited with error (code %d)\n", sdb->pid, status);
	sdb->pid = -1;
}

void sdb_disasm(SDB* sdb, char* addr) {
	if(strcmp(addr, "") == 0) {
		puts("** no addr is given");
		return;
	}
}
