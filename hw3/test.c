#include "libmini.h"

int main(int argc, char** argv) {
	char *msg = "test hello!!!\n";
	write(1, msg, strlen(msg));
	return 0;
}
