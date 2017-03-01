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
#include<dlfcn.h>

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
	CLManager(char *_oclLibName, char *_eglLibName);
	~CLManager();

	bool initCL(const char *source);
	void releaseCL();

	cl_device_id m_device;
	cl_context m_clContext;
	cl_command_queue m_queue;
	cl_program m_program;

	size_t max_cu;	//max compute units

	char oclLibraryName[1024];
	char eglLibraryName[1024];

	 void loadOCLLibrary();
	 void unloadOCLLibrary();

	 cl_int
	(*myClGetPlatformIDs)(cl_uint          /* num_entries */,
						cl_platform_id * /* platforms */,
						cl_uint *        /* num_platforms */) ;

/* Device APIs */
	 cl_int
	(*myClGetDeviceIDs)(cl_platform_id   /* platform */,
					  cl_device_type   /* device_type */,
					  cl_uint          /* num_entries */,
					  cl_device_id *   /* devices */,
					  cl_uint *        /* num_devices */) ;

	 cl_int
	(*myClGetDeviceInfo)(cl_device_id    /* device */,
					   cl_device_info  /* param_name */,
					   size_t          /* param_value_size */,
					   void *          /* param_value */,
					   size_t *        /* param_value_size_ret */) ;

	 cl_context
	(*myClCreateContext)(const cl_context_properties * /* properties */,
					   cl_uint                 /* num_devices */,
					   const cl_device_id *    /* devices */,
					   void (CL_CALLBACK * /* pfn_notify */)(const char *, const void *, size_t, void *),
					   void *                  /* user_data */,
					   cl_int *                /* errcode_ret */) ;

	 cl_command_queue
	(*myClCreateCommandQueue)(cl_context                     /* context */,
							cl_device_id                   /* device */,
							cl_command_queue_properties    /* properties */,
							cl_int *                       /* errcode_ret */) ;

	 cl_program
	(*myClCreateProgramWithSource)(cl_context        /* context */,
								 cl_uint           /* count */,
								 const char **     /* strings */,
								 const size_t *    /* lengths */,
								 cl_int *          /* errcode_ret */) ;

	 cl_int
	(*myClBuildProgram)(cl_program           /* program */,
					  cl_uint              /* num_devices */,
					  const cl_device_id * /* device_list */,
					  const char *         /* options */,
					  void (CL_CALLBACK *  /* pfn_notify */)(cl_program /* program */, void * /* user_data */),
					  void *               /* user_data */) ;

	 cl_int
	(*myClGetProgramBuildInfo)(cl_program            /* program */,
							 cl_device_id          /* device */,
							 cl_program_build_info /* param_name */,
							 size_t                /* param_value_size */,
							 void *                /* param_value */,
							 size_t *              /* param_value_size_ret */) ;

	 cl_int
	(*myClReleaseProgram)(cl_program /* program */) ;

	 cl_int
	(*myClReleaseCommandQueue)(cl_command_queue /* command_queue */) ;

	 cl_int
	(*myClReleaseContext)(cl_context /* context */) ;

	 cl_int
	(*myClEnqueueAcquireGLObjects)(cl_command_queue      /* command_queue */,
								 cl_uint               /* num_objects */,
								 const cl_mem *        /* mem_objects */,
								 cl_uint               /* num_events_in_wait_list */,
								 const cl_event *      /* event_wait_list */,
								 cl_event *            /* event */) ;
	 cl_int
	(*myClFinish)(cl_command_queue /* command_queue */) ;

	 cl_int
	(*myClEnqueueReleaseGLObjects)(cl_command_queue      /* command_queue */,
								 cl_uint               /* num_objects */,
								 const cl_mem *        /* mem_objects */,
								 cl_uint               /* num_events_in_wait_list */,
								 const cl_event *      /* event_wait_list */,
								 cl_event *            /* event */) ;

	 cl_mem
	(*myClCreateBuffer)(cl_context   /* context */,
					  cl_mem_flags /* flags */,
					  size_t       /* size */,
					  void *       /* host_ptr */,
					  cl_int *     /* errcode_ret */) ;

	 cl_int
	(*myClEnqueueWriteBuffer)(cl_command_queue   /* command_queue */,
							cl_mem             /* buffer */,
							cl_bool            /* blocking_write */,
							size_t             /* offset */,
							size_t             /* size */,
							const void *       /* ptr */,
							cl_uint            /* num_events_in_wait_list */,
							const cl_event *   /* event_wait_list */,
							cl_event *         /* event */) ;

	 cl_int
	(*myClSetKernelArg)(cl_kernel    /* kernel */,
					  cl_uint      /* arg_index */,
					  size_t       /* arg_size */,
					  const void * /* arg_value */) ;

	 cl_int
	(*myClEnqueueNDRangeKernel)(cl_command_queue /* command_queue */,
							  cl_kernel        /* kernel */,
							  cl_uint          /* work_dim */,
							  const size_t *   /* global_work_offset */,
							  const size_t *   /* global_work_size */,
							  const size_t *   /* local_work_size */,
							  cl_uint          /* num_events_in_wait_list */,
							  const cl_event * /* event_wait_list */,
							  cl_event *       /* event */) ;

	 cl_int
	(*myClReleaseMemObject)(cl_mem /* memobj */) ;

	 cl_int
	(*myClEnqueueReadBuffer)(cl_command_queue    /* command_queue */,
						   cl_mem              /* buffer */,
						   cl_bool             /* blocking_read */,
						   size_t              /* offset */,
						   size_t              /* size */,
						   void *              /* ptr */,
						   cl_uint             /* num_events_in_wait_list */,
						   const cl_event *    /* event_wait_list */,
						   cl_event *          /* event */) ;

	 cl_int
	(*myClReleaseKernel)(cl_kernel   /* kernel */) ;

	 cl_kernel
	(*myClCreateKernel)(cl_program      /* program */,
					  const char *    /* kernel_name */,
					  cl_int *        /* errcode_ret */) ;

	 cl_mem
	(*myClCreateFromGLTexture2D)(cl_context      /* context */,
							   cl_mem_flags    /* flags */,
							   cl_GLenum       /* target */,
							   cl_GLint        /* miplevel */,
							   cl_GLuint       /* texture */,
							   cl_int *        /* errcode_ret */) ;

	 cl_int
	(*myClEnqueueTask)(cl_command_queue  /* command_queue */,
					 cl_kernel         /* kernel */,
					 cl_uint           /* num_events_in_wait_list */,
					 const cl_event *  /* event_wait_list */,
					 cl_event *        /* event */) ;

	 cl_int
	(*myClGetEventProfilingInfo)(cl_event            /* event */,
							   cl_profiling_info   /* param_name */,
							   size_t              /* param_value_size */,
							   void *              /* param_value */,
							   size_t *            /* param_value_size_ret */) ;

	 void * oclLibraryHandle, * eglLibraryHandle;

	EGLAPI EGLDisplay EGLAPIENTRY (*myEglGetCurrentDisplay)(void);
	EGLAPI EGLint EGLAPIENTRY (*myEglGetError)(void);
	EGLAPI EGLContext EGLAPIENTRY (*myEglGetCurrentContext)(void);

};
}
