#pragma once

#include <math.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include<dlfcn.h>

#include "src/openglKernels/computeShaderKernels.h"

#include "glIncludes.h"
#include "cl_gl_interop.h"
#include "util.h"
#include "patternUtil.h"
#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"

namespace JNICLTracker{

    struct int2{
        int x, y;
    };

#define stats(v) { \
    GLint64 i64; \
    glGetInteger64v(v, &i64); \
    long long int lli = (long long int)i64; \
    ALOGV(#v ": %lld", lli); \
}

#define stats32_3(v) { \
    GLint i1, i2, i3; \
    glGetIntegeri_v(v, 0, &i1); \
    glGetIntegeri_v(v, 1, &i2); \
    glGetIntegeri_v(v, 2, &i3); \
    ALOGV(#v ": (%d, %d, %d)", i1, i2, i3); \
}

void assertNoGLErrors(const char *step);

class GLManager {
public:
	typedef struct _Params_ {
		bool opengl, verify;
		_Params_() {
			opengl = false;
			verify = false;
		}
	} Params;

	Params params;

public:
	GLManager();
	~GLManager();

	bool initGL(GLuint input_texture, GLuint output_texture, int width, int height);
    void initParameters(GLuint in_tex, GLuint out_tex, int width, int height);
	void releaseGL();
    void initializeGPUKernels();
    void initializeGPUMemoryBuffers();

    // GL
    void createBuffer(const char * str, int size);
    void createBuffer_SSBO(cl_mem &ssbo, int size);
    void createBuffer_SSBO_mem(cl_mem &ssbo, int size, void *memptr);
    bool createShader(const char *progName, const char * kernelString);
    void processFrame(cv::Mat &locMat, cv::Mat &frameMetadataF,
                      cv::Mat &frameMetadataI, int frameNo, double dt,
                      bool trackMultiPatter);

    // init params
    //void setParamsCenterToCorner(int mLocRecFromCornerData[][7]);
    //bool setCornerRefinementParameters();
    //void getCrossIds(int crossIds[]);

    // kernels
    bool copyColor();
    bool getRefinedCorners(cv::Mat &locMat, cv::Mat &frameMetadataF,
                                      cv::Mat &frameMetadataI, int frameNo, bool checkTrans, int debug,
                                      bool trackMultiPattern);
    bool getRefinedCornersForPattern(cv::Mat &locMat, float &intensity, int &patID, bool checkTrans, int patId_prev, int debug, int frameNo);
    //bool getRefinedCornersForPattern(cv::Mat &locMat, cv::Mat &frameMetadataF, cv::Mat &frameMetadataI, int frameNo, bool checkTrans, int patId_prev, int debug);

    //void getCentersFromCorners(cv::Mat &mLocations, cv::Mat &locMat, int mLocRecFromCornerData[][7]);
    bool getValidTrans(cv::Mat &mLocations, cv::Mat &locMat,
                                  cl_mem &mem_16Pts);
    //void getCornersFromCrossPts(cv::Mat &locMat, float crossPts[]);
    //float getReprojectionAndErrorForPattern(cv::Mat &locMat, float err_max,bool isComplete);

    //bool getPatternIdAndIntensity(cv::Mat &locMat, cv::Mat &frameMetadataI, cv::Mat &frameMetadataF, int patId_prev, int debug);
    bool getPatternIdAndIntensity(cv::Mat &locMat, int &patID, float &intensity, int patId_prev, int debug);

    bool colorConversion(cl_kernel &knl_colConversion, cl_mem &memobj_in,
                                    int w_img, int h_img, bool saveOutput, const char *fname);
    bool getColorPurity(cl_kernel &knl_purity, cl_mem &memobj_purity,
                                   int w_img, int h_img, bool saveOutput, const char *fname);
    int getBlockCorners(cl_kernel &knl_cornersZero,
                        cl_kernel &knl_getBlockCorners, cl_kernel &knl_refineCornerPoints,
                        cl_mem &memobj_in, cl_mem &memobj_purity, cl_mem &memobj_corners,
                        cl_mem & memobj_cornersNew, int w_img, int h_img, bool saveOutput,
                        const char *fname, const char *fname2);
    bool extractCornersAndPatternVotes(cl_kernel & knl_resetNCorners,
                                                  cl_kernel & knl_getNCorners, cl_kernel & knl_getCorners,
                                                  cl_kernel &knl_getLinePtAssignment, cl_mem &memobj_in,
                                                  cl_mem &memobj_corners, float **pxCorners, float **pyCorners,
                                                  int **pvotes, int &nCorners, size_t w_img, size_t h_img,
                                                  bool saveOutput, const char *fname);
    bool getPatternPoints(float *xCorners, float *yCorners, int *votes,
                                     int nCorners, cv::Mat &locMat, size_t w_img, size_t h_img,
                                     cv::Mat &frameMetadataF, cv::Mat &frameMetadataI,
                                     bool trackMultiPattern, int frameNo, int debug, bool saveOutput,
                                     const char *fname);
    //void validateCorners(std::vector<int> &id_pts, float *xCorners, float *yCorners);

    //cv::Mat getBoundingQuad(cv::Mat &locMat_this);
    //bool isInsideQuad(cv::Mat &quad, float x, float y);

    void findNewPattern(float *ptPurity, int *ptClass, int &count_patterns,
                                   cv::Mat &locMat, int nCorners, int Pow2[], uchar valid_ptSel[],
                                   float *xCorners, float *yCorners, int frameNo, int debug,
                                   cv::Mat &frameMetadataF, cv::Mat &frameMetadataI);

    CornerRefinementParam cornerParams;

    std::map<std::string, cl_mem> mems;
    std::map<std::string, cl_kernel> kernels;

	size_t max_cu;	//max compute units

    int m_sz_blk;

    int2 img_size;

    // textures
    GLuint input_texture;
    GLuint output_texture;

    // shaders

    // memory
    cl_mem D_ssbo;
};
}
