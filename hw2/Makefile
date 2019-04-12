all:
	gcc -o fsmon.so -shared -fPIC fsmon.c -ldl

test:
	make -C testcases
	make -C testcases test

clean:
	make -C testcases clean
	rm -f fsmon.so
