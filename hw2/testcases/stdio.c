#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

#define errquit(x) { perror(x); exit(-1); }
#define	TESTFILE "test1.txt"

int main() {
	int i;
	char buf[16];
	FILE *fp;
	srand(time(0) ^ getpid());
	
	if((fp = fopen(TESTFILE, "wt")) == NULL) errquit("fopen");
	for(i = 0; i < 10; i++) fprintf(fp, "%d\n", rand() % 100);
	fclose(fp);

	if((fp = fopen(TESTFILE, "rt")) == NULL) errquit("fopen");
	while((i = fgetc(fp)) != EOF);
	fclose(fp);

	if((fp = fopen(TESTFILE, "rt")) == NULL) errquit("fopen");
	while(fgets(buf, sizeof(buf), fp) != NULL);
	fclose(fp);

	if((fp = fopen(TESTFILE, "rt")) == NULL) errquit("fopen");
	while(fscanf(fp, "%d", &i) > 0);
	fclose(fp);

	unlink(TESTFILE);

	return 0;
}
