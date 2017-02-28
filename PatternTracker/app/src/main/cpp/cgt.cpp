#include "cgt.h"
#include "logger.h"

#include <EGL/egl.h> // requires ndk >= r5

namespace JNICLTracker{
/**
 * This file defines the interface to initialize the opencl, process the input frame and destroy the opencl context
 */
// initialize //
JNIEXPORT void JNICALL Java_com_samsung_dtl_colorpatterntracker_ColorGridTracker_initCL(JNIEnv* jenv, jobject obj, jint width, jint height, jint in_tex, jint out_tex) {
    clManager = new CLManager();
    clManager->initCL(cgTracker_kernel);

    tracker = new CLTracker(clManager);
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

}

