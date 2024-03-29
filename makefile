#
# Now, only the one file will be recompiled
#
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

test: 
	/bin/sleep 20000
test0:
	/usr/bin/who
	/bin/ls
test1: 
	echo "run ./a.out"
	./a.out
	echo "pass ./a.out"
test2: 
	echo "run /bin/asdfjh"
	/bin/asdfjh
	echo "pass /bin/asdfjh"
test3: 
	echo "run ./myprog1.c"
	./myprog1.c
	echo "pass ./myprog1.c"
test4: test5
	/bin/ls
test5: test6
	/usr/bin/who
test6: test4
	/usr/bin/whoami
test7: test8
	/bin/ls
