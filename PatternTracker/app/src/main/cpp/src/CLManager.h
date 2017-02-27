#pragma once

#include <map>
#include <math.h>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include <CL/cl.h>
#include <CL/cl_gl.h>
//#include <omp.h>
#include "opencv2/core/core.hpp"

#include <EGL/egl.h> // requires ndk >= r5

#include "util.h"

namespace JNICLTracker{
#ifdef __ANDROID_API__
	#include <GLES/gl.h>
#else
	#include <GL/gl.h>
#endif

#define CHECK_ERROR_OCL(err, op, releaseFunction, returnFunction)	\
	if (err != CL_SUCCESS) {										\
		print_out("Error during operation '%s' (%d)", op, err);	\
		releaseFunction();											\
		returnFunction;												\
	}

class CLManager {
public:
	typedef struct _Params_ {
		cl_device_type type;
		cl_uint platformIndex, deviceIndex;
		bool opengl, verify;
		_Params_() {
			type = CL_DEVICE_TYPE_ALL;
			opengl = false;
			deviceIndex = 0;
			platformIndex = 0;
			verify = false;
		}
	} Params;

	Params params;

public:
	CLManager();
	~CLManager();

	bool initCL(const char *source);
	void releaseCL();

	cl_device_id m_device;
	cl_context m_clContext;
	cl_command_queue m_queue;
	cl_program m_program;

	size_t max_cu;	//max compute units
};
}
