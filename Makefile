all: wclip

wclip: main.o
	$(CC) -o wclip main.o

main.o: main.c
	$(CC) -c main.c

clean:
	rm wclip.exe
