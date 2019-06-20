all: elftool.o
	gcc -o sdb main.c elftool.o -lelf

test: elftool.o test.c sdb.h
	gcc -g -o test test.c elftool.o -lelf
	./test

elftool.o:
	gcc -c -o elftool.o elftool.c
	
clean:
	rm -f *.o
	rm -f test sdb
