CC=gcc
FLAGS=-Wall -pedantic -Wextra -g
LDLIBS=
EXE=d

${EXE}: ll.o parse.o io.o
	${CC} ${FLAGS} -o ${EXE} ll.o parse.o io.o ${LDLIBS}
ll.o: ll.c ll.h
	${CC} ${FLAGS} -c ll.c 
parse.o: parse.c parse.h ll.h
	${CC} ${FLAGS} -c parse.c 

io.o: io.c io.h ll.h
	${CC} ${FLAGS} -c io.c 

