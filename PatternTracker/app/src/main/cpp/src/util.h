#pragma once

#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "logger.h"
#define LOG_TAG "GLES3JNI"
#define ALOGV(...) __android_log_print(ANDROID_LOG_VERBOSE, LOG_TAG, __VA_ARGS__)
#define ALOGE(...) __android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__)
#define NANOS_IN_SECOND 1000000000
namespace JNICLTracker{
	void print_out(const char *fmt, ...);
	long currentTimeInNanos();
}
