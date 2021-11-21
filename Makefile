cc=gcc
flags=-Wall -pedantic -Wextra -g
ldlibs=
exe=d
objects= main.o ll.o parse.o io.o ed.o err.o

${exe}: ${objects}
	${cc} ${flags} -o $@ $^ ${ldlibs}

main.o: main.c ll.h parse.h io.h err.h
	${cc} ${flags} -c main.c

ll.o: ll.c ll.h err.h
	${cc} ${flags} -c ll.c 

err.o: err.c err.h
	${cc} ${flags} -c err.c 

ed.o: ed.c ed.h ll.h
	${cc} ${flags} -c ed.c 

parse.o: parse.c parse.h ll.h
	${cc} ${flags} -c parse.c 

io.o: io.c io.h ll.h err.h
	${cc} ${flags} -c io.c 

