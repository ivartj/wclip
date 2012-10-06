#define WIN32_LEAN_AND_MEAN 

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

void closeclip(void);
int openclip(void);
int fprintclip(FILE *out);
int fsetclip(FILE *in);
void *freadtobuffer(FILE *in, size_t *retsize);

void closeclip(void)
{
	CloseClipboard();
}

int openclip(void)
{
	BOOL success = OpenClipboard(NULL);
	if(success)
		return 0;
	else
		return 1;
}

int fprintclip(FILE *out)
{
	HANDLE data = GetClipboardData(CF_TEXT);
	if(data == NULL) {
		fprintf(stderr, "Failed to retrieve clipboard data.\n");
		exit(1);
	}
	char *str = GlobalLock(data);
	if(str != NULL) {
		fputs(str, out);
		GlobalUnlock(data);
	}
	return str == NULL;
}

int fsetclip(FILE *in)
{
	void *buffer, *gbuffer;
	size_t bufsize;
	BOOL success;
	HGLOBAL hglb;
	int retval = 1;

	buffer = freadtobuffer(in, &bufsize);

	hglb = GlobalAlloc(GMEM_MOVEABLE, bufsize);
	if(hglb == NULL) goto ret;

	gbuffer = GlobalLock(hglb);
	if(gbuffer == NULL) goto ret;
	memcpy(gbuffer, buffer, bufsize);

	EmptyClipboard();

	if(SetClipboardData(CF_TEXT, hglb) != NULL)
		retval = 0;
	else
		GlobalFree(hglb);

ret:
	free(buffer);
	return retval;
}

void *freadtobuffer(FILE *in, size_t *retsize)
{
	void *buffer, *pos;
	size_t size = 256, offset = 0;
	buffer = pos = malloc(size);
	for(;;) {
		offset += fread(pos, 1, size + buffer - pos, in);
		if(offset < size)
			break;
		buffer = realloc(buffer, size *= 2);
		pos = buffer + offset;
	}
	if(offset)
		((char *)buffer)[offset] = '\0';
	if(retsize != NULL)
		*retsize = size;
	return buffer;
}

void usage(FILE *out)
{
	fprintf(out, "wclip [ -o | <file> ]\n");
}

int main(int argc, char *argv[])
{
	int err, i;
	FILE *file;

	err = openclip();
	if(err) {
		fprintf(stderr, "Failed to open clipboard.");
		exit(1);
	}
	atexit(closeclip);
		
	if(argc <= 1) {
		err = fsetclip(stdin);
		exit(err);
	}

	if(strcmp(argv[1], "-o") == 0) {
		err = fprintclip(stdout);
		exit(err);
	}

	if(!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
		usage(stdout);
		exit(0);
	}

	/* The binary flag prevents carriage returns from being omitted
	 * in read operations. */
	file = fopen(argv[1], "rb");

	if(file == NULL) {
		fprintf(stderr, "wclip: %s", strerror(errno));
		exit(1);
	}
	err = fsetclip(file);

	exit(err);
}
