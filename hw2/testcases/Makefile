
CC	= gcc
PROGS	= dir file fscanf getline stdio
PRELOAD	= ../fsmon.so

all: $(PROGS)

%: %.c
	$(CC) -o $@ $(CFLAGS) $^
	chmod +x $@

test: wget cat ls
	@echo
	LD_PRELOAD=$(PRELOAD) ./dir > /dev/null
	@echo
	LD_PRELOAD=$(PRELOAD) ./file > /dev/null
	@echo
	echo 8877 | LD_PRELOAD=$(PRELOAD) ./fscanf
	@echo
	LD_PRELOAD=$(PRELOAD) ./getline > /dev/null
	@echo
	LD_PRELOAD=$(PRELOAD) ./stdio > /dev/null

wget:
	LD_PRELOAD=$(PRELOAD) wget -O /dev/null google.com > /dev/null
	
cat:
	LD_PRELOAD=$(PRELOAD) cat /etc/passwd > /dev/null

ls:
	MONITOR_OUTPUT=/tmp/lsdev.log LD_PRELOAD=$(PRELOAD) ls -la /dev > /dev/null

clean:
	rm -f $(PROGS)
