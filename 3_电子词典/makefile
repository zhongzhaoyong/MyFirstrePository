test:func.o ser.o
	gcc func.o ser.o -o test -lsqlite3 -pthread
func.o:1_serfunc.c
	gcc -c 1_serfunc.c -o func.o
ser.o:2_ser.c
	gcc -c 2_ser.c -o ser.o -pthread

clean:
	rm *.o
