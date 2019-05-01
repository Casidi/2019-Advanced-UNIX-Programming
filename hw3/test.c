#include "libmini.h"

void handler(int s) {
	char *msg = "inside handler\n";
	write(1, msg, strlen(msg));
}

int main(int argc, char** argv) {
	char *msg = "test hello!!!\n";
	write(1, msg, strlen(msg));
	signal(SIGALRM, handler);
	alarm(2);
	pause();
	char *msg1 = "after pause\n";
	write(1, msg1, strlen(msg1));
	return 0;
}
