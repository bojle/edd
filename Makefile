cc=gcc
flags=-Wall -pedantic -Wextra -g -Wno-unused-parameter
ldlibs=-lreadline
exe=edd
objects= main.o ll.o parse.o io.o ed.o err.o aux.o undo.o 
macros=-D ED_INCLUDE_READLINE=0 -D ED_INCLUDE_HISTORY=0
install_dir=/usr/local/bin

${exe}: ${objects}
	${cc} ${flags} ${macros} -o $@ $^ ${ldlibs}

main.o: main.c ll.h parse.h io.h err.h ed.h undo.h
	${cc} ${flags} -c main.c

ll.o: ll.c ll.h err.h
	${cc} ${flags} -c ll.c 

err.o: err.c err.h
	${cc} ${flags} -c err.c 

ed.o: ed.c ed.h ll.h io.h aux.h parse.h
	${cc} ${flags} -c ed.c 

parse.o: parse.c parse.h ll.h aux.h undo.h io.h
	${cc} ${flags} -c parse.c 

io.o: io.c io.h ll.h err.h ed.h aux.h
	${cc} ${flags} -c io.c 

aux.o: aux.c aux.h err.h io.h ll.h undo.h
	${cc} ${flags} -c aux.c 

undo.o: undo.c undo.h ll.h parse.h aux.h err.h ed.h
	${cc} ${flags} -c undo.c 

install: ${exe}
	cp ./${exe} ${install_dir}/${exe}

uninstall:
	rm ${install_dir}/${exe}

clean:
	rm  *.o
