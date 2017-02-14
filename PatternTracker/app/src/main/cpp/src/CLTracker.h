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
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"
#include "CLManager.h"
#include "util.h"

namespace JNICLTracker{

class CLTracker {
public:
	CLTracker(CLManager * _clManager);
	~CLTracker();

	struct int2{
		int x, y;
	};

	struct GPUVars{
		cl_context context;
		cl_command_queue queue;
	};

	GPUVars gpuVars; // relevant GPU parameters
	int2 img_size;   // image size
	GLuint in_tex;   // input texture
	GLuint out_tex;  // output texture

	CLManager *clManager; // opencl manager

	cl_mem mem_images[2]; // opencl memory for input and output textures

	std::map<std::string, cl_mem> mems;
	std::map<std::string, cl_kernel> kernels;

	size_t m_sz_blk; // generic block size parameter used in algorithms

	size_t gws[2]; // global working set
	size_t lws[2]; // local working set

	struct CornerRefinementParam{
		int mLocRecFromCornerData[16][7];
		int crossIDs[48];
		float crossPts[48];
		float colVals[48];
		cl_mem mem_crossIds;
		cl_mem mem_16Pts;
		cl_mem mem_crossPts;
		cl_mem mem_col16Pts;
		cv::Mat ptsTM;
		cv::Mat ptsTM_homo;
	};
	CornerRefinementParam cornerParams;

	// initialize //
	bool setupOpenCL(int width, int height, GLuint input_texture, GLuint output_texture);
	bool setCornerRefinementParameters();
	void setParamsCenterToCorner(int mLocRecFromCornerData[][7]);
	void getCrossIds(int crossIDs[]);
	bool initializeGPUKernels();
	bool initializeGPUMemoryBuffers();

	// process //
	bool runOpenCL(cv::Mat &locMat, cv::Mat &frameMetadataF, cv::Mat &frameMetadataI, int frameNo, double dt, bool trackMultiPattern);
	double runCLKernels(cv::Mat &locMat, cv::Mat &frameMetadataF, cv::Mat &frameMetadataI, int frameNo, double dt, bool trackMultiPattern);

	void set2DSizes(const char *str, int g0, int g1, int l0, int l1);

	double getProcTime(cl_event k_proc_event, const char *str);
	bool colorConversion(cl_kernel &knl_colConversion, cl_mem &memobj_in, int w_img, int h_img, bool saveOutput, const char *fname);
	bool getCorners(cl_kernel &knl_getCorners, cl_mem &memobj_integR, cl_mem &memobj_integG, cl_mem &memobj_integB, cl_mem &memobj_corner, int w_img, int h_img, bool saveOutput, const char *fname);
	int getBlockCorners(cl_kernel &knl_cornersZero, cl_kernel &knl_getBlockCorners, cl_kernel &knl_refineCornerPoints, cl_mem &memobj_in, cl_mem &memobj_purity, cl_mem &memobj_corners, cl_mem & memobj_cornersNew, int w_img, int h_img, bool saveOutput, const char *fname, const char *fname2);
	bool getColorPurity(cl_kernel &knl_purity, cl_mem &memobj_purity, int w_img, int h_img, bool saveOutput, const char *fname);
	bool readImage(cl_kernel &knl_readImg, cl_mem &memobj_imgc, int w_img, int h_img, bool saveOutput, const char *fname);
	bool getCorners(cl_kernel & knl_resetNCorners, cl_kernel & knl_getNCorners, cl_kernel & knl_getCorners, cl_kernel &knl_getLinePtAssignment, cl_mem &memobj_in, cl_mem &memobj_corners, cv::Mat &locMat, size_t w_img, size_t h_img, bool saveOutput, const char *fname);
	bool getRefinedCornersForPattern(cv::Mat &locMat,  cv::Mat &frameMetadataF, cv::Mat &frameMetadataI, int frameNo, bool checkTrans, int patId_prev, int debug);
	bool getRefinedCorners(cv::Mat &locMat,  cv::Mat &frameMetadataF, cv::Mat &frameMetadataI, int frameNo, bool checkTrans, int debug, bool trackMultiPattern);


	void getCentersFromCorners(cv::Mat &mLocations, cv::Mat &locMat, int mLocRecFromCornerData[][7]);
	bool getValidTrans(cv::Mat &mLocations, cv::Mat &locMat, cl_mem &mem_16Pts);

	void getCornerFromCrossingIds(int id[]);
	bool plotCorners(cv::Mat &locMat, float r, float g, float b);
	bool copyColor(GPUVars &gpuVars);
	float getReprojectionAndError(cv::Mat &locMat, float err_max);
	float getReprojectionAndErrorForPattern(cv::Mat &locMat, float err_max, bool isComplete);
	void validateCorners(std::vector<int> &id_pts, float *xCorners, float *yCorners);

	bool extractCornersAndPatternVotes(cl_kernel & knl_resetNCorners, cl_kernel & knl_getNCorners, cl_kernel & knl_getCorners, cl_kernel &knl_getLinePtAssignment, cl_mem &memobj_in, cl_mem &memobj_corners, float **pxCorners, float **pyCorners, int **pvotes, int &nCorners, size_t w_img, size_t h_img, bool saveOutput, const char *fname);
	bool getPatternPoints(float *xCorners, float *yCorners, int *votes, int nCorners, cv::Mat &locMat,  size_t w_img, size_t h_img, cv::Mat &frameMetadataF, cv::Mat &frameMetadataI, bool trackMultiPattern, int frameNo, int debug, bool saveOutput, const char *fname);
	cv::Mat getBoundingQuad(cv::Mat &locMat_this);
	bool isInsideQuad(cv::Mat &quad,float x, float y);
	void findNewPattern(float *ptPurity, int *ptClass, int &count_patterns, cv::Mat &locMat,  int nCorners, int Pow2[], uchar valid_ptSel[], float *xCorners, float *yCorners, int frameNo, int debug, cv::Mat &frameMetadataF, cv::Mat &frameMetadataI);

	bool getPatternIdAndIntensity(cv::Mat &locMat, cv::Mat &frameMetadataI, cv::Mat &frameMetadataF,int patId_prev, int debug);
	void getPatternIdAndIntensityFromGrayVals(float indicator[], cv::Mat &frameMetadataI, cv::Mat &frameMetadataF, int patId_prev);
	void getCornersFromCrossPts(cv::Mat &locMat, float crossPts[]);

	void transformCorners(cv::Mat &locMat, cv::Mat &outMat, float tx, float ty, float theta, float x_mid, float y_mid);

	void unsetCornerRefinementParameters();

	void setWorkingSets(size_t gws1, size_t gws2, size_t lws1, size_t lws2);

	// destroy //
	bool cleanupOpenCL();
};
}
