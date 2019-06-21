#include "sdb.h"

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

int main(int argc, char** argv) {
	char* line = NULL;
	char command[64];
	size_t line_max = 0;

	SDB* sdb = sdb_create();

	if(argc == 2) {
		sdb_load(sdb, argv[1]);
	}

	while(1) {
		printf("sdb> ");
		getline((char**)&line, &line_max, stdin);
		sscanf(line, "%s", command);
		if(strcmp(command, "break") == 0 || strcmp(command, "b") == 0) {
			char bp_addr[16];
			sscanf(line, "%s %s", command, bp_addr);
			sdb_set_break(sdb, bp_addr);
		} else if(strcmp(command, "cont") == 0 || strcmp(command, "c") == 0) {
			sdb_cont(sdb);
		} else if(strcmp(command, "delete") == 0) {
			int index;
			sscanf(line, "%s %d", command, &index);
			sdb_delete(sdb, index);
		} else if(strcmp(command, "disasm") == 0 || strcmp(command, "d") == 0) {
			char addr[16] = {0};
			sscanf(line, "%s %s", command, addr);
			sdb_disasm(sdb, addr);
		} else if(strcmp(command, "dump") == 0) {
			char addr[16];
			sscanf(line, "%s %s", command, addr);
			sdb_dump(sdb, addr);
		} else if(strcmp(command, "exit") == 0 || strcmp(command, "q") == 0) {
			break;
		} else if(strcmp(command, "get") == 0 || strcmp(command, "g") == 0) {
			char oprand[16];
			sscanf(line, "%s %s", command, oprand);
			sdb_get(sdb, oprand);
		} else if(strcmp(command, "getregs") == 0) {
			sdb_getregs(sdb);
		} else if(strcmp(command, "help") == 0 || strcmp(command, "h") == 0) {
			printf("%s", help_msg);
		} else if(strcmp(command, "list") == 0 || strcmp(command, "l") == 0) {
			sdb_list(sdb);
		} else if(strcmp(command, "load") == 0) {
			char new_filename[64];
			sscanf(line, "%s %s", command, new_filename);
			sdb_load(sdb, new_filename);
		} else if(strcmp(command, "run") == 0 || strcmp(command, "r") == 0) {
			sdb_run(sdb);
		} else if(strcmp(command, "vmmap") == 0 || strcmp(command, "m") == 0) {
			sdb_vmmap(sdb);
		} else if(strcmp(command, "set") == 0 || strcmp(command, "s") == 0) {
			char oprand1[16];
			char oprand2[16];
			unsigned long long val;
			sscanf(line, "%s %s %s", command, oprand1, oprand2);
			if(oprand2[0] == '0' && oprand2[1] == 'x')
				sscanf(oprand2+2, "%llx", &val);
			else
				sscanf(oprand2, "%llu", &val);
			sdb_set(sdb, oprand1, val);
		} else if(strcmp(command, "si") == 0) {
			sdb_step(sdb);
		} else if(strcmp(command, "start") == 0) {
			sdb_start(sdb);
		} else {
			printf("Unknown command: %s\n", command);
		}
	}

	return 0;
}
