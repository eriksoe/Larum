build:
	g++ -Wall *.c -o larum.exe

clean:
	rm *.o *.so *.exe || true
