all: mini

test: mytest alarm1 alarm2 alarm3 jmp1 jmp2
	@echo "All tests run"

mytest: mini
	gcc -c -g -Wall -fno-stack-protector -nostdlib -I. -I.. -DUSEMINI test.c
	ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o test test.o start.o -L. -L.. -lmini
	LD_LIBRARY_PATH=. ./test
	@echo

alarm1: mini
	gcc -c -g -Wall -fno-stack-protector -nostdlib -I. -I.. -DUSEMINI alarm1.c
	ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o alarm1 alarm1.o start.o -L. -L.. -lmini
	rm alarm1.o
	-LD_LIBRARY_PATH=. ./alarm1
	@echo

alarm2: mini
	gcc -c -g -Wall -fno-stack-protector -nostdlib -I. -I.. -DUSEMINI alarm2.c
	ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o alarm2 alarm2.o start.o -L. -L.. -lmini
	rm alarm2.o
	LD_LIBRARY_PATH=. ./alarm2
	@echo

alarm3: mini
	gcc -c -g -Wall -fno-stack-protector -nostdlib -I. -I.. -DUSEMINI alarm3.c
	ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o alarm3 alarm3.o start.o -L. -L.. -lmini
	rm alarm3.o
	LD_LIBRARY_PATH=. ./alarm3 &
	sleep 3
	killall alarm3 -s INT
	@echo

jmp1: mini
	gcc -o jmp1.o -c -g -Wall -fno-stack-protector -nostdlib -I. -I.. -DUSEMINI jmp1.c
	ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o jmp1 jmp1.o start.o -L. -L.. -lmini
	rm jmp1.o
	LD_LIBRARY_PATH=. ./jmp1
	@echo

jmp2: mini
	gcc -o jmp2.o -c -g -Wall -fno-stack-protector -nostdlib -I. -I.. -DUSEMINI jmp2.c
	ld -m elf_x86_64 --dynamic-linker /lib64/ld-linux-x86-64.so.2 -o jmp2 jmp2.o start.o -L. -L.. -lmini
	rm jmp2.o
	LD_LIBRARY_PATH=. ./jmp2
	@echo

mini:
	yasm -f elf64 -DYASM -D__x86_64__ -DPIC libmini64.asm -o libmini64.o
	gcc -c -g -Wall -fno-stack-protector -fPIC -nostdlib libmini.c
	ld -shared -o libmini.so libmini64.o libmini.o
	yasm -f elf64 -DYASM -D__x86_64__ -DPIC start.asm -o start.o
	@echo

clean:
	rm -f *.o *.so
