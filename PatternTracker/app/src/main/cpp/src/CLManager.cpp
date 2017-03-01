#include <stddef.h>
#include <stdlib.h>
#include <sys/time.h>
#include <iostream>
#include <exception>
#include <stdexcept>

#include "CLManager.h"

namespace JNICLTracker{
CLManager::CLManager(char *_oclLibName, char *_eglLibName) {
	strcpy(oclLibraryName,_oclLibName);
	strcpy(eglLibraryName,_eglLibName);
	loadOCLLibrary();
	m_clContext = 0;
	m_queue = 0;
	m_program = 0;
}

CLManager::~CLManager() {
	unloadOCLLibrary();
}

void	CLManager::unloadOCLLibrary(){
		dlclose(oclLibraryHandle);
		dlclose(eglLibraryHandle);
	}

void	CLManager::loadOCLLibrary(){
		oclLibraryHandle = dlopen(oclLibraryName, RTLD_GLOBAL | RTLD_NOW);
		eglLibraryHandle = dlopen(eglLibraryName, RTLD_GLOBAL | RTLD_NOW);

		*(void **)(&myClGetPlatformIDs) = dlsym(oclLibraryHandle, "clGetPlatformIDs");
		*(void **)(&myClGetDeviceIDs) = dlsym(oclLibraryHandle, "clGetDeviceIDs");
		*(void **)(&myClGetDeviceInfo) = dlsym(oclLibraryHandle, "clGetDeviceInfo");
		*(void **)(&myClCreateContext) = dlsym(oclLibraryHandle, "clCreateContext");
		*(void **)(&myClReleaseContext) = dlsym(oclLibraryHandle, "clReleaseContext");
		*(void **)(&myClReleaseCommandQueue) = dlsym(oclLibraryHandle, "clReleaseCommandQueue");
		*(void **)(&myClCreateBuffer) = dlsym(oclLibraryHandle, "clCreateBuffer");
		*(void **)(&myClReleaseMemObject) = dlsym(oclLibraryHandle, "clReleaseMemObject");
		*(void **)(&myClCreateProgramWithSource) = dlsym(oclLibraryHandle, "clCreateProgramWithSource");
		*(void **)(&myClReleaseProgram) = dlsym(oclLibraryHandle, "clReleaseProgram");
		*(void **)(&myClBuildProgram) = dlsym(oclLibraryHandle, "clBuildProgram");
		*(void **)(&myClGetProgramBuildInfo) = dlsym(oclLibraryHandle, "clGetProgramBuildInfo");
		*(void **)(&myClCreateKernel) = dlsym(oclLibraryHandle, "clCreateKernel");
		*(void **)(&myClReleaseKernel) = dlsym(oclLibraryHandle, "clReleaseKernel");
		*(void **)(&myClSetKernelArg) = dlsym(oclLibraryHandle, "clSetKernelArg");
		*(void **)(&myClGetEventProfilingInfo) = dlsym(oclLibraryHandle, "clGetEventProfilingInfo");
		*(void **)(&myClFinish) = dlsym(oclLibraryHandle, "clFinish");
		*(void **)(&myClEnqueueReadBuffer) = dlsym(oclLibraryHandle, "clEnqueueReadBuffer");
		*(void **)(&myClEnqueueWriteBuffer) = dlsym(oclLibraryHandle, "clEnqueueWriteBuffer");
		*(void **)(&myClEnqueueNDRangeKernel) = dlsym(oclLibraryHandle, "clEnqueueNDRangeKernel");
		*(void **)(&myClCreateCommandQueue) = dlsym(oclLibraryHandle, "clCreateCommandQueue");
		*(void **)(&myClEnqueueTask) = dlsym(oclLibraryHandle, "clEnqueueTask");
		*(void **)(&myClEnqueueAcquireGLObjects) = dlsym(oclLibraryHandle, "clEnqueueAcquireGLObjects");
		*(void **)(&myClEnqueueReleaseGLObjects) = dlsym(oclLibraryHandle, "clEnqueueReleaseGLObjects");
		*(void **)(&myClCreateFromGLTexture2D) = dlsym(oclLibraryHandle, "clCreateFromGLTexture2D");

		*(void **)(&myEglGetCurrentDisplay) = dlsym(eglLibraryHandle, "eglGetCurrentDisplay");
		*(void **)(&myEglGetError) = dlsym(eglLibraryHandle, "eglGetError");
		*(void **)(&myEglGetCurrentContext) = dlsym(eglLibraryHandle, "eglGetCurrentContext");
	}

bool CLManager::initCL(const char *source) {

	EGLDisplay mEglDisplay;
	EGLContext mEglContext;

	if ((mEglDisplay = myEglGetCurrentDisplay()) == EGL_NO_DISPLAY) {
		JNICLTracker::print_out("eglGetCurrentDisplay() returned error %d", myEglGetError());
	}
	JNICLTracker::print_out("eglGetCurrentDisplay() returned error %d success is %d", myEglGetError(),EGL_SUCCESS);

	if ((mEglContext = myEglGetCurrentContext()) == EGL_NO_CONTEXT) {
		print_out("eglGetCurrentContext() returned error %d", myEglGetError());
	}
	print_out("eglGetCurrentContext() returned error %d success is %d", myEglGetError(), EGL_SUCCESS);

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
	err = myClGetPlatformIDs(params.platformIndex+1, platforms, &numPlatforms);
	CHECK_ERROR_OCL(err, "getting platforms", releaseCL, return false);
	print_out("Platform index %d (%d platforms found)", params.platformIndex, numPlatforms);
	if (params.platformIndex >= numPlatforms) {
		print_out("Platform index %d out of range (%d platforms found)",
			params.platformIndex, numPlatforms);
		return false;
	}
	platform = platforms[params.platformIndex];

	cl_device_id devices[params.deviceIndex+1];
	err = myClGetDeviceIDs(platform, params.type, params.deviceIndex+1, devices, &numDevices);
	CHECK_ERROR_OCL(err, "getting devices", releaseCL, return false);
	print_out("Device index %d out of range (%d devices found)",params.deviceIndex, numDevices);
	if (params.deviceIndex >= numDevices) {
		print_out("Device index %d out of range (%d devices found)",
			params.deviceIndex, numDevices);
		return false;
	}
	m_device = devices[params.deviceIndex];

	char name[64];
	err = myClGetDeviceInfo(m_device, CL_DEVICE_NAME, 64, name, NULL);
	CHECK_ERROR_OCL(err, "getting device info device name", releaseCL, return false);
	print_out("Using device: %s", name);

	char clVersionName[1024];
	err = myClGetDeviceInfo(m_device, CL_DEVICE_VERSION, 1024, clVersionName, NULL);
	CHECK_ERROR_OCL(err, "getting device info device version", releaseCL, return false);
	print_out("Using opencl version : %s", clVersionName);

	err = myClGetDeviceInfo(m_device, CL_DEVICE_PROFILE, 1024, clVersionName, NULL);
	CHECK_ERROR_OCL(err, "getting device info device version profile", releaseCL, return false);
	print_out("Using opencl version profile: %s", clVersionName);

	err = myClGetDeviceInfo(m_device, CL_DEVICE_EXTENSIONS, 1024, clVersionName, NULL);
	CHECK_ERROR_OCL(err, "getting device info device extensions", releaseCL, return false);
	print_out("Using opencl version profile: %s", clVersionName);

	cl_ulong device_size;
	err = myClGetDeviceInfo(m_device, CL_DEVICE_GLOBAL_MEM_SIZE, sizeof(device_size), &device_size, NULL);
	CHECK_ERROR_OCL(err, "global mem", releaseCL, return false);
	print_out("CL_DEVICE_GLOBAL_MEM_SIZE: %lu bytes", device_size);

	err = myClGetDeviceInfo(m_device, CL_DEVICE_LOCAL_MEM_SIZE, sizeof(device_size), &device_size, NULL);
	CHECK_ERROR_OCL(err, "local mem", releaseCL, return false);
	print_out("CL_DEVICE_LOCAL_MEM_SIZE: %lu bytes", device_size);

	err = myClGetDeviceInfo(m_device, CL_DEVICE_MAX_COMPUTE_UNITS, sizeof(size_t), &max_cu, NULL);
	CHECK_ERROR_OCL(err, "compute units", releaseCL, return false);
	print_out("CL_DEVICE_MAX_COMPUTE_UNITS: %lu", max_cu);

	if (params.opengl) context_prop[5] = (cl_context_properties) platform;

	m_clContext = myClCreateContext(context_prop, 1, &m_device, NULL, NULL, &err);
	if(err == CL_INVALID_PLATFORM){
		print_out("CL_INVALID_PLATFORM");
	}
	if(err == CL_INVALID_VALUE){
		print_out("CL_INVALID_VALUE");
	}
	if(err == CL_INVALID_DEVICE){
		print_out("CL_INVALID_DEVICE");
	}
	if(err == CL_DEVICE_NOT_AVAILABLE){
		print_out("CL_DEVICE_NOT_AVAILABLE");
	}
	if(err == CL_OUT_OF_HOST_MEMORY){
		print_out("CL_OUT_OF_HOST_MEMORY");
	}
	CHECK_ERROR_OCL(err, "creating context", releaseCL, return false);

	m_queue = myClCreateCommandQueue(m_clContext, m_device, CL_QUEUE_PROFILING_ENABLE, &err);
	CHECK_ERROR_OCL(err, "creating command queue", releaseCL, return false);

	m_program = myClCreateProgramWithSource(m_clContext, 1, &source, NULL, &err);
	CHECK_ERROR_OCL(err, "creating program", releaseCL, return false);

	err = myClBuildProgram(m_program, 1, &m_device, options, NULL, NULL);
	if (err == CL_BUILD_PROGRAM_FAILURE) {
		size_t sz;
		myClGetProgramBuildInfo(
			m_program, m_device, CL_PROGRAM_BUILD_LOG, 0, NULL, &sz);
		char *log = (char*)malloc(++sz);
		myClGetProgramBuildInfo(
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
		myClReleaseProgram(m_program);
		m_program = 0;
	}
	if (m_queue) {
		myClReleaseCommandQueue(m_queue);
		m_queue = 0;
	}
	if (m_clContext) {
		myClReleaseContext(m_clContext);
		m_clContext = 0;
	}
}
}
