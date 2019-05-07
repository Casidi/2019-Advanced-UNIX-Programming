#include "libmini.h"

static jmp_buf jb;

#define PRINT(s) {\
	char* msg = s;\
	write(1, msg, strlen(msg));}

int main() {
	sigset_t set;
	sigemptyset(&set);
	sigaddset(&set, SIGALRM);
	sigprocmask(SIG_BLOCK, &set, NULL);

	volatile int has_jumped = 0;
	PRINT("before setjmp\n");
	if(setjmp(jb) != 0) {
		PRINT("after setjmp\n");
		has_jumped = 1;
		
		sigset_t set_after_jmp;
		sigemptyset(&set_after_jmp);
		sigprocmask(0, NULL, &set_after_jmp);
		if(sigismember(&set_after_jmp, SIGALRM))
			PRINT("PASS: The sigmask is preserved\n")
		else
			PRINT("FAIL: The sigmask is not preserved\n")
	}

	if(has_jumped == 0) {
		PRINT("before longjmp\n");
		longjmp(jb, 1);
		PRINT("after longjmp\n");
	}
	return 0;
}

