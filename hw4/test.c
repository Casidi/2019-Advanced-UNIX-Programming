#include "sdb.h"

int main(int argc, char** argv) {
	SDB* sdb = sdb_create();
	sdb_vmmap(sdb);

	puts("\nFirst case...");
	sdb_load(sdb, "sample/hello64");
	sdb_vmmap(sdb);
	sdb_start(sdb);

	puts("\nSecond case...");
	sdb_load(sdb, "sample/guess");
	sdb_vmmap(sdb);
	return 0;
}
