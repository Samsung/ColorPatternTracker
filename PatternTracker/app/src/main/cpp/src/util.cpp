#include <stddef.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
#include <exception>
#include <stdexcept>

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

}
