build:
	g++ vm.c -o larum.exe

clean:
	rm *.o *.so *.exe || true
