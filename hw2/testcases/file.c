#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define errquit(x) { perror(x); exit(-1); }
#define FILENAME1 "./test1.txt"
#define	FILENAME2 "./test2.txt"
#define	DIRNAME	"./dir1"

int main() {
	int fd = creat(FILENAME1, 0644);
	int fd2, fd3;
	char buf[16];
	struct stat st;

	chdir("./././");

	if(fd < 0) errquit("creat");
	write(fd, "ABCD", 4);
	lseek(fd, 4096, SEEK_SET);
	write(fd, "EFGH", 4);
	pwrite(fd, "IJKL", 4, 8192);
	close(fd);

	fd = open(FILENAME1, O_RDONLY);
	if(fd < 0) errquit("open");
	fd2 = dup(fd);
	fd3 = dup2(fd, 1000);
	read(fd2, buf, sizeof(buf));
	pread(fd3, buf, sizeof(buf), 8192);
	close(fd);

	stat(FILENAME1, &st);
	chown(FILENAME1, 0, 0);
	chmod(FILENAME1, 0600);
	lstat(FILENAME1, &st);

	rename(FILENAME1, FILENAME2);
	link(FILENAME2, FILENAME1);
	unlink(FILENAME2);
	symlink(FILENAME1, FILENAME2);
	readlink(FILENAME2, buf, sizeof(buf));

	mkdir(DIRNAME, 0777);
	lstat(DIRNAME, &st);
	rmdir(DIRNAME);

	unlink(FILENAME1);
	remove(FILENAME2);

	return 0;
}

