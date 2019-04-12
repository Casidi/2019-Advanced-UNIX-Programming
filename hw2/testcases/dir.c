#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <dirent.h>

int main() {
	DIR* dir = opendir(".");
	struct dirent *d;
	if(dir == NULL) exit(-1);
	while((d = readdir(dir)) != NULL) {
		printf("%s\n", d->d_name);
	}
	closedir(dir);
	return 0;
}

