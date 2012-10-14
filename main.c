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

char *command = "wclip";

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
		fprintf(stderr, "%s: Failed to retrieve clipboard data.", command);
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
	void *buffer, *globalbuffer;
	size_t bufsize;
	BOOL success;
	HGLOBAL hglb;
	int rc = 1;

	buffer = freadtobuffer(in, &bufsize);
	if(buffer == NULL)
		return 1;

	hglb = GlobalAlloc(GMEM_MOVEABLE, bufsize);
	if(hglb == NULL) goto ret;

	globalbuffer = GlobalLock(hglb);
	if(globalbuffer == NULL) goto ret;
	memcpy(globalbuffer, buffer, bufsize);

	EmptyClipboard(); /* Obviously required. */

	if(SetClipboardData(CF_TEXT, hglb) != NULL)
		rc = 0;
	else
		GlobalFree(hglb);

ret:
	free(buffer);
	return rc;
}

void *freadtobuffer(FILE *in, size_t *retsize)
{
	char *buffer;
	size_t size = 256, offset = 0;
	int c, prev = 0;

	buffer = malloc(size);

	for(;;) {
		c = fgetc(in);

		if(c == EOF)
			break;
		
		if(c == '\n' && prev != '\r') {
			buffer[offset++] = '\r';
			if(offset == size)
				buffer = realloc(buffer, size *= 2);
		}

		buffer[offset++] = c;
		if(offset == size)
			buffer = realloc(buffer, size *= 2);

		prev = c;
	}

	if(offset == 0) {
		free(buffer);
		return NULL;
	}

	buffer = realloc(buffer, offset + 1);
	((char *)buffer)[offset] = '\0';

	if(retsize != NULL)
		*retsize = offset + 1;

	return buffer;
}

void usage(FILE *out)
{
	fprintf(out, "%s [ -o | <file> ]", command);
}

int main(int argc, char *argv[])
{
	int err, i;
	FILE *file;

	if(argc)
		command = argv[0];

	err = openclip();
	if(err) {
		fprintf(stderr, "%s: Failed to open clipboard.", command);
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

	file = fopen(argv[1], "r");

	if(file == NULL) {
		fprintf(stderr, "%s: %s", strerror(errno), command);
		exit(1);
	}
	err = fsetclip(file);

	exit(err);
}
