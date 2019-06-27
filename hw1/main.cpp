#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <getopt.h>
#include <dirent.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/stat.h>
#include "hw1_utils.h"
#include <vector>

using namespace std;

vector<connection_entry> all_connections;
void parse_protocol(const char* protocol) {
	char proto_file_name[32] = "/proc/net/";
	strcat(proto_file_name, protocol);

	FILE *fp = fopen(proto_file_name, "r");
	size_t len = 0;
	char* line = NULL;
	int line_num = 1;
	int is_ipv6 = (strcmp(protocol, "tcp6") && strcmp(protocol, "udp6"))? 0: 1;

	while(getline(&line, &len, fp) != -1) {
		struct connection_entry entry;
		if(line_num != 1) {
			strcpy(entry.ip_proto, protocol);
			parse_line(line, &entry, is_ipv6);
			all_connections.push_back(entry);
		}
		line_num++;
	}

	free(line);
	fclose(fp);
}

static struct option long_options[] = {
	{"tcp", optional_argument, 0, 0},
	{"udp", optional_argument, 0, 0},
	{0,0,0,0}
};

int main(int argc, char **argv) {
	int tcp_option_presents = 0;
	int udp_option_presents = 0;
	while(1) {
		int option_index = 0;
		int c = getopt_long(argc, argv, "tu", long_options, &option_index);
		if(c == -1)
			break;
		switch(c) {
		case 0:
			if(option_index == 0)
				tcp_option_presents = 1;
			if(option_index == 1)
				udp_option_presents = 1;
			break;
		case 't':
			tcp_option_presents = 1;
			break;
		case 'u':
			udp_option_presents = 1;
			break;
		default:
			return 0;
		}
	}

	char filter[64] = "";
	if(argv[optind] != NULL) {
		//printf("Filter: %s\n", argv[optind]);
		strcpy(filter, argv[optind]);
	}

	parse_protocol("tcp");
	parse_protocol("tcp6");
	parse_protocol("udp");
	parse_protocol("udp6");

	DIR* proc_dir = opendir("/proc");
	struct dirent* ptr;
	while((ptr = readdir(proc_dir)) != NULL) {
		if(isdigit(ptr->d_name[0])) {
			//printf("dname: %s\n", ptr->d_name);
			char proc_fd_dir_path[32];
			sprintf(proc_fd_dir_path, "/proc/%s/fd", ptr->d_name);
			//printf("Opening: %s\n", proc_fd_dir_path);
			DIR* proc_fd_dir = opendir(proc_fd_dir_path);
			struct dirent* fd_entry;
			while((fd_entry = readdir(proc_fd_dir)) != NULL) {
				if(fd_entry->d_name[0] != '.') {
					//printf("\tfd: %s\n", fd_entry->d_name);
					char fd_path[PATH_MAX];
					sprintf(fd_path, "%s/%s", proc_fd_dir_path, fd_entry->d_name);
					struct stat sb;
					stat(fd_path, &sb);
					char *buffer = (char*)malloc(PATH_MAX);
					readlink(fd_path, buffer, PATH_MAX);
					if(strncmp(buffer, "socket", 6) == 0) {
						//printf("%s -> %s\n", fd_path, buffer);
						int inode;
						sscanf(buffer, "socket:[%d]", &inode);
						//printf("inode: %d\n", inode);
						for(int i = 0; i < all_connections.size(); ++i) {
							if(all_connections[i].inode == inode) {
								sscanf(ptr->d_name, "%d", &all_connections[i].pid);
								break;
							}
						}
					}
					free(buffer);
				}
			}
			closedir(proc_fd_dir);
		}
	}
	closedir(proc_dir);

	vector<connection_entry> tcp_entries;
	vector<connection_entry> udp_entries;
	if(!(!tcp_option_presents && udp_option_presents)) {
		for(int i = 0; i < all_connections.size(); ++i) {
			connection_entry *entry = &all_connections[i];
			if(strncmp(entry->ip_proto, "tcp", 3) == 0) {
				char proc_path[64];
				char* cmdline = NULL;
				size_t len = 0;
				sprintf(proc_path, "/proc/%d/cmdline", entry->pid);
				FILE* fp = fopen(proc_path, "r");
				getline(&cmdline, &len, fp);
				fclose(fp);

				if(strstr(cmdline, filter) == NULL)
					continue;
				tcp_entries.push_back(*entry);
			}
		}
	}
	if(!(tcp_option_presents && !udp_option_presents)) {
		for(int i = 0; i < all_connections.size(); ++i) {
			connection_entry *entry = &all_connections[i];
			if(strncmp(entry->ip_proto, "udp", 3) == 0) {
				char proc_path[64];
				char* cmdline = NULL;
				size_t len = 0;
				sprintf(proc_path, "/proc/%d/cmdline", entry->pid);
				FILE* fp = fopen(proc_path, "r");
				getline(&cmdline, &len, fp);
				fclose(fp);

				if(strstr(cmdline, filter) == NULL)
					continue;
				udp_entries.push_back(*entry);
			}
		}
	}

	if(!tcp_entries.empty()) {
		printf("List of TCP connections:\n");
		printf("%-7s%-25s%-25s%s\n", "Proto", "Local Address", "Foreign Address", "PID/Program name and arguments"); 
		for(int i = 0; i < tcp_entries.size(); ++i) {
			connection_entry *entry = &tcp_entries[i];
			print_entry(entry, filter);
		}
	}
	
	if(!tcp_entries.empty() && !udp_entries.empty()) {
		puts("");
	}

	if(!udp_entries.empty()) {
		printf("List of UDP connections:\n");
		printf("%-7s%-25s%-25s%s\n", "Proto", "Local Address", "Foreign Address", "PID/Program name and arguments"); 
		for(int i = 0; i < udp_entries.size(); ++i) {
			connection_entry *entry = &udp_entries[i];
			print_entry(entry, filter);
		}
	}

	return 0;
} 
