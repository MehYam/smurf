#include <errno.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <unistd.h>
#include <sys/types.h>
#include <pwd.h>

#include "util.h"

void eprintf(const char *format, ...)
{
	va_list ap;

	va_start(ap, format);
	vfprintf(stderr, format, ap);
	va_end(ap);

	if (format[0] != '\0' && format[strlen(format)-1] == ':') {
		fputc(' ', stderr);
		perror(NULL);
	} else {
		fputc('\n', stderr);
	}
}

void die(const char *msg)
{
	eprintf("%s", msg);
	exit(1);
}

void getHomeDir(char* dest, size_t size)
{
	const char* dir = NULL;
	if ((dir = getenv("HOME")) == NULL)
	{
		struct passwd* pw = NULL;
		if ((pw = getpwuid(getuid())) != NULL)
		{
			dir = pw->pw_dir;
		}
	}
	if (dir != NULL)
	{
		strncpy(dest, dir, size);
		dest[size-1] = 0;
	}
}