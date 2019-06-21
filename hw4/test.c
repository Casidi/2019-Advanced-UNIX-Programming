#include "sdb.h"

int main(int argc, char** argv) {
	SDB* sdb = sdb_create();
	sdb_vmmap(sdb);

	puts("\nTest Case 1...");
	sdb_load(sdb, "sample/hello64");
	sdb_vmmap(sdb);
	sdb_start(sdb);
	sdb_vmmap(sdb);
	sdb_get(sdb, "rip");
	sdb_run(sdb);

	puts("\nTest Case 2...");
	sdb_load(sdb, "sample/hello64");
	sdb_start(sdb);
	sdb_getregs(sdb);

	puts("\nTest Case 3...");
	sdb_load(sdb, "sample/hello64");
	sdb_disasm(sdb, "");
	sdb_disasm(sdb, "0x4000b0");
	sdb_start(sdb);
	sdb_set_break(sdb, "0x4000c6");
	sdb_disasm(sdb, "0x4000c6");
	sdb_dump(sdb, "0x4000c6");

	puts("\nTest Case 4...");
	sdb_load(sdb, "sample/hello64");
	sdb_disasm(sdb, "0x4000b0");
	sdb_set_break(sdb, "0x4000c6");
	sdb_list(sdb);
	sdb_run(sdb);
	sdb_set(sdb, "rip", 0x4000b0);
	sdb_cont(sdb);
	sdb_delete(sdb, 0);
	sdb_set(sdb, "rip", 0x4000b0);
	sdb_cont(sdb);

	puts("\nTest Case 5...");
	sdb_load(sdb, "sample/guess");
	sdb_vmmap(sdb);
	sdb_disasm(sdb, "0x985");
	sdb_disasm(sdb, "");
	sdb_set_break(sdb, "0x9cc");
	sdb_start(sdb);
	sdb_vmmap(sdb);
	sdb_cont(sdb);
	sdb_get(sdb, "rax");
	unsigned long long val = sdb_get(sdb, "rdx");
	sdb_set(sdb, "rax", val);
	sdb_cont(sdb);

	puts("\nTest Case 6...Test step command");
	sdb_load(sdb, "sample/hello64");
	sdb_start(sdb);
	for(int i = 0; i < 10; ++i)
		sdb_step(sdb);

	return 0;
}
