cc=gcc
flags=-Wall -pedantic -Wextra -g
ldlibs=
exe=d
objects= ll.o parse.o io.o 

${exe}: ${objects}
	${cc} ${flags} -o $@ $^ ${ldlibs}

ll.o: ll.c ll.h
	${cc} ${flags} -c ll.c 

parse.o: parse.c parse.h ll.h
	${cc} ${flags} -c parse.c 

io.o: io.c io.h ll.h
	${cc} ${flags} -c io.c 

