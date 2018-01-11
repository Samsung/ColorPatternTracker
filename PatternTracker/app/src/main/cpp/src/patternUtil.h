#pragma once

#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>

#include "opencv2/core/core.hpp"
#include "opencv2/imgproc/imgproc.hpp"
#include "opencv2/highgui/highgui.hpp"
#include "opencv2/calib3d/calib3d.hpp"

#include "logger.h"
#include "cl_gl_interop.h"
namespace JNICLTracker{
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
	void getCentersFromCorners(cv::Mat &mLocations, cv::Mat &locMat, int mLocRecFromCornerData[][7]);
    void getPatternIdAndIntensityFromGrayVals(float indicator[], int patId_prev, int &id_code, float &intensity);
    float getReprojectionAndErrorForPattern(cv::Mat &locMat, float err_max, bool isComplete, CornerRefinementParam &cornerParams);
    void getCornersFromCrossPts(cv::Mat &locMat, float crossPts[]);
    void validateCorners(std::vector<int> &id_pts, float *xCorners, float *yCorners);
    cv::Mat getBoundingQuad(cv::Mat &locMat_this);
    bool isInsideQuad(cv::Mat &quad, float x, float y);
    void setParamsCenterToCorner(int mLocRecFromCornerData[][7]);
    void getCrossIds(int crossIds[]);
    bool setCornerRefinementParameters(CornerRefinementParam &cornerParams);
}
