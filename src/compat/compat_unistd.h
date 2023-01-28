#ifndef H_COMPAT_UNISTD
#define H_COMPAT_UNISTD

#ifndef _WIN32
#include <unistd.h>
#else
#include <io.h>

#ifdef _WIN64
typedef __int64 ssize_t;
#else
typedef long ssize_t;
#endif

#define STDIN_FILENO 0
#define STDOUT_FILENO 1
#define STDERR_FILENO 2

#endif

#endif
