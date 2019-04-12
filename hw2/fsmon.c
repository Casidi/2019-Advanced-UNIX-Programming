#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <dlfcn.h>
#include <dirent.h>
#include <sys/stat.h>
#include <libgen.h>

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

	char* log_name = getenv("MONITOR_OUTPUT");
	if(log_name != NULL) {
		FILE* (*my_fopen)(const char*, const char*);
		int (*my_fclose)(FILE*) = load_function("fclose");
		my_fopen = load_function("fopen");
		FILE *fp = my_fopen(log_name, "a");

		vfprintf(fp, msg, vl);
		my_fclose(fp);
	} else {
		vfprintf(stderr, msg, vl);
	}
	
	va_end(vl);
}

char* fd_to_filename(int fd) {
	static char filename[128];
	static char fd_path[64];

	if(fd == 1) {
		sprintf(filename, "<STDOUT>");
		return filename;
	}
	if(fd == 2) {
		sprintf(filename, "<STDERR>");
		return filename;
	}

	sprintf(fd_path, "/proc/self/fd/%d", fd);
	int (*my_readlink)(char*, char*, int) = load_function("readlink");
	int n = my_readlink(fd_path, filename, 127);
	filename[n] = 0;
	return filename;
}

char* get_file_name(FILE* fp) {
	return fd_to_filename(fileno(fp));
}

char* get_dir_name(DIR* dp) {
	return fd_to_filename(dirfd(dp));
}

char* get_verbose_stat(struct stat* s) {
	static char buff[64];
	sprintf(buff, "{mode=%d, size=%ld}", s->st_mode, s->st_size);
	return buff;
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
#define BEGIN_MON4(func,PT1,PT2,PT3,PT4,RT)\
	static RT (*old_##func)(PT1,PT2,PT3,PT4);\
	RT func(PT1 p1, PT2 p2, PT3 p3, PT4 p4) {\
	old_##func = load_function(#func);\
	RT ret = old_##func(p1,p2,p3,p4);
#define END_MON return ret;}

//special case, the dir name should be retreived before closed
BEGIN_MON1(closedir, DIR*, int)
	write_msg("closedir(\"%s\") = %d\n", get_dir_name(p1), ret);
END_MON

BEGIN_MON1(opendir, const char*, DIR*)
	write_msg("opendir(\"%s\") = 0x%X\n", p1, ret);
END_MON

BEGIN_MON1(readdir, DIR*, struct dirent*)
	if(ret != NULL)
		write_msg("readdir(\"%s\") = %s\n", get_dir_name(p1), ret->d_name);
END_MON

BEGIN_MON2(creat, const char*, mode_t, int)
	write_msg("creat(\"%s\", %d) = %d\n", p1, p2, ret);
END_MON

BEGIN_MON2(open, const char*, int, int)
	write_msg("open(\"%s\", %d) = %d\n", p1, p2, ret);
END_MON

BEGIN_MON3(read, int, void*, size_t, int)
	write_msg("read(\"%s\", 0x%X, %d) = %d\n", fd_to_filename(p1), p2, p3, ret);
END_MON

BEGIN_MON3(write, int, void*, size_t, int)
	write_msg("write(%d, 0x%X, %d) = %d\n", p1, p2, p3, ret);
END_MON

BEGIN_MON1(dup, int, int)
	write_msg("dup(%d) = %d\n", p1, ret);
END_MON

BEGIN_MON2(dup2, int,int,int)
	write_msg("dup2(%d, %d) = %d\n", p1,p2,ret);
END_MON

BEGIN_MON1(close, int, int)
	write_msg("close(%d) = %d\n", p1, ret);
END_MON

BEGIN_MON2(lstat, const char*, struct stat*, int)
	write_msg("lstat(\"%s\", 0x%X) = %d\n", p1, p2, ret);
END_MON

BEGIN_MON2(stat, const char*, struct stat*, int)
	write_msg("stat(\"%s\", 0x%X) = %d\n", p1, p2, ret);
END_MON

BEGIN_MON4(pwrite, int, const void*, size_t, off_t, ssize_t)
	write_msg("pwrite(%d, 0x%X, %d, %d) = %d\n", p1, p2, p3, p4, ret);
END_MON

BEGIN_MON2(fopen, const char*, const char*, FILE*)
	write_msg("fopen(\"%s\", \"%s\") = 0x%X\n", p1, p2, ret);
END_MON

//special case, the file name should be read before closed!!!
static int (*old_fclose)(FILE*);
int fclose(FILE* p1) {
	old_fclose = load_function("fclose");
	char *filename = get_file_name(p1);
	int ret = old_fclose(p1);
	write_msg("fclose(\"%s\") = %d\n", filename, ret);
	return ret;
}

BEGIN_MON4(fread, void*, size_t, size_t, FILE*, size_t)
	write_msg("pwrite(0x%X, %d, %d, %s) = %d\n", p1, p2, p3, get_file_name(p4), ret);
END_MON

BEGIN_MON4(fwrite, const void*, size_t, size_t, FILE*, size_t)
	write_msg("fwrite(\"%s\", %d, %d, %s) = %d\n", p1,p2,p3,get_file_name(p4),ret);
END_MON

BEGIN_MON4(fwrite_unlocked, const void*, size_t, size_t, FILE*, size_t)
	write_msg("fwrite_unlocked(0x%X, %d, %d, %s) = %d\n", p1,p2,p3,get_file_name(p4),ret);
END_MON

BEGIN_MON1(fgetc, FILE*, int)
	write_msg("fgetc(%s) = %d\n", get_file_name(p1), ret);
END_MON

BEGIN_MON3(fgets, char*, int, FILE*, char*)
	write_msg("fgets(%s, %d, %s) = \"%s\"\n", p1, p2, get_file_name(p3), ret);
END_MON

//BEGIN_MONN(fscanf, 
//END_MON

//BEGIN_MONN(fprintf, 
//END_MON

BEGIN_MON1(chdir, const char*, int)
	write_msg("chdir(\"%s\") = %d\n", p1, ret);
END_MON

BEGIN_MON3(chown, const char*, uid_t, gid_t, int)
	write_msg("chown(\"%s\", %d, %d) = %d\n", p1, p2, p3, ret);
END_MON

BEGIN_MON2(chmod, const char*, mode_t, int)
	write_msg("chmod(%s, %d) = %d\n", p1, p2, ret);
END_MON

BEGIN_MON1(remove, const char*, int)
	write_msg("remove(\"%s\") = %d\n", p1, ret);
END_MON

BEGIN_MON2(rename, const char*, const char*, int)
	write_msg("rename(\"%s\", \"%s\") = %d\n", p1, p2, ret);
END_MON

BEGIN_MON2(link, const char*, const char*, int)
	write_msg("link(\"%s\", \"%s\") = %d\n", p1, p2, ret);
END_MON

BEGIN_MON1(unlink, const char*, int)
	write_msg("unlink(\"%s\") = %d\n", p1, ret);
END_MON

BEGIN_MON3(readlink, const char*, char*, size_t, ssize_t)
	write_msg("readlink(\"%s\", \"%s\", %d) = %d\n", p1, p2, p3, ret);
END_MON

BEGIN_MON2(symlink, const char*, const char*, int)
	write_msg("syslink(\"%s\", \"%s\") = %d\n", p1, p2, ret);
END_MON

BEGIN_MON2(mkdir, const char*, mode_t, int)
	write_msg("mkdir(\"%s\", %d) = %d\n", p1, p2, ret);
END_MON

BEGIN_MON1(rmdir, const char*, int)
	write_msg("rmdir(\"%s\") = %d\n", p1, ret);
END_MON

BEGIN_MON3(__xstat, int, const char*, struct stat*, int)
	write_msg("__xstat(%d, %s, %X) = %d\n", p1, p2, p3, ret);
END_MON

BEGIN_MON3(__lxstat, int, const char*, struct stat*, int)
	write_msg("__lxstat(%d, %s, %X %s) = %d\n", p1, p2, p3, get_verbose_stat(p3), ret);
END_MON

BEGIN_MON2(fputs_unlocked, void*, FILE*, int)
	write_msg("fputs_unlocked(0x%X, %s) = %d\n", p1, get_file_name(p2), ret);
END_MON

