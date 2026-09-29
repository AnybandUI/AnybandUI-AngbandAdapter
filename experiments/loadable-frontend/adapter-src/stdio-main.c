/* The executable owns transport. The session owns protocol and engine access.
 */
#include "session.h"
#include <stdio.h>
#include <stdlib.h>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/select.h>
#include <unistd.h>
#endif
static bool read_line(void *user, char *line, size_t length)
{
	return fgets(line, (int)length, stdin) != NULL;
}
static bool write_line(void *user, const char *line)
{
	return puts(line) >= 0 && fflush(stdout) == 0;
}
static bool poll_input(void *user)
{
#ifdef _WIN32
	DWORD n = 0;
	return PeekNamedPipe(GetStdHandle(STD_INPUT_HANDLE), NULL, 0, NULL, &n,
	                     NULL) &&
	       n > 0;
#else
	fd_set fds;
	struct timeval t = {0, 0};
	FD_ZERO(&fds);
	FD_SET(0, &fds);
	return select(1, &fds, NULL, NULL, &t) > 0;
#endif
}
static void stop(void *user, int status)
{
	exit(status);
}
int main(int argc, char **argv)
{
	const struct anybandui_io io = {NULL, read_line, write_line, poll_input,
	                                stop};
	setvbuf(stdin, NULL, _IONBF, 0);
	return anybandui_run(&io, argc, argv);
}