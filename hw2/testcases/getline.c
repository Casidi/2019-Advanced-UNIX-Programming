#include <stdio.h>
#include <stdlib.h>

int main() {
	char *lineptr = NULL;
	size_t linelen = 0;
	ssize_t rlen;
	FILE *fp = fopen("/etc/passwd", "rt");
	if(fp == NULL) return -1;

	while((rlen = getline(&lineptr, &linelen, fp)) >= 0) {
		if(rlen > 0) fwrite(lineptr, 1, rlen, stdout);
	}

	if(lineptr != NULL) free(lineptr);
	fclose(fp);
	return 0;
}

