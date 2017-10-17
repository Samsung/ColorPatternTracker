#include "cgt.h"


namespace JNICLTracker{
void tryComputeShader();


void printOpenGLStats() {
    stats(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE);
    stats(GL_MAX_SHADER_STORAGE_BLOCK_SIZE);
    stats(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS);
    stats(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS);
    stats32_3(GL_MAX_COMPUTE_WORK_GROUP_COUNT);
    stats32_3(GL_MAX_COMPUTE_WORK_GROUP_SIZE);
}

/**
 * This file defines the interface to initialize the opencl, process the input frame and destroy the opencl context
 */
// initialize //
JNIEXPORT void JNICALL Java_com_samsung_dtl_colorpatterntracker_ColorGridTracker_initCL(JNIEnv* jenv, jobject obj, jint width, jint height, jint in_tex, jint out_tex) {
    const char* v = (const char*)glGetString(GL_VERSION);
    ALOGV("GL %s: %s\n", "Version", v);

    glManager = new GLManager();
    glManager->initGL(in_tex, out_tex, width, height);
    //tryComputeShader();
}

// process //
JNIEXPORT void JNICALL Java_com_samsung_dtl_colorpatterntracker_ColorGridTracker_processFrame(JNIEnv* jenv, jobject obj, jlong addrLoc, jlong addrMetadataF, jlong addrMetadataI, jint frameNo, jdouble dt, jboolean trackMultiPattern) {
    cv::Mat& mLoc  = *(cv::Mat*)addrLoc;
    cv::Mat& mMetadataF  = *(cv::Mat*)addrMetadataF; // floating point metadata
    cv::Mat& mMetadataI  = *(cv::Mat*)addrMetadataI; // integer metadata
    print_out("running opencl\n");
    glManager->processFrame(mLoc, mMetadataF, mMetadataI, frameNo, dt, trackMultiPattern);
}

// destroy //
JNIEXPORT void JNICALL Java_com_samsung_dtl_colorpatterntracker_ColorGridTracker_destroyCL(JNIEnv* jenv, jobject obj) {
    //tracker->cleanupOpenCL();
}

    void tryComputeShader() {
        int i;
        printOpenGLStats();
        // Initialize our compute program
        GLuint compute_prog = glCreateProgram();
        assertNoGLErrors("create program");

        const int workgroupSize = 1024; // max supported by Nexus 6

        GLuint shader = glCreateShader(GL_COMPUTE_SHADER);
        assertNoGLErrors("create shader");
        const GLchar* sources = { colConversion_kernel };
        glShaderSource(shader, 1, &sources, NULL);
        assertNoGLErrors("shader source");
        glCompileShader(shader);
        assertNoGLErrors("compile shader");

        GLint shader_ok = 0;
        glGetShaderiv(shader, GL_COMPILE_STATUS, &shader_ok);
        assertNoGLErrors("post compile shader");
        if (shader_ok != GL_TRUE) {
            ALOGE("Could not compile shader:\n");
            GLint log_len;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &log_len);
            char* log = (char*)malloc(log_len * sizeof(char));
            glGetShaderInfoLog(shader, log_len, NULL, log);
            ALOGE("%s\n", log);
            glDeleteShader(shader);
            return;
        }

        glAttachShader(compute_prog, shader);
        assertNoGLErrors("attach shader");
        glLinkProgram(compute_prog);
        assertNoGLErrors("link shader");

        ALOGV("Program linked");
        const int POINTS = 63*1024;  // N6: max number of points that can be actually retrieved using MapBufferRange
        const size_t sizeInBytes = POINTS * 4 * sizeof(float); // N6: only 1MB out of 134,217,728 max texture size works with MapBufferRange
        GLuint buffers[2];
        glGenBuffers(2, buffers);
        GLuint position_buffer = buffers[0];
        GLuint velocity_buffer = buffers[1];

        printOpenGLStats();

        ALOGV("All done with tryComputeShader");
        return;
    }
}

