# can be used to test 
CC=/usr/bin/gcc
CFLAG=-g 
a.out: myprog1.o myprog2.o myprog3.o 
	$(CC) $(CFLAG) myprog1.o myprog2.o myprog3.o
myprog1.o: myprog1.c myprog.h
	$(CC) -c myprog1.c
myprog2.o: myprog2.c myprog.h
	$(CC) -c myprog2.c
myprog3.o: myprog3.c myprog.h
	$(CC) -c myprog3.c
clean:
	/bin/rm -f a.out
	/bin/rm -f myprog1.o myprog2.o myprog3.o
	/bin/rm -f myprog1.c~ myprog2.c~ myprog3.c~ myprog.h~
