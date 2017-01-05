#include <stddef.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
#include <exception>
#include <stdexcept>

#include "CLManager.h"
namespace JNICLTracker{
CLManager::CLManager() {
	m_clContext = 0;
	m_queue = 0;
	m_program = 0;
}

CLManager::~CLManager() {
}

bool CLManager::initCL(const char *source) {
	EGLDisplay mEglDisplay;
	EGLContext mEglContext;

	if ((mEglDisplay = eglGetCurrentDisplay()) == EGL_NO_DISPLAY) {
		JNICLTracker::print_out("eglGetCurrentDisplay() returned error %d", eglGetError());
	}
	JNICLTracker::print_out("eglGetCurrentDisplay() returned error %d success is %d", eglGetError(),EGL_SUCCESS);

	if ((mEglContext = eglGetCurrentContext()) == EGL_NO_CONTEXT) {
		print_out("eglGetCurrentContext() returned error %d", eglGetError());
	}
	print_out("eglGetCurrentContext() returned error %d success is %d", eglGetError(), EGL_SUCCESS);

	cl_context_properties context_prop[7];
	context_prop[0] = CL_GL_CONTEXT_KHR;
	context_prop[1] = (cl_context_properties) mEglContext;
	context_prop[2] = CL_EGL_DISPLAY_KHR;
	context_prop[3] = (cl_context_properties) mEglDisplay;
	context_prop[4] = CL_CONTEXT_PLATFORM;
	context_prop[6] = 0;

	params.opengl = true;
	char options[1024];
	sprintf(options, "-cl-fast-relaxed-math");

	// Ensure no existing context
	releaseCL();

	cl_int err;
	cl_uint numPlatforms, numDevices;

	cl_platform_id platform, platforms[params.platformIndex+1];
	err = clGetPlatformIDs(params.platformIndex+1, platforms, &numPlatforms);
	CHECK_ERROR_OCL(err, "getting platforms", releaseCL, return false);
	print_out("Platform index %d (%d platforms found)", params.platformIndex, numPlatforms);
	if (params.platformIndex >= numPlatforms) {
		print_out("Platform index %d out of range (%d platforms found)",
			params.platformIndex, numPlatforms);
		return false;
	}
	platform = platforms[params.platformIndex];

	cl_device_id devices[params.deviceIndex+1];
	err = clGetDeviceIDs(platform, params.type, params.deviceIndex+1, devices, &numDevices);
	CHECK_ERROR_OCL(err, "getting devices", releaseCL, return false);
	print_out("Device index %d out of range (%d devices found)",params.deviceIndex, numDevices);
	if (params.deviceIndex >= numDevices) {
		print_out("Device index %d out of range (%d devices found)",
			params.deviceIndex, numDevices);
		return false;
	}
	m_device = devices[params.deviceIndex];

	char name[64];
	err = clGetDeviceInfo(m_device, CL_DEVICE_NAME, 64, name, NULL);
	CHECK_ERROR_OCL(err, "getting device info device name", releaseCL, return false);
	print_out("Using device: %s", name);

	cl_ulong device_size;
	err = clGetDeviceInfo(m_device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(device_size), &device_size, NULL);
	CHECK_ERROR_OCL(err, "global mem", releaseCL, return false);
	print_out("CL_DEVICE_GLOBAL_MEM_SIZE: %lu bytes", device_size);

	err = clGetDeviceInfo(m_device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(device_size), &device_size, NULL);
	CHECK_ERROR_OCL(err, "local mem", releaseCL, return false);
	print_out("CL_DEVICE_LOCAL_MEM_SIZE: %lu bytes", device_size);

	err = clGetDeviceInfo(m_device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(size_t), &max_cu, NULL);
	CHECK_ERROR_OCL(err, "compute units", releaseCL, return false);
	print_out("CL_DEVICE_MAX_COMPUTE_UNITS: %lu", max_cu);

	if (params.opengl) context_prop[5] = (cl_context_properties) platform;

	m_clContext = clCreateContext(context_prop, 1, &m_device, NULL, NULL, &err);
	CHECK_ERROR_OCL(err, "creating context", releaseCL, return false);

	m_queue = clCreateCommandQueue(m_clContext, m_device, CL_QUEUE_PROFILING_ENABLE, &err);
	CHECK_ERROR_OCL(err, "creating command queue", releaseCL, return false);

	m_program = clCreateProgramWithSource(m_clContext, 1, &source, NULL, &err);
	CHECK_ERROR_OCL(err, "creating program", releaseCL, return false);

	err = clBuildProgram(m_program, 1, &m_device, options, NULL, NULL);
	if (err == CL_BUILD_PROGRAM_FAILURE) {
		size_t sz;
		clGetProgramBuildInfo(
			m_program, m_device, CL_PROGRAM_BUILD_LOG, 0, NULL, &sz);
		char *log = (char*)malloc(++sz);
		clGetProgramBuildInfo(
			m_program, m_device, CL_PROGRAM_BUILD_LOG, sz, log, NULL);
		print_out(log);
		free(log);
	}
	CHECK_ERROR_OCL(err, "building program", releaseCL, return false);

	print_out("OpenCL context initialised.");
	return true;
}

void CLManager::releaseCL() {
	if (m_program) {
		clReleaseProgram(m_program);
		m_program = 0;
	}
	if (m_queue) {
		clReleaseCommandQueue(m_queue);
		m_queue = 0;
	}
	if (m_clContext) {
		clReleaseContext(m_clContext);
		m_clContext = 0;
	}
}
}
