#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <ctype.h>
#include <libgen.h>
#include <sys/ptrace.h>
#include <sys/wait.h>
#include <sys/user.h>
#include <capstone/capstone.h>
#include "elftool.h"

#define MAX_BREAK 10

typedef struct {
	int is_used;
	unsigned long long addr;
	unsigned long long original_word;
} bp_t;

typedef struct {
	elf_handle_t* eh;
	elf_strtab_t* strtab;
	csh cshandle;
	char path[128];
	pid_t pid;
	bp_t breaks[MAX_BREAK];
	unsigned long long last_disasm_addr;
} SDB;

unsigned long long convert_dynamic_addr(SDB* sdb, unsigned long long addr) {
	char proc_info_path[64];
	char* line = NULL;
	size_t len = 0;
	sprintf(proc_info_path, "/proc/%d/maps", sdb->pid);
	FILE* fp = fopen(proc_info_path, "r");
	int is_executable = 0;
	int is_exe_area = 0;
	unsigned long long base_addr;
	while(getline(&line, &len, fp) != -1) {
		char* token = strtok(line, " \n");
		int count = 0;
		is_executable = 0;
		is_exe_area = 0;
		while(token != NULL) {
			switch(count) {
			case 0: {
				char *second = strchr(token, '-');
				*second = 0;
				second++;
				unsigned long long begin, end;
				sscanf(token, "%llx", &begin);
				sscanf(second, "%llx", &end);
				//printf("%016llx-%016llx ", begin, end);
				base_addr = begin;
				break;
			}
			case 1:
				token[3] = 0;
				if(strcmp(token, "r-x") == 0)
					is_executable = 1;
				break;
			case 2: {
				unsigned int offset;
				sscanf(token, "%x", &offset);
				break;
			}
			case 5:
				if(strcmp(basename(token), basename(sdb->path)) == 0)
					is_exe_area = 1;
				break;
			default:
				break;
			}

			count++;
			token = strtok(NULL, " \n");
		}
		if(is_executable && is_exe_area) {
			break;
		}
	}
	if(line)
		free(line);
	fclose(fp);

	return base_addr + addr;
}

SDB* sdb_create() {
	SDB* sdb = (SDB*)malloc(sizeof(SDB));

	elf_init();
	cs_open(CS_ARCH_X86, CS_MODE_64, &(sdb->cshandle));
	sdb->eh = NULL;
	sdb->strtab = NULL;
	sdb->path[0] = 0;
	sdb->pid = -1;

	int i;
	for(i = 0; i < MAX_BREAK; ++i)
		sdb->breaks[i].is_used = 0;
	sdb->last_disasm_addr = 0;

	return sdb;
}

int sdb_is_loaded(SDB* sdb) {
	return strcmp(sdb->path, "") != 0;
}

int sdb_is_running(SDB* sdb) {
	return sdb->pid != -1;
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

	for(int i = 1; i < sdb->eh->shnum; i++) {
		if(strcmp(&(sdb->strtab->data[sdb->eh->shdr[i].name]), ".text") == 0) {
			printf("** program '%s' loaded. entry point 0x%lx, "
				"vaddr 0x%llx, offset 0x%llx, size 0x%llx\n",
				new_filename, (sdb->eh)->entrypoint, (sdb->eh)->shdr[i].addr, 
				(sdb->eh)->shdr[i].offset, (sdb->eh)->shdr[i].size);
			break;
		}
	}

	sdb->pid = -1;
	strcpy(sdb->path, new_filename);
	int i;
	for(i = 0; i < MAX_BREAK; ++i)
		sdb->breaks[i].is_used = 0;
	sdb->last_disasm_addr = 0;
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
	if(!sdb_is_loaded(sdb)) {
		printf("No program loaded\n");
		return;
	} else {
		elf_handle_t *eh = sdb->eh;
		elf_strtab_t *tab = sdb->strtab;
		if(!sdb_is_running(sdb)) {
			//not running
			for(int i = 1; i < eh->shnum; i++) {
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

#define MATCH_REG_NAME(r) if(strcmp(reg_name, #r) == 0) reg_val = regs.r;
unsigned long long sdb_get(SDB* sdb, char* reg_name) {
	struct user_regs_struct regs;
	ptrace(PTRACE_GETREGS, sdb->pid, 0, &regs);

	unsigned long long reg_val;
	MATCH_REG_NAME(rax);
	MATCH_REG_NAME(rbx);
	MATCH_REG_NAME(rcx);
	MATCH_REG_NAME(rdx);
	MATCH_REG_NAME(r8);
	MATCH_REG_NAME(r9);
	MATCH_REG_NAME(r10);
	MATCH_REG_NAME(r11);
	MATCH_REG_NAME(r12);
	MATCH_REG_NAME(r13);
	MATCH_REG_NAME(r14);
	MATCH_REG_NAME(r15);
	MATCH_REG_NAME(rdi);
	MATCH_REG_NAME(rsi);
	MATCH_REG_NAME(rbp);
	MATCH_REG_NAME(rsp);
	MATCH_REG_NAME(rip);
	MATCH_REG_NAME(eflags);
	printf("%s = %llu (0x%llx)\n", reg_name, reg_val, reg_val);
	return reg_val;
}


void sdb_patch_all_bp(SDB* sdb) {
	int i;
	for(i = 0; i < MAX_BREAK; ++i) {
		bp_t* bp = &(sdb->breaks[i]);
		if(bp->is_used) {
			//printf("type = %d\n", sdb->eh->type);
			unsigned long long addr = bp->addr;
			if(sdb->eh->type == 3) {
				addr = convert_dynamic_addr(sdb, addr);
			}

			struct user_regs_struct regs;
			ptrace(PTRACE_GETREGS, sdb->pid, 0, &regs);
			if(regs.rip == addr) {
				ptrace(PTRACE_SINGLESTEP, sdb->pid, 0,0);
				if(waitpid(sdb->pid, 0, 0) < 0)
					puts("Failed to wait");
			}

			unsigned long long word = ptrace(PTRACE_PEEKTEXT, sdb->pid, addr, 0);
			if ((word & 0xff) == 0xcc)
				continue;
			bp->original_word = word;

			if(ptrace(PTRACE_POKETEXT, sdb->pid, addr, 
				(bp->original_word & 0xffffffffffffff00) | 0xcc) != 0) {
				puts("failed to patch the code");
				return;
			}
		}
	}
}

void sdb_start(SDB* sdb) {
	if(!sdb_is_loaded(sdb)) {
		printf("No program loaded\n");
		return;
	}

	int status;
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
		printf("** pid %d\n", sdb->pid);
	}

	sdb_patch_all_bp(sdb);
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

void sdb_cont(SDB* sdb) {
	if(!sdb_is_running(sdb)) {
		puts("not running");
		return;
	}
	
	sdb_patch_all_bp(sdb);

	int status;
	ptrace(PTRACE_CONT, sdb->pid, 0, 0);
	while(waitpid(sdb->pid, &status, 0) > 0) {
		if(!WIFSTOPPED(status))
			continue;
		struct user_regs_struct regs;
		ptrace(PTRACE_GETREGS, sdb->pid, 0, &regs);

		int i;
		for(i = 0; i < MAX_BREAK; ++i) {
			bp_t* bp = &(sdb->breaks[i]);
			unsigned long long vaddr = bp->addr;
			if(sdb->eh->type == 3)
				vaddr = convert_dynamic_addr(sdb, vaddr);
			if(bp->is_used && vaddr == (regs.rip-1)) {
				ptrace(PTRACE_POKETEXT, sdb->pid, vaddr, bp->original_word);
				regs.rip--;
				ptrace(PTRACE_SETREGS, sdb->pid, 0, &regs);

				unsigned long long buffer;
				buffer = ptrace(PTRACE_PEEKTEXT, sdb->pid, vaddr, 0);
				cs_insn *insn;
				size_t count;
				if((count = cs_disasm(sdb->cshandle, (uint8_t*)&buffer, 8, vaddr, 0, &insn)) > 0) {
					char bytes_str[16] = "";
					char byte[8];
					for(int j = 0; j < insn[0].size; ++j) {
						sprintf(byte, "%02x ", insn[0].bytes[j]);
						strcat(bytes_str, byte);
					}

					printf("** breakpoint @ %10lx: %-21s", insn[0].address, bytes_str);
					printf("%-7s%s\n", insn[0].mnemonic, insn[i].op_str);
					cs_free(insn, count);
				} else {
					printf("ERROR: Failed to disassemble given code!\n");
				}

				return;
			}
		}
	}

	if(status == 0)
		printf("** child process %d terminated normally (code %d)\n", sdb->pid, status);
	else
		printf("** child process %d exited with error (code %d)\n", sdb->pid, status);
	sdb->pid = -1;
}

void sdb_run(SDB* sdb) {
	if(!sdb_is_loaded(sdb)) {
		printf("No program loaded\n");
		return;
	}

	int status;
	if(!sdb_is_running(sdb)) {
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
			printf("** pid %d\n", sdb->pid);
		}
	} else {
		printf("** program %s is already running.\n", sdb->path);
	}

	sdb_patch_all_bp(sdb);

	ptrace(PTRACE_CONT, sdb->pid, 0, 0);
	while(waitpid(sdb->pid, &status, 0) > 0) {
		if(!WIFSTOPPED(status))
			continue;
		struct user_regs_struct regs;
		ptrace(PTRACE_GETREGS, sdb->pid, 0, &regs);

		int i;
		for(i = 0; i < MAX_BREAK; ++i) {
			bp_t* bp = &(sdb->breaks[i]);
			unsigned long long vaddr = bp->addr;
			if(sdb->eh->type == 3)
				vaddr = convert_dynamic_addr(sdb, vaddr);

			if(bp->is_used && vaddr == (regs.rip-1)) {
				ptrace(PTRACE_POKETEXT, sdb->pid, vaddr, bp->original_word);
				regs.rip--;
				ptrace(PTRACE_SETREGS, sdb->pid, 0, &regs);

				unsigned long long buffer;
				buffer = ptrace(PTRACE_PEEKTEXT, sdb->pid, vaddr, 0);
				cs_insn *insn;
				size_t count;
				if((count = cs_disasm(sdb->cshandle, (uint8_t*)&buffer, 8, vaddr, 0, &insn)) > 0) {
					char bytes_str[16] = "";
					char byte[8];
					for(int j = 0; j < insn[0].size; ++j) {
						sprintf(byte, "%02x ", insn[0].bytes[j]);
						strcat(bytes_str, byte);
					}

					printf("** breakpoint @ %10lx: %-21s", insn[0].address, bytes_str);
					printf("%-7s%s\n", insn[0].mnemonic, insn[i].op_str);
					cs_free(insn, count);
				} else {
					printf("ERROR: Failed to disassemble given code!\n");
				}
				return;
			}
		}
	}

	if(status == 0)
		printf("** child process %d terminated normally (code %d)\n", sdb->pid, status);
	else
		printf("** child process %d exited with error (code %d)\n", sdb->pid, status);
	sdb->pid = -1;
}

#define SET_IF_MATCHED(r) if(strcmp(reg_name, #r) == 0) regs.r = val;
void sdb_set(SDB* sdb, char* reg_name, unsigned long long val) {
	struct user_regs_struct regs;
	ptrace(PTRACE_GETREGS, sdb->pid, 0, &regs);
	SET_IF_MATCHED(rax);
	SET_IF_MATCHED(rbx);
	SET_IF_MATCHED(rcx);
	SET_IF_MATCHED(rdx);
	SET_IF_MATCHED(r8);
	SET_IF_MATCHED(r9);
	SET_IF_MATCHED(r10);
	SET_IF_MATCHED(r11);
	SET_IF_MATCHED(r12);
	SET_IF_MATCHED(r13);
	SET_IF_MATCHED(r14);
	SET_IF_MATCHED(r15);
	SET_IF_MATCHED(rdi);
	SET_IF_MATCHED(rsi);
	SET_IF_MATCHED(rbp);
	SET_IF_MATCHED(rsp);
	SET_IF_MATCHED(rip);
	SET_IF_MATCHED(eflags);
	if(ptrace(PTRACE_SETREGS, sdb->pid, 0, &regs) != 0) {
		puts("Failed to set regs");
		return;
	}
}

void sdb_disasm(SDB* sdb, char* addr_str) {
	unsigned long long addr;
	puts(addr_str);
	if(strcmp(addr_str, "") == 0) {
		if (sdb->last_disasm_addr == 0x0) {
			puts("** no addr is given");
			return;
		} else {
			addr = sdb->last_disasm_addr;
		}
	} else {
		if(addr_str[1] == 'x')
			addr_str += 2;
		sscanf(addr_str, "%llx", &addr);
		sdb->last_disasm_addr = addr;
	}

	char buffer[64] = {0};
	unsigned long long ptr;
	if(!sdb_is_running(sdb)) {
		//not running, read from file

		// find that the addr is located in which section 
		elf_handle_t *eh = sdb->eh;
		size_t file_offset;
		int is_valid_addr = 0;
		for(int i = 0; i < eh->shnum; i++) {
			if(addr >= eh->shdr[i].addr && addr < eh->shdr[i].addr + eh->shdr[i].size) {
				file_offset = eh->shdr[i].offset + addr - eh->shdr[i].addr;
				is_valid_addr = 1;
				break;
			}
		}
		if(!is_valid_addr) {
			printf("Invalid addr!\n");
			return;
		}

		ptr = addr;
		FILE* fp = fopen(sdb->path, "rb");
		fseek(fp, file_offset, SEEK_SET); 
		ptr += fread(buffer, 1, sizeof(buffer), fp);
		fclose(fp);
	} else {
		//running
		//restore changes caused by break points
		int i;
		for(i = 0; i < MAX_BREAK; ++i) {
			bp_t* bp = &(sdb->breaks[i]);
			if(bp->is_used) {
				if(ptrace(PTRACE_POKETEXT, sdb->pid, bp->addr, 
					bp->original_word) != 0) {
					puts("failed to patch the code");
				}
			}
		}

		for(ptr = addr; ptr < addr + sizeof(buffer); ptr += 8) {
			unsigned long long peek;
			errno = 0;
			peek = ptrace(PTRACE_PEEKTEXT, sdb->pid, ptr, NULL);
			if(errno != 0)
				break;
			memcpy(&buffer[ptr-addr], &peek, 8);
		}

		sdb_patch_all_bp(sdb);
	}

	cs_insn *insn;
	size_t count;
	if((count = cs_disasm(sdb->cshandle, (uint8_t*)buffer, ptr-addr, addr, 0, &insn)) > 0) {
		int i;
		for(i = 0; i < count && i < 10; ++i) {
			char bytes_str[16] = "";
			char byte[8];
			for(int j = 0; j < insn[i].size; ++j) {
				sprintf(byte, "%02x ", insn[i].bytes[j]);
				strcat(bytes_str, byte);
			}

			sdb->last_disasm_addr += insn[i].size;
			printf("%10lx: %-21s", insn[i].address, bytes_str);
			printf("%-7s%s\n", insn[i].mnemonic, insn[i].op_str);
		}
		cs_free(insn, count);
	} else {
		printf("ERROR: Failed to disassemble given code!\n");
	}
}

void sdb_dump(SDB* sdb, char* addr_str) {
	if(strcmp(addr_str, "") == 0) {
		puts("** no addr is given");
		return;
	}
	if(!sdb_is_running(sdb)) {
		puts("not running");
		return;
	}

	if(addr_str[1] == 'x')
		addr_str += 2;
	unsigned long long addr;
	sscanf(addr_str, "%llx", &addr);

	unsigned long long ptr;
	unsigned char buffer[80];
	for(ptr = addr; ptr < addr + sizeof(buffer); ptr += 8) {
		unsigned long long peek;
		errno = 0;
		peek = ptrace(PTRACE_PEEKTEXT, sdb->pid, ptr, NULL);
		if(errno != 0)
			break;
		memcpy(&buffer[ptr-addr], &peek, 8);
	}

	int i, j;
	for(i = 0; i < 5; ++i) {
		printf("%10llx:", addr+i*16);
		for(j = 0; j < 16; ++j) {
			unsigned char c = buffer[i*16+j];
			printf(" %02x", c);
		}
		printf("  |");
		for(j = 0; j < 16; ++j) {
			char c = buffer[i*16+j];
			if(isprint(c))
				printf("%c", c);
			else
				printf(".");
		}
		printf("|\n");
	}
}


void sdb_set_break(SDB* sdb, char* addr_str) {
	if(!sdb_is_loaded(sdb)) {
		puts("No program loaded");
		return;
	}

	if(addr_str[1] == 'x')
		addr_str += 2;
	unsigned long long addr;
	sscanf(addr_str, "%llx", &addr);


	int i;
	bp_t* bp;
	for(i = 0; i < MAX_BREAK; ++i) {
		if(!(sdb->breaks[i].is_used)) {
			bp = &(sdb->breaks[i]);
			break;
		}
	}

	bp->is_used = 1;
	bp->addr = addr;

	if(sdb_is_running(sdb))
		sdb_patch_all_bp(sdb);
}

void sdb_list(SDB* sdb) {
	int i;
	for(i = 0; i < MAX_BREAK; ++i) {
		bp_t* bp = &(sdb->breaks[i]);
		if(bp->is_used)
			printf("%4d: %llx\n", i, bp->addr);
	}
}

void sdb_delete(SDB* sdb, int index) {
	if(index >= 0 && index < MAX_BREAK && sdb->breaks[index].is_used) {
		bp_t* bp = &(sdb->breaks[index]);
		unsigned long long word = ptrace(PTRACE_PEEKTEXT, sdb->pid, bp->addr, 0);
		if ((word & 0xff) == 0xcc) {
			ptrace(PTRACE_POKETEXT, sdb->pid, bp->addr, bp->original_word);
		}

		bp->is_used = 0;
		printf("** breakpoint %d deleted.\n", index);
	}
}

void sdb_step(SDB* sdb) {
	if(!sdb_is_running(sdb)) {
		puts("Not running");
		return;
	}

	if(ptrace(PTRACE_SINGLESTEP, sdb->pid, 0,0) < 0) {
		puts("Step failed");
		return;
	}
	if(waitpid(sdb->pid, 0, 0) < 0)
		puts("Failed to wait");
}
