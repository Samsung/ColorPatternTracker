#include <stddef.h>
#include <stdlib.h>
#include <sys/time.h>
#include <exception>
#include <stdexcept>

#include <time.h>

#include "util.h"

namespace JNICLTracker{

void print_out(const char *fmt, ...) {
	va_list args;
	va_start(args, fmt);
	size_t sz = vsnprintf(NULL, 0, fmt, args) + 1;
	char *msg = (char*)malloc(sz);
	vsprintf(msg, fmt, args);

	__android_log_print(ANDROID_LOG_DEBUG, "cgt", "%s", msg);

	free(msg);
	va_end(args);
}

long currentTimeInNanos() {

	struct timespec res;
	clock_gettime(CLOCK_MONOTONIC, &res);
	return (res.tv_sec * NANOS_IN_SECOND) + res.tv_nsec;
}

}
