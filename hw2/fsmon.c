#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <dirent.h>

static DIR* (*old_opendir)(const char*);
//static struct dirent* (*old_readdir)(DIR *);
static int (*old_creat)(const char *, mode_t);
static int (*old_open)(const char*, int);

static int (*old_fclose)(FILE*);
static FILE* (*old_fopen)(const char*, const char*);

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

__attribute__((constructor)) void init_mon(void) {
	fprintf(stderr, "Monitor initializing...\n");
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
	//fprintf(stderr, "Before output: %s\n", msg);
	vfprintf(stderr, msg, vl);
	//fprintf(stderr, "After output\n");
	
	va_end(vl);
}

FILE* fopen(const char* filename, const char* mode) {
	old_fopen = load_function("fopen");
	FILE* ret_val = old_fopen(filename, mode);
	write_msg("fopen(\"%s\", \"%s\") = 0x%X\n", filename, mode, ret_val);
	return ret_val;
}

int fclose(FILE* fp) {
	old_fclose = load_function("fclose");
	int ret = old_fclose(fp);
	write_msg("fclose(0x%X) = %d\n", fp, ret);
	return ret;
}

DIR* opendir(const char* dirname) {
	old_opendir = load_function("opendir");
	DIR* dir = old_opendir(dirname);
	write_msg("opendir(\"%s\") = 0x%X\n", dirname, dir);
	return dir;
}
/*
#define MON_FUNC(f,RT, T1, msg_fmt) static int (*old_##f)(T1);\
	RT f(T1 p1) {\
		old_##f = load_function(#f);\
		RT ret = old_##f(p1);\
		write_msg(msg_fmt, p1, ret);\
		return ret;\
	}
MON_FUNC(closedir, int, DIR*, "closedir(0x%X) = %d\n");
MON_FUNC(readdir, struct dirent*, DIR*, "readdir(0x%X) = 0x%X\n")
*/

static int (*old_closedir)(DIR*);
int closedir(DIR* dir) {
	old_closedir = load_function("closedir");
	int ret = old_closedir(dir);
	write_msg("closedir(0x%X) = %d\n", dir, ret);
	return ret;
}

int open(const char* pathname, int flags) {
	old_open = load_function("open");
	int ret = old_open(pathname, flags);
	write_msg("open(\"%s\", %d) = %d\n", pathname, flags, ret);
	return ret;
}

static struct dirent* (*old_readdir)(DIR*);
struct dirent* readdir(DIR* dp) {
	old_readdir = load_function("readdir");
	struct dirent* ret = old_readdir(dp);
	//write_msg("readdir(\"%s\") = %s\n", dp->ent->d_name, ret->d_name); // not work
	//fprintf(stderr, "d_name = %s\n", ret->d_name);
	//write_msg("readdir(\"%X\") = %s\n", dp, ret->d_name); // not work
	write_msg("readdir() = ?\n");
	return ret;
}

int creat(const char *path, mode_t mode) {
	old_creat = load_function("creat");
	int ret = old_creat(path, mode);
	write_msg("creat(\"%s\", %d) = %d\n", path, mode, ret);
	return ret;
}
