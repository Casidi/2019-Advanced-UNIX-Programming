all:
	gcc -c -o elftool.o elftool.c
	gcc -o sdb main.c elftool.o -lelf

test:
	echo "load sample/hello64\nrun\nload test\nrun\nvmmap\nexit\n" | ./sdb
	echo
