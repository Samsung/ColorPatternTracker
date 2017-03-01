#include "cgt.h"
#include "logger.h"

#include <EGL/egl.h> // requires ndk >= r5

namespace JNICLTracker{
/**
 * This file defines the interface to initialize the opencl, process the input frame and destroy the opencl context
 */
// initialize //
JNIEXPORT void JNICALL Java_com_samsung_dtl_colorpatterntracker_ColorGridTracker_initCL(JNIEnv* jenv, jobject obj, jint width, jint height, jint in_tex, jint out_tex) {
    char oclLibName[1024];
    char eglLibName[1024];
    getLibNames(oclLibName, eglLibName);

    clManager = new CLManager(oclLibName, eglLibName);
    clManager->initCL(cgTracker_kernel);

    tracker = new CLTracker(clManager, oclLibName);
    tracker->setupOpenCL(width, height, in_tex, out_tex);
}

// process //
JNIEXPORT void JNICALL Java_com_samsung_dtl_colorpatterntracker_ColorGridTracker_processFrame(JNIEnv* jenv, jobject obj, jlong addrLoc, jlong addrMetadataF, jlong addrMetadataI, jint frameNo, jdouble dt, jboolean trackMultiPattern) {
    //double start = omp_get_wtime();
    cv::Mat& mLoc  = *(cv::Mat*)addrLoc;
    cv::Mat& mMetadataF  = *(cv::Mat*)addrMetadataF; // floating point metadata
    cv::Mat& mMetadataI  = *(cv::Mat*)addrMetadataI; // integer metadata
    print_out("running opencl\n");
    tracker->runOpenCL(mLoc, mMetadataF, mMetadataI, frameNo, dt, trackMultiPattern);

    //double end = omp_get_wtime();
//print_out("Finished OpenCL kernels in %lf ms", (end-start)*1000);
}

// destroy //
JNIEXPORT void JNICALL Java_com_samsung_dtl_colorpatterntracker_ColorGridTracker_destroyCL(JNIEnv* jenv, jobject obj) {
    tracker->cleanupOpenCL();
}

void getLibNames(char *oclLibName, char *eglLibName){
    strcpy(oclLibName,"/system/vendor/lib/egl/libGLES_mali.so");
    strcpy(eglLibName,"/system/vendor/lib/egl/libGLES_mali.so");

    // ARM Mali
    if (FILE *file = fopen("/system/vendor/lib/egl/libGLES_mali.so", "r")) {
        fclose(file);
        strcpy(oclLibName,"/system/vendor/lib/egl/libGLES_mali.so");
        strcpy(eglLibName,"/system/vendor/lib/egl/libGLES_mali.so");
        return;
    }

    if (FILE *file = fopen("/system/lib/egl/libGLES_mali.so", "r")) {
        fclose(file);
        strcpy(oclLibName,"/system/lib/egl/libGLES_mali.so");
        strcpy(eglLibName,"/system/lib/egl/libGLES_mali.so");
        return;
    }

    // PowerVR
    if (FILE *file = fopen("/system/vendor/lib/libPVROCL.so", "r")) {
        fclose(file);
        strcpy(oclLibName,"/system/vendor/lib/libPVROCL.so");
        strcpy(eglLibName,"/system/lib/libEGL.so");
        return;
    }

    //Qualcomm Adreno
    if (FILE *file = fopen("/system/vendor/lib/libOpenCL.so", "r")) { //
        fclose(file);
        strcpy(oclLibName,"/system/vendor/lib/libOpenCL.so");
        strcpy(eglLibName,"/system/lib/libEGL.so");
        return;
    }
    if (FILE *file = fopen("/system/lib/egl/libGLES_mali.so", "r")) { //
        fclose(file);
        strcpy(oclLibName,"/system/lib/egl/libGLES_mali.so");
        strcpy(eglLibName,"/system/lib/libEGL.so");
        return;
    }

    if (FILE *file = fopen("/vendor/lib/egl/libGLESv2_adreno.so", "r")) {
        fclose(file);
        strcpy(oclLibName,"/vendor/lib/egl/libGLESv2_adreno.so");
        strcpy(eglLibName,"/system/lib/libEGL.so");
        return;
    }

}

}

