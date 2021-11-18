CC=gcc
FLAGS=-Wall -pedantic -Wextra -g
LDLIBS=
EXE=d

${EXE}: ed.c
	${CC} ${FLAGS} -o ${EXE} ed.c ${LDLIBS}

ll: ll.o 
	${CC} ${FLAGS} -o ${EXE} ll.o ${LDLIBS}
ll.o: ll.c ll.h
	${CC} ${FLAGS} -c ll.c 
