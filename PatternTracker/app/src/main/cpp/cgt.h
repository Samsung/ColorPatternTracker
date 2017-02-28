#ifndef JNIAPI_H
#define JNIAPI_H
#include <jni.h>
#include <android/native_window.h> // requires ndk >= r5
#include <android/native_window_jni.h> // requires ndk >= r5

#include "src/opencl/cgTracker.h"

#include "src/CLManager.h"
#include "src/CLTracker.h"
#include "util.h"

namespace JNICLTracker{

CLManager* clManager;
CLTracker* tracker;

extern "C" {
	JNIEXPORT void JNICALL Java_com_samsung_dtl_colorpatterntracker_ColorGridTracker_initCL(JNIEnv* jenv, jobject obj, jint width, jint height, jint in_tex, jint out_tex);
	JNIEXPORT void JNICALL Java_com_samsung_dtl_colorpatterntracker_ColorGridTracker_processFrame(JNIEnv* jenv, jobject obj, jlong addrLoc, jlong addrMetadataF, jlong addrMetadataI, jint frameNo, jdouble dt, jboolean trackMultiPattern);
	JNIEXPORT void JNICALL Java_com_samsung_dtl_colorpatterntracker_ColorGridTracker_destroyCL(JNIEnv* jenv, jobject obj);
};
}

#endif // JNIAPI_H
