#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <string.h>
#include "elftool.h"

char* help_msg = "- break {instruction-address}: add a break point\n"
	"- cont: continue execution\n"
	"- delete {break-point-id}: remove a break point\n"
	"- disasm addr: disassemble instructions in a file or a memory region\n"
	"- dump addr [length]: dump memory content\n"
	"- exit: terminate the debugger\n"
	"- get reg: get a single value from a register\n"
	"- getregs: show registers\n"
	"- help: show this message\n"
	"- list: list break points\n"
	"- load {path/to/a/program}: load a program\n"
	"- run: run the program\n"
	"- vmmap: show memory layout\n"
	"- set reg val: get a single value to a register\n"
	"- si: step into instruction\n"
	"- start: start the program and stop at the first instruction\n";

char filename[64] = "";

void load_program(char* new_filename, elf_handle_t** eh, elf_strtab_t** tab) {
	if(*eh) {
		elf_close(*eh);
		*eh = NULL;
	}

	if((*eh = elf_open(new_filename)) == NULL) {
		fprintf(stderr, "** unabel to open '%s'.\n", new_filename);
		return;
	}

	if(elf_load_all(*eh) < 0) {
		fprintf(stderr, "** unable to load '%s.\n", new_filename);
		return;
	}

	for(*tab = (*eh)->strtab; *tab != NULL; *tab = (*tab)->next) {
		if((*tab)->id == (*eh)->shstrndx) 
			break;
	}
	if(*tab == NULL) {
		fprintf(stderr, "** section header string table not found.\n");
		return;
	}

	for(int i = 0; i < (*eh)->shnum; i++) {
		if(strcmp(&(*tab)->data[(*eh)->shdr[i].name], ".text") == 0) {
			printf("** program '%s' loaded. entry point 0x%lx, "
				"vaddr 0x%llx, offset 0x%llx, size 0x%llx\n",
				filename, (*eh)->entrypoint, (*eh)->shdr[i].addr, 
				(*eh)->shdr[i].offset, (*eh)->shdr[i].size);
			break;
		}
	}

	strcpy(filename, new_filename);
}

int main(int argc, char** argv) {
	size_t line_max = 256;
	char* line = malloc(line_max);
	char command[64];
	pid_t pid_tracee = -1;
	elf_handle_t *eh = NULL;
	elf_strtab_t *tab = NULL;

	elf_init();
	if(argc == 2) {
		load_program(argv[1], &eh, &tab);
	}

	while(1) {
		printf("sdb> ");
		getline(&line, &line_max, stdin);
		sscanf(line, "%s", command);
		if(strcmp(command, "break") == 0) {
		} else if(strcmp(command, "cont") == 0) {
		} else if(strcmp(command, "delete") == 0) {
		} else if(strcmp(command, "disasm") == 0) {
		} else if(strcmp(command, "dum") == 0) {
		} else if(strcmp(command, "exit") == 0 || strcmp(command, "q") == 0) {
			break;
		} else if(strcmp(command, "get") == 0) {
		} else if(strcmp(command, "getregs") == 0) {
		} else if(strcmp(command, "help") == 0 || strcmp(command, "h") == 0) {
			printf("%s", help_msg);
		} else if(strcmp(command, "list") == 0) {
		} else if(strcmp(command, "load") == 0) {
			char new_filename[64];
			sscanf(line, "%s %s", command, new_filename);
			load_program(new_filename, &eh, &tab);
		} else if(strcmp(command, "run") == 0) {
			if(strcmp(filename, "") == 0) {
				printf("No program loaded\n");
				continue;
			}

			if((pid_tracee = fork()) < 0) {
				perror("Failed to fork");
				continue;
			}
			if(pid_tracee == 0) {
				if(ptrace(PTRACE_TRACEME, 0,0,0) < 0) {
					perror("ptrace failed");
					return -1;
				}
				execlp(filename, filename, NULL);
				printf("'%s'\n", filename);
				perror("execlp");
				return -1;
			} else {
				int status;
				if(waitpid(pid_tracee, &status, 0) < 0) {
					printf("waitpid failed\n");
					continue;
				}
				//assert(WIFSTOPPED(status));
				ptrace(PTRACE_SETOPTIONS, pid_tracee, 0, PTRACE_O_EXITKILL);
				ptrace(PTRACE_CONT, pid_tracee, 0, 0);
				waitpid(pid_tracee, &status, 0);
				perror("done");
			}
		} else if(strcmp(command, "vmmap") == 0) {
			if(strcmp(filename, "") == 0) {
				printf("No program loaded\n");
				continue;
			} else {
				if(pid_tracee == -1) {
					//not running
					for(int i = 0; i < eh->shnum; i++) {
						if(strcmp(&tab->data[eh->shdr[i].name], ".text") == 0) {
							printf("%016llx-%016llx r-x %llx       %s\n",
								eh->shdr[i].addr, eh->shdr[i].addr+eh->shdr[i].size,
								eh->shdr[i].offset, filename);
							break;
						}
					}
				} else {
				}
			}
		} else if(strcmp(command, "set") == 0) {
		} else if(strcmp(command, "si") == 0) {
		} else if(strcmp(command, "start") == 0) {
		} else {
			printf("Unknown command: %s\n", command);
		}
	}

	free(line);
	return 0;
}
