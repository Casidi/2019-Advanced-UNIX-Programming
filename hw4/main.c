#include <stdio.h>
#include <stdlib.h>
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

void load_program(char* filename) {
	elf_handle_t *eh = NULL;
	if((eh = elf_open(filename)) == NULL) {
		fprintf(stderr, "** unabel to open '%s'.\n", filename);
		return;
	}

	if(elf_load_all(eh) < 0) {
		fprintf(stderr, "** unable to load '%s.\n", filename);
		return;
	}

	elf_strtab_t *tab = NULL;
	for(tab = eh->strtab; tab != NULL; tab = tab->next) {
		if(tab->id == eh->shstrndx) 
			break;
	}
	if(tab == NULL) {
		fprintf(stderr, "** section header string table not found.\n");
		return;
	}

	for(int i = 0; i < eh->shnum; i++) {
		if(strcmp(&tab->data[eh->shdr[i].name], ".text") == 0) {
			printf("** program '%s' loaded. entry point 0x%lx, vaddr 0x%llx, offset 0x%llx, size 0x%llx\n",
				filename, eh->entrypoint, eh->shdr[i].addr, eh->shdr[i].offset, eh->shdr[i].size);
			break;
		}
	}
	if(eh) {
		elf_close(eh);
		eh = NULL;
	}
}

int main(int argc, char** argv) {
	size_t line_max = 256;
	char* line = malloc(line_max);
	char command[64];

	elf_init();
	if(argc == 2) {
		load_program(argv[1]);
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
		} else if(strcmp(command, "exit") == 0) {
			break;
		} else if(strcmp(command, "get") == 0) {
		} else if(strcmp(command, "getregs") == 0) {
		} else if(strcmp(command, "help") == 0) {
			printf("%s", help_msg);
		} else if(strcmp(command, "list") == 0) {
		} else if(strcmp(command, "load") == 0) {
			char filename[64];
			sscanf(line, "%s %s", command, filename);
			load_program(filename);
		} else if(strcmp(command, "run") == 0) {
		} else if(strcmp(command, "vmmap") == 0) {
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
