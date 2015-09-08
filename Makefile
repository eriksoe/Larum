CFLAGS = -Wall -ggdb

build:
	g++ ${CFLAGS} *.c -o larum.exe

clean:
	rm *.o *.so *.exe || true
