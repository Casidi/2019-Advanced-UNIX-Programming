#include <netinet/in.h>
#include <arpa/inet.h>

struct connection_entry {
	char ip_proto[16];
	char local_addr[64];
	char local_port[8];
	char remote_addr[64];
	char remote_port[8];
	int pid, inode;
};

void parse_addr_port_pair(char* pair, char* addr, char* port_str, int is_ipv6) {
	if(is_ipv6) {
		struct in6_addr bin_addr;
		unsigned long port;
		sscanf(pair, "%x:%x", &(bin_addr.s6_addr), &port);
		inet_ntop(AF_INET6, (void*)&bin_addr, addr, 64);
		if(port == 0)
			sprintf(port_str, "*");
		else
			sprintf(port_str, "%d", port);
	} else {
		struct in_addr bin_addr;
		unsigned long port;
		sscanf(pair, "%x:%x", &(bin_addr.s_addr), &port);
		inet_ntop(AF_INET, (void*)&bin_addr, addr, 64);
		if(port == 0)
			sprintf(port_str, "*");
		else
			sprintf(port_str, "%d", port);
	}
}

void parse_line(char* line, struct connection_entry* entry, int is_ipv6) {
	char *token = strtok(line, " ");
	int count = 0;
	while(token != NULL) {
		if(token[0] != '\n') {
			if(count == 1) {
				parse_addr_port_pair(token, entry->local_addr, entry->local_port, is_ipv6);
			}
			if(count == 2) {
				parse_addr_port_pair(token, entry->remote_addr, entry->remote_port, is_ipv6);
			}
			if(count == 9) {
				sscanf(token, "%d", &entry->inode);
			}
			count++;
		}
		token = strtok(NULL, " ");
	}
}

void print_entry(struct connection_entry* entry, char* filter) {
        char local_end[32];
        char remote_end[32];
        sprintf(local_end, "%s:%s", entry->local_addr, entry->local_port);
        sprintf(remote_end, "%s:%s", entry->remote_addr, entry->remote_port);

        char proc_path[64];
        char* cmdline = NULL;
        size_t len = 0;
        sprintf(proc_path, "/proc/%d/cmdline", entry->pid);
        FILE* fp = fopen(proc_path, "r");
        getline(&cmdline, &len, fp);
        fclose(fp);

        char program_path[32];
        sscanf(cmdline, "%s ", program_path);
        printf("%-7s%-25s%-25s%d/%s %s\n",
                entry->ip_proto, local_end, remote_end, entry->pid,
                basename(program_path), cmdline+strlen(program_path)+1);
}

