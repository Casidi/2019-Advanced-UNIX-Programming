#include <stdio.h>

int main() {
	int value;
	fprintf(stdout, "Value: ");
	fscanf(stdin, "%d", &value);
	fprintf(stdout, "Value [%d]\n", value);
	return 0;
}

