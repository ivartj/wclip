all: wclip

wclip: main.c
	$(CC) -o wclip main.c

clean:
	rm wclip.exe
