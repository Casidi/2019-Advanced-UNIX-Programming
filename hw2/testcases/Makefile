
CC	= gcc
PROGS	= dir file fscanf getline stdio
PRELOAD	= ../fsmon.so

all: $(PROGS)

%: %.c
	$(CC) -o $@ $(CFLAGS) $^

test: wget cat ls

wget:
	LD_PRELOAD=$(PRELOAD) wget -O /dev/null google.com > /dev/null
	
cat:
	LD_PRELOAD=$(PRELOAD) cat /etc/passwd > /dev/null

ls:
	MONITOR_OUTPUT=/tmp/lsdev.log LD_PRELOAD=$(PRELOAD) ls -la /dev > /dev/null

clean:
	rm -f $(PROGS)
