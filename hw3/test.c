#include "libmini.h"

void handler(int s) {
	char *msg = "inside handler\n";
	write(1, msg, strlen(msg));
}

int main(int argc, char** argv) {
	char *msg = "test hello!!!\n";
	write(1, msg, strlen(msg));
	signal(SIGALRM, handler);
	write(1, msg, strlen(msg));
	alarm(2);
	write(1, msg, strlen(msg));
	pause();
	write(1, msg, strlen(msg));
	char *msg1 = "after pause\n";
	write(1, msg1, strlen(msg1));
	return 0;
}
