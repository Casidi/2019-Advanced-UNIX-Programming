#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
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

	strcpy(sdb->path, new_filename);
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
		}
	}
}

void sdb_start() {
}

void sdb_run(SDB* sdb) {
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
		//assert(WIFSTOPPED(status));
		ptrace(PTRACE_SETOPTIONS, sdb->pid, 0, PTRACE_O_EXITKILL);
		ptrace(PTRACE_CONT, sdb->pid, 0, 0);
		waitpid(sdb->pid, &status, 0);
		perror("done");
	}
}
