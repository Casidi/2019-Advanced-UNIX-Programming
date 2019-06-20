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

	return 0;
}
