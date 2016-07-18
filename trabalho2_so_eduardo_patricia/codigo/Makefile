CC=gcc

all: trabalho2

trabalho2: main.o input.o interface.o processo.o comum.h
	$(CC) -std=gnu99 -o trabalho2 main.o input.o interface.o processo.o -lpthread

main.o: main.c comum.h
	$(CC) -std=gnu99 -c main.c

input.o: input.c comum.h
	$(CC) -std=gnu99 -c input.c

interface.o: interface.c comum.h
	$(CC) -std=gnu99 -c interface.c
	
processo.o: processo.c comum.h
	$(CC) -std=gnu99 -c processo.c

.PHONY: clean

clean:
	rm -f *.o trabalho2
