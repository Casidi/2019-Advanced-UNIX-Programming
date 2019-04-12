#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <dirent.h>

void* load_function(const char* func_name) {
	void *handle = dlopen("libc.so.6", RTLD_LAZY);
	void *func = NULL;
	if(handle == NULL)
		return NULL;
	func = dlsym(handle, func_name);
	if(func == NULL)
		fprintf(stderr, "Failed to load function: %s\n", func_name);
	return func;
}

void write_msg(const char* msg, ...) {
	va_list vl;
	va_start(vl, msg);

	/*char* log_name = getenv("MONITOR_OUTPUT");
	if(log_name != NULL) {
		old_fopen = load_function("fopen");
		old_fclose = load_function("fclose");
		FILE *fp = old_fopen(log_name, "a");

		vfprintf(fp, msg, vl);
		old_fclose(fp);
	} else {
		vfprintf(stderr, msg, vl);
	}*/

	//for testing
	vfprintf(stderr, msg, vl);
	
	va_end(vl);
}

#define BEGIN_MON1(func,PT1,RT)\
	static RT (*old_##func)(PT1);\
	RT func(PT1 p1) {\
	old_##func = load_function(#func);\
	RT ret = old_##func(p1);
#define BEGIN_MON2(func,PT1,PT2,RT)\
	static RT (*old_##func)(PT1,PT2);\
	RT func(PT1 p1, PT2 p2) {\
	old_##func = load_function(#func);\
	RT ret = old_##func(p1, p2);
#define BEGIN_MON3(func,PT1,PT2,PT3,RT)\
	static RT (*old_##func)(PT1,PT2,PT3);\
	RT func(PT1 p1, PT2 p2, PT3 p3) {\
	old_##func = load_function(#func);\
	RT ret = old_##func(p1,p2,p3);
#define END_MON return ret;}

BEGIN_MON1(closedir, DIR*, int)
	write_msg("closedir(0x%X) = %d\n", p1, ret);
END_MON

BEGIN_MON1(opendir, const char*, DIR*)
	write_msg("opendir(\"%s\") = 0x%X\n", p1, ret);
END_MON

BEGIN_MON1(readdir, DIR*, struct dirent*)
	//write_msg("readdir(\"%s\") = %s\n", dp->ent->d_name, ret->d_name); // not work
	//fprintf(stderr, "d_name = %s\n", ret->d_name);
	//write_msg("readdir(\"%X\") = %s\n", p1, ret->d_name); // not work
	write_msg("readdir() = ?\n");
END_MON

BEGIN_MON2(creat, const char*, mode_t, int)
	write_msg("creat(\"%s\", %d) = %d\n", p1, p2, ret);
END_MON

BEGIN_MON2(open, const char*, int, int)
	write_msg("open(\"%s\", %d) = %d\n", p1, p2, ret);
END_MON

BEGIN_MON3(read, int, void*, size_t, int)
	write_msg("read(%d, 0x%X, %d) = %d\n", p1, p2, p3, ret);
END_MON

BEGIN_MON3(write, int, void*, size_t, int)
	write_msg("write(%d, 0x%X, %d) = %d\n", p1, p2, p3, ret);
END_MON

BEGIN_MON1(dup, int, int)
	write_msg("dup(%d) = %d\n", p1, ret);
END_MON

BEGIN_MON2(fopen, const char*, const char*, FILE*)
	write_msg("fopen(\"%s\", \"%s\") = 0x%X\n", p1, p2, ret);
END_MON

BEGIN_MON1(fclose, FILE*, int)
	write_msg("fclose(0x%X) = %d\n", p1, ret);
END_MON
