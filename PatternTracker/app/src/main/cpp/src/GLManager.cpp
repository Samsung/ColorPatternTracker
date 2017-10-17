#include <stddef.h>
#include <stdlib.h>
#include <sys/time.h>

#include "GLManager.h"

namespace JNICLTracker{

    void assertNoGLErrors(const char *step) {
        GLenum err = glGetError();
        if (err != GL_NO_ERROR) {
            ALOGE("Failed to %s: %d", step, err);
        } else {
            //ALOGV("Completed %s", step);
        }
    }

    GLManager::GLManager() {
    }

    GLManager::~GLManager() {

    }

    bool GLManager::getRefinedCorners(cv::Mat &locMat, cv::Mat &frameMetadataF,
                                      cv::Mat &frameMetadataI, int frameNo, bool checkTrans, int debug,
                                      bool trackMultiPattern) {
        int nPtsPat = 9;
        int nPatterns = locMat.rows / nPtsPat;
        float err = 500000; //large number w.r.t. image width/height
        int count_tracked = 0;
        for (int i = 0; i < nPatterns; i++) {
            frameMetadataI.at<int>(i, 0) = -1;
        }
        for (int i = 0; i < nPatterns; i++) {
            int id_best = frameMetadataI.at<int>(i, 2);
            if (id_best >= 0) {
                cv::Mat locMat_this = locMat(
                        cv::Rect(0, id_best * nPtsPat, 2, nPtsPat));
                cv::Mat frameMetadataF_this = frameMetadataF(
                        cv::Rect(0, id_best, 1, 1));
                cv::Mat frameMetadataI_this = frameMetadataI(
                        cv::Rect(0, id_best, 1, 1));
                print_out("Tracking for %d", id_best);
                GLManager::getRefinedCornersForPattern(locMat_this,
                                                       frameMetadataF_this, frameMetadataI_this, frameNo,
                                                       checkTrans, id_best, debug);
                print_out("refining finished 1");
                if (frameMetadataI_this.at<int>(0, 0) == id_best) {
                    count_tracked++;
                    if (trackMultiPattern && count_tracked == 2)
                        return true;
                    if (!trackMultiPattern && count_tracked == 1)
                        return true;
                    print_out("Tracking success");
                } else {
                    print_out("Tracking fail");
                    frameMetadataI.at<int>(id_best, 1) = -1;
                    frameMetadataI.at<int>(i, 2) = -1;
                }
            }
        }
        return true;
    }

    void GLManager::getCentersFromCorners(cv::Mat &mLocations, cv::Mat &locMat,
                                          int mLocRecFromCornerData[][7]) {
        // get the center points
        float midPts[6][2][2];

        int mCornerLineID[6][3];
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                mCornerLineID[i][j] = j * 3 + i;
                mCornerLineID[i + 3][j] = i * 3 + j;
            }
        }

        // for each horizontal and vertical line get the mid points
        for (int i = 0; i < 6; i++) {
            for (int j = 0; j < 2; j++) {
                midPts[i][j][0] = ((float) locMat.at<float>(mCornerLineID[i][j], 0)
                                   + (float) locMat.at<float>(mCornerLineID[i][j + 1], 0)) / 2;
                midPts[i][j][1] = ((float) locMat.at<float>(mCornerLineID[i][j], 1)
                                   + (float) locMat.at<float>(mCornerLineID[i][j + 1], 1)) / 2;
            }
        }

        // getting center location points from corner points/mid points
        for (int i = 0; i < 16; i++) {
            int basePt = mLocRecFromCornerData[i][0];
            int hMidLine = mLocRecFromCornerData[i][1];
            int hMidPt = mLocRecFromCornerData[i][2];
            int vMidLine = mLocRecFromCornerData[i][3];
            int vMidPt = mLocRecFromCornerData[i][4];

            float x = (float) locMat.at<float>(basePt, 0);
            float y = (float) locMat.at<float>(basePt, 1);
            x += mLocRecFromCornerData[i][5] * (x - midPts[hMidLine][hMidPt][0])
                 + mLocRecFromCornerData[i][6]
                   * (x - midPts[vMidLine][vMidPt][0]);
            y += mLocRecFromCornerData[i][5] * (y - midPts[hMidLine][hMidPt][1])
                 + mLocRecFromCornerData[i][6]
                   * (y - midPts[vMidLine][vMidPt][1]);
            mLocations.at<float>(i, 0) = x;
            mLocations.at<float>(i, 1) = y;

            //print_out("center:%f %f",x,y);
        }
    }

    bool GLManager::getValidTrans(cv::Mat &mLocations, cv::Mat &locMat,
                                  cl_mem &mem_16Pts) {
        // get valid transformations
        float d_w = sqrt(
                (locMat.at<float>(4, 0) - locMat.at<float>(1, 0))
                * (locMat.at<float>(4, 0) - locMat.at<float>(1, 0))
                + (locMat.at<float>(4, 1) - locMat.at<float>(1, 1))
                  * (locMat.at<float>(4, 1) - locMat.at<float>(1, 1)))
                    / 2;
        float d_h = sqrt(
                (locMat.at<float>(4, 0) - locMat.at<float>(3, 0))
                * (locMat.at<float>(4, 0) - locMat.at<float>(3, 0))
                + (locMat.at<float>(4, 1) - locMat.at<float>(3, 1))
                  * (locMat.at<float>(4, 1) - locMat.at<float>(3, 1)))
                    / 2;
        float d_t = 3.142 / 12; // 30 degree
        float x_mid = locMat.at<float>(4, 0);
        float y_mid = locMat.at<float>(4, 1);

        // get the angle of the positive x axis
        float angle = atan2(locMat.at<float>(4, 0) - locMat.at<float>(3, 0),
                            locMat.at<float>(4, 0) - locMat.at<float>(1, 0));

        // getting tranformation validity
        int err, ret;

        int nx_check = 2;
        int ny_check = 2;
        int nt_check = 2;
        int n_check = (2 * nx_check + 1) * (2 * ny_check + 1) * (2 * nt_check + 1);

        int * transValidity = (int*) malloc(n_check * sizeof(int));

        cl_mem mem_transValidity;
        createBuffer_SSBO(mem_transValidity, n_check * sizeof(int));

        cl_mem mem_inputVals;
        float inVals[5];
        inVals[0] = x_mid;
        inVals[1] = y_mid;
        inVals[2] = angle;
        inVals[3] = d_w;
        inVals[4] = d_h;
        createBuffer_SSBO_mem(mem_inputVals, 5 * sizeof(float),inVals);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mem_transValidity);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mem_16Pts);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mem_inputVals);
        glUseProgram(kernels["getTransValidity"]);assertNoGLErrors("using program");
        glDispatchCompute(16/4, 8/2, 1);assertNoGLErrors("dispatch compute");
        glFinish();

        glBindBuffer( GL_SHADER_STORAGE_BUFFER, mem_transValidity );
        int *validity_gpu= (int *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, n_check * sizeof(int), GL_MAP_READ_BIT);
        std::memcpy( transValidity,validity_gpu, n_check * sizeof(int));
        glFinish();
        glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
        glFinish();

        int count = 0;
        int closest_dist = 100;
        int validx = -1;
        int validy = -1;
        int validt = -1;
        for (int i = -nx_check; i <= nx_check; i++) {
            for (int j = -ny_check; j <= ny_check; j++) {
                for (int k = -nt_check; k <= nt_check; k++) {
                    int transDistance = abs(i) + abs(j) + abs(k);
                    //print_out("valid: %d %d",transDistance,(int)transValidity[count]);
                    if (transValidity[count] != 0) {
                        if (transDistance < closest_dist) {
                            closest_dist = transDistance;
                            validx = i;
                            validy = j;
                            validt = k;
                        }
                    }
                    count++;
                }
            }
        }
        if (validx == -1)
            return false;

        float dx = validx * d_w * cos(angle) + validy * d_h * sin(angle);
        float dy = -validx * d_w * sin(angle) + validy * d_h * cos(angle);
        float dt = validt * d_t;

        float trans[2][2] = { { cos(dt), -sin(dt) }, { sin(dt), cos(dt) } };

        for (int i = 0; i < 16; i++) {
            float x = mLocations.at<float>(i, 0) - x_mid;
            float y = mLocations.at<float>(i, 1) - y_mid;

            mLocations.at<float>(i, 0) = x * trans[0][0] + y * trans[0][1] + dx
                                         + x_mid;
            mLocations.at<float>(i, 1) = x * trans[1][0] + y * trans[1][1] + dy
                                         + y_mid;
        }
        glDeleteBuffers(0,&mem_transValidity);
        glDeleteBuffers(0,&mem_inputVals);
        free(transValidity);
        return true;
    }

    bool GLManager::getRefinedCornersForPattern(cv::Mat &locMat,
                                                cv::Mat &frameMetadataF, cv::Mat &frameMetadataI, int frameNo,
                                                bool checkTrans, int patId_prev, int debug) {
        cv::Mat locMat_old;
        locMat.copyTo(locMat_old);

        int err, ret;

        int w_img = img_size.x;
        int h_img = img_size.y;

        cv::Mat mLocations;
        mLocations = cv::Mat::zeros(16, 2, CV_32FC1);
        getCentersFromCorners(mLocations, locMat,
                              cornerParams.mLocRecFromCornerData);

        glBindBuffer( GL_SHADER_STORAGE_BUFFER, cornerParams.mem_16Pts );
        float *points = (float *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, 16 * 2 * sizeof(float), GL_MAP_WRITE_BIT|GL_MAP_INVALIDATE_BUFFER_BIT );
        std::memcpy( points, mLocations.data, 16 * 2 * sizeof(float));
        glFinish();
        glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );

        if (checkTrans) {
            bool ptsValid = getValidTrans(mLocations, locMat,
                                          cornerParams.mem_16Pts);

            if (!ptsValid) {
                locMat.setTo(cv::Scalar(-1));
                print_out("No valid trans");
                return false;
            }
        }

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, cornerParams.mem_16Pts);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, cornerParams.mem_crossIds);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, cornerParams.mem_crossPts);
        glUseProgram(kernels["getLineCrossing"]);assertNoGLErrors("using program");
        glDispatchCompute(24, 1, 1);assertNoGLErrors("dispatch compute");
        glFinish();

        glBindBuffer( GL_SHADER_STORAGE_BUFFER, cornerParams.mem_crossPts );
        float *buffer_gpu= (float *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, 24 * 2 * sizeof(float), GL_MAP_READ_BIT);
        std::memcpy( cornerParams.crossPts,buffer_gpu, 24 * 2 * sizeof(float));
        glFinish();
        glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
        glFinish();

        getCornersFromCrossPts(locMat, cornerParams.crossPts);
        err = getReprojectionAndErrorForPattern(locMat, 10, true);
        getPatternIdAndIntensity(locMat, frameMetadataI, frameMetadataF, patId_prev,
                                 debug);

        // write points on image
        // debug
        if (debug > 0) {
            print_out("running end marking\n");
            if (frameMetadataI.at<int>(0, 0) == -1)
                return false;

            // marking detected points
            print_out("running marking\n");
            cl_mem mem_9Pts;
            createBuffer_SSBO_mem(mem_9Pts,9 * 2 * sizeof(float),locMat.data);
            glFinish();


            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mem_9Pts);
            glUseProgram(kernels["markDetectedCorners"]);assertNoGLErrors("using program");
            glDispatchCompute(9, 1, 1);assertNoGLErrors("dispatch compute");
            glFinish();

            glDeleteBuffers(1,&mem_9Pts);
        }
// debug

        return true;
    }

    bool GLManager::getPatternIdAndIntensity(cv::Mat &locMat, cv::Mat &frameMetadataI,
                                             cv::Mat &frameMetadataF, int patId_prev, int debug) {
        int err, ret;
        // getting the indicator bits
        float endPts[12][2];
        float k = 30.0 / 23.0; // it is point 7/8th inter point length further from end point
        for (int i = 0; i < 3; i++) {
            // horizontal lines
            float pt0x = locMat.at<float>(i, 0);
            float pt0y = locMat.at<float>(i, 1);
            float pt1x = locMat.at<float>(3 + i, 0);
            float pt1y = locMat.at<float>(3 + i, 1);
            float pt2x = locMat.at<float>(6 + i, 0);
            float pt2y = locMat.at<float>(6 + i, 1);

            float a = std::sqrt(
                    (pt0x - pt1x) * (pt0x - pt1x) + (pt0y - pt1y) * (pt0y - pt1y));
            float b = std::sqrt(
                    (pt2x - pt1x) * (pt2x - pt1x) + (pt2y - pt1y) * (pt2y - pt1y));
            float c0 = a * (b + a) * (k - 1) / (b + a * (1 - k));
            float c1 = b * (a + b) * (k - 1) / (a + b * (1 - k));

            endPts[i * 4 + 0][0] = pt0x + (pt0x - pt2x) / (a + b) * c0;
            endPts[i * 4 + 0][1] = pt0y + (pt0y - pt2y) / (a + b) * c0;
            endPts[i * 4 + 1][0] = pt2x + (pt2x - pt0x) / (a + b) * c1;
            endPts[i * 4 + 1][1] = pt2y + (pt2y - pt0y) / (a + b) * c1;

            // vertical lines
            pt0x = locMat.at<float>(3 * i, 0);
            pt0y = locMat.at<float>(3 * i, 1);
            pt1x = locMat.at<float>(3 * i + 1, 0);
            pt1y = locMat.at<float>(3 * i + 1, 1);
            pt2x = locMat.at<float>(3 * i + 2, 0);
            pt2y = locMat.at<float>(3 * i + 2, 1);

            a = std::sqrt(
                    (pt0x - pt1x) * (pt0x - pt1x) + (pt0y - pt1y) * (pt0y - pt1y));
            b = std::sqrt(
                    (pt2x - pt1x) * (pt2x - pt1x) + (pt2y - pt1y) * (pt2y - pt1y));
            c0 = a * (b + a) * (k - 1) / (b + a * (1 - k));
            c1 = b * (a + b) * (k - 1) / (a + b * (1 - k));

            endPts[i * 4 + 2][0] = pt0x + (pt0x - pt2x) / (a + b) * c0;
            endPts[i * 4 + 2][1] = pt0y + (pt0y - pt2y) / (a + b) * c0;
            endPts[i * 4 + 3][0] = pt2x + (pt2x - pt0x) / (a + b) * c1;
            endPts[i * 4 + 3][1] = pt2y + (pt2y - pt0y) / (a + b) * c1;
        }

        // 12 points anti-clockwise from top
        cv::Mat endPtsXY;
        endPtsXY = cv::Mat::zeros(12, 2, CV_32FC1);

        endPtsXY.at<float>(0, 0) = (endPts[2][0] + endPts[4 + 2][0]) / 2;
        endPtsXY.at<float>(0, 1) = (endPts[2][1] + endPts[4 + 2][1]) / 2;

        endPtsXY.at<float>(1, 0) = (endPts[4 + 2][0] + endPts[2 * 4 + 2][0]) / 2;
        endPtsXY.at<float>(1, 1) = (endPts[4 + 2][1] + endPts[2 * 4 + 2][1]) / 2;

        endPtsXY.at<float>(2, 0) = endPts[1][0]
                                   + (endPts[1][0] - endPts[4 + 1][0]) / 2;
        endPtsXY.at<float>(2, 1) = endPts[1][1]
                                   + (endPts[1][1] - endPts[4 + 1][1]) / 2;

        endPtsXY.at<float>(3, 0) = (endPts[1][0] + endPts[4 + 1][0]) / 2;
        endPtsXY.at<float>(3, 1) = (endPts[1][1] + endPts[4 + 1][1]) / 2;

        endPtsXY.at<float>(4, 0) = (endPts[4 + 1][0] + endPts[4 * 2 + 1][0]) / 2;
        endPtsXY.at<float>(4, 1) = (endPts[4 + 1][1] + endPts[4 * 2 + 1][1]) / 2;

        endPtsXY.at<float>(5, 0) = endPts[4 * 2 + 1][0]
                                   + (endPts[4 * 2 + 1][0] - endPts[4 + 1][0]) / 2;
        endPtsXY.at<float>(5, 1) = endPts[4 * 2 + 1][1]
                                   + (endPts[4 * 2 + 1][1] - endPts[4 + 1][1]) / 2;

        endPtsXY.at<float>(6, 0) = (endPts[4 + 3][0] + endPts[2 * 4 + 3][0]) / 2;
        endPtsXY.at<float>(6, 1) = (endPts[4 + 3][1] + endPts[2 * 4 + 3][1]) / 2;

        endPtsXY.at<float>(7, 0) = (endPts[3][0] + endPts[4 + 3][0]) / 2;
        endPtsXY.at<float>(7, 1) = (endPts[3][1] + endPts[4 + 3][1]) / 2;

        endPtsXY.at<float>(8, 0) = endPts[4 * 2][0]
                                   + (endPts[4 * 2][0] - endPts[4][0]) / 2;
        endPtsXY.at<float>(8, 1) = endPts[4 * 2][1]
                                   + (endPts[4 * 2][1] - endPts[4][1]) / 2;

        endPtsXY.at<float>(9, 0) = (endPts[4][0] + endPts[4 * 2][0]) / 2;
        endPtsXY.at<float>(9, 1) = (endPts[4][1] + endPts[4 * 2][1]) / 2;

        endPtsXY.at<float>(10, 0) = (endPts[0][0] + endPts[4][0]) / 2;
        endPtsXY.at<float>(10, 1) = (endPts[0][1] + endPts[4][1]) / 2;

        endPtsXY.at<float>(11, 0) = endPts[0][0]
                                    + (endPts[0][0] - endPts[4][0]) / 2;
        endPtsXY.at<float>(11, 1) = endPts[0][1]
                                    + (endPts[0][1] - endPts[4][1]) / 2;

        // get pattern colors
        cl_mem mem_12Pts;
        createBuffer_SSBO_mem(mem_12Pts,12 * 2 * sizeof(float),endPtsXY.data);
        glFinish();

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mem_12Pts);
        glUseProgram(kernels["getPatternIndicator"]);assertNoGLErrors("using program");
        glDispatchCompute(12, 1, 1);assertNoGLErrors("dispatch compute");
        glFinish();


        float tempVec[24];
        glBindBuffer( GL_SHADER_STORAGE_BUFFER, mem_12Pts );
        cl_uchar *data_gpu= (cl_uchar *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, 12 * 2 * sizeof(float), GL_MAP_READ_BIT);
        std::memcpy( tempVec,data_gpu, 12 * 2 * sizeof(float));
        glFinish();
        glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
        glFinish();

        glDeleteBuffers(1,&mem_12Pts);

        print_out(
                "patID [%f %f] [%f %f] [%f %f] [%f %f] [%f %f] [%f %f] [%f %f] [%f %f] [%f %f] [%f %f] [%f %f] [%f %f]",
                tempVec[0], tempVec[1], tempVec[2], tempVec[3], tempVec[4],
                tempVec[5], tempVec[6], tempVec[7], tempVec[8], tempVec[9],
                tempVec[10], tempVec[11], tempVec[12], tempVec[13], tempVec[14],
                tempVec[15], tempVec[16], tempVec[17], tempVec[18], tempVec[19],
                tempVec[20], tempVec[21], tempVec[22], tempVec[23]);

        float patternGrayVal[12];
        for (int i = 0; i < 12; i++) {
            patternGrayVal[i] = tempVec[2 * i];
            if (std::abs(tempVec[2 * i + 1]) < 0.05)
                patternGrayVal[i] = -2;
        }
        getPatternIdAndIntensityFromGrayVals(patternGrayVal,
                                                        frameMetadataI, frameMetadataF, patId_prev);

        if (debug > 0) {
            int patternId = frameMetadataI.at<int>(0, 0);
            if (patternId == -1)
                return true;

            cl_mem mem_12Pts2;
            createBuffer_SSBO_mem(mem_12Pts,12 * 2 * sizeof(float),endPtsXY.data);
            glFinish();

            float r, g, b;
            if (patternId == -1) {
                r = 0;
                g = 0;
                b = 0;
            }
            if (patternId == 0) {
                r = 1;
                g = 0;
                b = 0;
            }
            if (patternId == 1) {
                r = 0;
                g = 1;
                b = 0;
            }
            if (patternId == 2) {
                r = 0;
                g = 0;
                b = 1;
            }
            if (patternId == 3) {
                r = 0;
                g = 1;
                b = 1;
            }
            if (patternId == 4) {
                r = 1;
                g = 0;
                b = 1;
            }
            if (patternId == 5) {
                r = 1;
                g = 1;
                b = 0;
            }

            float colVals[3];
            colVals[0]=r;
            colVals[1]=g;
            colVals[2]=b;

            cl_mem mem_colVals;
            createBuffer_SSBO_mem(mem_colVals,3 * sizeof(float),colVals);
            glFinish();

            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mem_12Pts2);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mem_colVals);
            glUseProgram(kernels["plotCorners"]);assertNoGLErrors("using program");
            glDispatchCompute(12, 1, 1);assertNoGLErrors("dispatch compute");
            glFinish();

            glDeleteBuffers(1,&mem_12Pts2);
            glDeleteBuffers(1,&mem_colVals);
        }
        return true;
    }

    void GLManager::getPatternIdAndIntensityFromGrayVals(float indicator[],
                                                         cv::Mat &frameMetadataI, cv::Mat &frameMetadataF, int patId_prev) {
        const int nCodes = 6;
        const int nBits = 12;

        int codes[nCodes][nBits] = { { 0, 0, 1, 0, 0, 0, 1, 3, 2, 1, 0, 1 }, { 0, 3,
                                             0, 0, 0, 2, 1, 3, 2, 0, 3, 0 },
                                     { 0, 0, 1, 0, 1, 0, 0, 3, 2, 0, 3, 1 }, { 0, 3, 0, 0, 1, 2, 0, 3, 2,
                                                                  1, 0, 0 }, { 0, 3, 1, 0, 0, 2, 0, 0, 0, 1, 3, 1 }, { 0, 3,
                                             1, 0, 1, 2, 1, 0, 0, 0, 0, 1 } };

        int colorIndicator[12];

        for (int i = 0; i < nBits; i++) {
            if (indicator[i] >= 2) {
                indicator[i] = indicator[i] - 2;
                colorIndicator[i] = 3;
            } else {
                if (indicator[i] >= 1) {
                    indicator[i] = indicator[i] - 1;
                    colorIndicator[i] = 2;
                } else {
                    if (indicator[i] >= 0) {
                        colorIndicator[i] = 1;
                    }
                }
            }
        }

        int id_code = -1;
        int id_th = -1;
        int nValidBits = 0;

        for (int i = 0; i < nBits; i++)
            if (indicator[i] != -1)
                nValidBits++;

        if (nValidBits < nBits - 3) {
            frameMetadataI.at<int>(0, 0) = id_code;
            return;
        }

        const int nTh = 6;
        float th_vec[nTh] = { 0.1, 0.15, 0.2, 0.25, 0.3, 0.35 };

        if (nValidBits == 0) {
            frameMetadataI.at<int>(0, 0) = id_code;
            return;
        }

        int hCodes[6];
        int thCodes[6];

        int h_min_code = nBits + 1;
        for (int i = 0; i < nCodes; i++) {
            if (i != patId_prev && patId_prev != -1)
                continue;
            int h_min = nBits + 1;
            int id_th_this = -1;
            for (int j = 0; j < nTh; j++) {
                float th = th_vec[j];
                int h = 0;
                for (int k = 0; k < nBits; k++) {
                    if (indicator[k] != -1
                        && ((indicator[k] > th && codes[i][k] == 0)
                            || (indicator[k] <= th && codes[i][k] > 0)
                            || (indicator[k] > th
                                && codes[i][k] != colorIndicator[k])))
                        h++;
                }
                if (h < h_min) {
                    h_min = h;
                    id_th_this = j;
                }
            }
            hCodes[i] = h_min;
            thCodes[i] = id_th_this;

            if (h_min < h_min_code) {
                id_code = i;
                id_th = id_th_this;
                h_min_code = h_min;
            }
        }

        float intensity = 0;
        int count = 0;
        if (id_code != -1) {
            int bits[nBits];
            for (int i = 0; i < nBits; i++) {
                if (indicator[i] > th_vec[id_th]) {
                    bits[i] = 1;
                    intensity += indicator[i];
                    count++;
                } else {
                    bits[i] = 0;
                }
                if (indicator[i] < 0) {
                    bits[i] = 2;
                }
            }

            if (count == 0)
                id_code = -1;
            if (id_code == -1) {
                print_out("hcodes [%d %d] [%d %d] [%d %d] [%d %d] [%d %d] [%d %d]",
                          hCodes[0], thCodes[0], hCodes[1], thCodes[1], hCodes[2],
                          thCodes[2], hCodes[3], thCodes[3], hCodes[4], thCodes[4],
                          hCodes[5], thCodes[5]);
                print_out("th=%f", th_vec[id_th]);
                print_out("code this: %f %f %f %f  %f %f %f %f  %f %f %f %f",
                          indicator[0], indicator[1], indicator[2], indicator[3],
                          indicator[4], indicator[5], indicator[6], indicator[7],
                          indicator[8], indicator[9], indicator[10], indicator[11]);
                print_out("code  sel: %d %d %d %d  %d %d %d %d  %d %d %d %d",
                          codes[id_code][0], codes[id_code][1], codes[id_code][2],
                          codes[id_code][3], codes[id_code][4], codes[id_code][5],
                          codes[id_code][6], codes[id_code][7], codes[id_code][8],
                          codes[id_code][9], codes[id_code][10], codes[id_code][11]);
                print_out("code this: %d %d %d %d  %d %d %d %d  %d %d %d %d",
                          bits[0], bits[1], bits[2], bits[3], bits[4], bits[5],
                          bits[6], bits[7], bits[8], bits[9], bits[10], bits[11]);
            }
        }

        if (h_min_code > 1 && patId_prev == -1)
            id_code = -1;
        if (h_min_code > 3 && patId_prev >= 0)
            id_code = -1;
        frameMetadataI.at<int>(0, 0) = id_code;
        frameMetadataF.at<float>(0, 0) = intensity / count;

        return;
    }

    float GLManager::getReprojectionAndErrorForPattern(cv::Mat &locMat, float err_max,
                                                       bool isComplete) {

        int nCorners = 0;
        cv::Mat mCorners_tm_full;
        cv::Mat mCorners_full;

        if (isComplete) {
            nCorners = 9;
            mCorners_tm_full = cornerParams.ptsTM;
            mCorners_full = locMat.reshape(2);
        } else {
            for (int i = 0; i < 9; i++) {
                if (locMat.at<float>(i, 0) != -1 && locMat.at<float>(i, 1) != -1)
                    nCorners++;
            }

            if (nCorners == 0)
                return 100000;
            mCorners_tm_full = cv::Mat::zeros(nCorners, 1, CV_32FC2);
            mCorners_full = cv::Mat::zeros(nCorners, 1, CV_32FC2);

            nCorners = 0;
            for (int j = 0; j < 3; j++) {
                for (int i = 0; i < 3; i++) {
                    if (locMat.at<float>(j * 3 + i, 0) != -1
                        && locMat.at<float>(j * 3 + i, 1) != -1) {
                        mCorners_tm_full.at < cv::Vec2f > (nCorners++) = cv::Vec2f(
                                cornerParams.ptsTM_homo.at<float>(j * 3 + i, 0),
                                cornerParams.ptsTM_homo.at<float>(j * 3 + i, 1));
                    }
                }
            }

            nCorners = 0;
            for (int i = 0; i < 9; i++) {
                if (locMat.at<float>(i, 0) != -1 && locMat.at<float>(i, 1) != -1) {
                    mCorners_full.at < cv::Vec2f > (nCorners++) = cv::Vec2f(
                            locMat.at<float>(i, 0), locMat.at<float>(i, 1));
                    //print_out("corners:%f %f\n",locMat.at<float>(i,0),locMat.at<float>(i,1));
                }
            }
        }

        cv::Mat OutputMat = cv::Mat();
        cv::Mat homog = findHomography(mCorners_full, mCorners_tm_full, 0, 10,
                                       OutputMat);
        homog.convertTo(homog, locMat.type());

        cv::Mat pts_rec;
        pts_rec = homog.inv() * cornerParams.ptsTM_homo.t();

        // reprojection error
        float err = 0;
        int count = 0;
        for (int i = 0; i < 9; i++) {
            if (locMat.at<float>(i, 0) != -1 && locMat.at<float>(i, 1) != -1) {
                err += std::abs(
                        locMat.at<float>(i, 0)
                        - pts_rec.at<float>(0, i)
                          / pts_rec.at<float>(2, i));
                count++;
            }
        }

        err /= count;
        //print_out("err: %f",err);
        if (err > 10) {
            locMat.setTo(cv::Scalar(-1));
        } else {
            for (int i = 0; i < 9; i++) {
                locMat.at<float>(i, 0) = pts_rec.at<float>(0, i)
                                         / pts_rec.at<float>(2, i);
                locMat.at<float>(i, 1) = pts_rec.at<float>(1, i)
                                         / pts_rec.at<float>(2, i);
                //print_out("corners-complete:%f %f\n",locMat.at<float>(i,0),locMat.at<float>(i,1));
            }
        }

        return err;
    }

    void GLManager::getCornersFromCrossPts(cv::Mat &locMat, float crossPts[]) {
        // line fitting for 3 horizontal and 3 vertical lines
        cv::Mat points_line;
        points_line = cv::Mat::zeros(4, 2, CV_32FC1);
        cv::Mat line;
        line = cv::Mat::zeros(6, 4, CV_32FC1);
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 4; j++) {
                points_line.at<float>(j, 0) = crossPts[2 * (i * 4 + j)];
                points_line.at<float>(j, 1) = crossPts[2 * (i * 4 + j) + 1];
                //print_out("pt: %f %f",crossPts[2*(i*4+j)], crossPts[2*(i*4+j)+1]);
            }
            // estimate horizontal line
            fitLine(points_line, line.row(i), CV_DIST_L2, 0, 0.01, 0.01);
            //print_out("line: %f %f %f %f ",line.at<float>(i,0),line.at<float>(i,1),line.at<float>(i,2),line.at<float>(i,3));

            for (int j = 0; j < 4; j++) {
                points_line.at<float>(j, 0) = crossPts[2 * (12 + i * 4 + j)];
                points_line.at<float>(j, 1) = crossPts[2 * (12 + i * 4 + j) + 1];
                //print_out("pt: %f %f",crossPts[2*(12+i*4+j)], crossPts[2*(12+i*4+j)+1]);
            }
            // estimate horizontal line
            fitLine(points_line, line.row(3 + i), CV_DIST_L2, 0, 0.01, 0.01);
            //print_out("line: %f %f %f %f ",line.at<float>(3+i,0),line.at<float>(3+i,1),line.at<float>(3+i,2),line.at<float>(3+i,3));
        }

        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 3; j++) {
                float x0v = line.at<float>(i, 0);
                float y0v = line.at<float>(i, 1);
                float x0 = line.at<float>(i, 2);
                float y0 = line.at<float>(i, 3);

                float x1v = line.at<float>(3 + j, 0);
                float y1v = line.at<float>(3 + j, 1);
                float x1 = line.at<float>(3 + j, 2);
                float y1 = line.at<float>(3 + j, 3);

                float xnew, ynew;
                if (std::abs(x1v * y0v - y1v * x0v) > std::abs(x0v * y1v - y0v * x1v)) {
                    float t = (y0v * (x0 - x1) - x0v * (y0 - y1))
                              / (x1v * y0v - y1v * x0v);
                    xnew = x1 + t * x1v;
                    ynew = y1 + t * y1v;
                } else {
                    float t = (y1v * (x1 - x0) - x1v * (y1 - y0))
                              / (x0v * y1v - y0v * x1v);
                    xnew = x0 + t * x0v;
                    ynew = y0 + t * y0v;
                }
                locMat.at<float>(3 * j + i, 0) = xnew;
                locMat.at<float>(3 * j + i, 1) = ynew;
            }
        }
    }

    void GLManager::processFrame(cv::Mat &locMat, cv::Mat &frameMetadataF,
                                 cv::Mat &frameMetadataI, int frameNo, double dt,
                                 bool trackMultiPattern) {

        long time_start, time_end;
        time_start =  currentTimeInNanos();
        int w_img = img_size.x;
        int h_img = img_size.y;

        int debug = 0;
        if (frameNo != -1) {
            debug = 1;
        }
        //debug = 2;

        char buf[256];

        // debug
        if (debug > 0) {
            print_out("copying colors\n");
            copyColor();
            if (debug > 1) {
                // write Image
                sprintf(buf, "/storage/sdcard0/imgc.txt");
                //sprintf(buf, "/storage/emulated/0/imgc.txt");
                //readImage(kernels["readImage"], mems["imgc"], w_img, h_img, true,buf);
            }
        }
        time_end =  currentTimeInNanos();
        print_out("Time taken for copyColor : %f ms",double(time_end-time_start)/1000000);

        // debug
        print_out("refining corners\n");
        int flag = 0;
        GLManager::getRefinedCorners(locMat, frameMetadataF, frameMetadataI, frameNo,
                                     true, debug, trackMultiPattern);
        time_end =  currentTimeInNanos();
        print_out("Time taken for refining corners: %f ms",double(time_end-time_start)/1000000);

        for (int i = 0; i < 6; i++)
            if (frameMetadataI.at<int>(i, 2) != -1) {
                flag = 1;
                break;
            }
        if (flag == 0) {
            for (int i = 0; i < 6; i++) {
                frameMetadataF.at<float>(i, 0) = -1;
                frameMetadataI.at<int>(i, 0) = -1;
                frameMetadataI.at<int>(i, 1) = -1;
            }

            // convert Color
            print_out("color conversion\n");
            colorConversion(kernels["colConversionGL"], mems["img"], w_img,
                                       h_img, false, "/storage/sdcard0/img.txt");
            //print_out("image: %d %d",w_img, h_img);
            time_end =  currentTimeInNanos();
            print_out("Time taken for col conversion : %f ms",double(time_end-time_start)/1000000);

            // get color purity in mems["purity"]
            print_out("color purity\n");
            getColorPurity(kernels["getColorPurity"], mems["purity"],
                                      w_img, h_img, false, "/storage/sdcard0/purity.txt");
            time_end =  currentTimeInNanos();
            print_out("Time taken for purity : %f ms",double(time_end-time_start)/1000000);

            // get Corners in mems["cornersNew"]
            print_out("block corners\n");

            getBlockCorners(kernels["cornersZero"],
                                       kernels["getBlockCorners"], kernels["refineCorners"],
                                       mems["img"], mems["purity"], mems["corners"],
                                       mems["cornersNew"], w_img, h_img, false,
                                       "/storage/sdcard0/corners10.txt",
                                       "/storage/sdcard0/corners10_2.txt");
            time_end =  currentTimeInNanos();
            print_out("Time taken for corners : %f ms",double(time_end-time_start)/1000000);

            // get corners into locmat
            float *xCorners;
            float *yCorners;
            int *votes;
            int nCorners;
            print_out("voting\n");
            extractCornersAndPatternVotes(kernels["resetNCorners"],
                                                     kernels["getNCorners"], kernels["getCorners"],
                                                     kernels["getLinePtAssignment"], mems["img"], mems["cornersNew"],
                                                     &xCorners, &yCorners, &votes, nCorners, w_img, h_img, false,
                                                     "/storage/sdcard0/lines.txt");
            time_end =  currentTimeInNanos();
            print_out("Time taken for votes : %f ms",double(time_end-time_start)/1000000);

            print_out("points\n");
            getPatternPoints(xCorners, yCorners, votes, nCorners, locMat,
                                        w_img, h_img, frameMetadataF, frameMetadataI, trackMultiPattern,
                                        frameNo, debug, false, "/storage/sdcard0/lines.txt");
            time_end =  currentTimeInNanos();
            print_out("Time taken for patterns : %f ms",double(time_end-time_start)/1000000);

        }

        // debug
        if (debug > 1) {
            strcat(buf, "_pts.bin");
            FILE *fid = fopen(buf, "w");
            fwrite((float*) locMat.data, sizeof(float), 18, fid);
            fclose(fid);
        }
        // debug

        time_end =  currentTimeInNanos();
        print_out("Time taken : %f ms",double(time_end-time_start)/1000000);
        return;//omp_get_wtime() - start;
    }

    void GLManager::validateCorners(std::vector<int> &id_pts, float *xCorners,
                                    float *yCorners) {

        int id_valid[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
        float x0, y0, x1, x2, y1, y2, d;
        for (int i = 0; i < 3; i++) {
            // horizontal
            if (id_pts[i] != -1 && id_pts[3 + i] != -1 && id_pts[6 + i] != -1) {
                x0 = xCorners[id_pts[i]];
                y0 = yCorners[id_pts[i]];
                x1 = xCorners[id_pts[3 + i]];
                y1 = yCorners[id_pts[3 + i]];
                x2 = xCorners[id_pts[6 + i]];
                y2 = yCorners[id_pts[6 + i]];

                d = std::abs((y2 - y0) * x1 - (x2 - x0) * y1 + x2 * y0 - y2 * x0)
                    / std::sqrt((y2 - y0) * (y2 - y0) + (x2 - x0) * (x2 - x0));
                if (d < 7 && (x1 - x0) * (x1 - x2) <= 49
                    && (y1 - y0) * (y1 - y2) <= 49) { // x1 and y1 are between x0 and x2, and y0 and y2
                    id_valid[i] = 1;
                    id_valid[3 + i] = 1;
                    id_valid[6 + i] = 1;
                }
            }

            //vertical
            if (id_pts[3 * i] != -1 && id_pts[3 * i + 1] != -1
                && id_pts[3 * i + 2] != -1) {
                x0 = xCorners[id_pts[3 * i]];
                y0 = yCorners[id_pts[3 * i]];
                x1 = xCorners[id_pts[3 * i + 1]];
                y1 = yCorners[id_pts[3 * i + 1]];
                x2 = xCorners[id_pts[3 * i + 2]];
                y2 = yCorners[id_pts[3 * i + 2]];

                d = std::abs((y2 - y0) * x1 - (x2 - x0) * y1 + x2 * y0 - y2 * x0)
                    / std::sqrt((y2 - y0) * (y2 - y0) + (x2 - x0) * (x2 - x0));
                if (d < 7 && (x1 - x0) * (x1 - x2) <= 49
                    && (y1 - y0) * (y1 - y2) <= 49) { // x1 and y1 are between x0 and x2, and y0 and y2
                    id_valid[3 * i] = 1;
                    id_valid[3 * i + 1] = 1;
                    id_valid[3 * i + 2] = 1;
                }
            }
        }

        // check diagonals
        if (id_pts[0] != -1 && id_pts[4] != -1 && id_pts[8] != -1) {
            x0 = xCorners[id_pts[0]];
            y0 = yCorners[id_pts[0]];
            x1 = xCorners[id_pts[4]];
            y1 = yCorners[id_pts[4]];
            x2 = xCorners[id_pts[8]];
            y2 = yCorners[id_pts[8]];

            d = std::abs((y2 - y0) * x1 - (x2 - x0) * y1 + x2 * y0 - y2 * x0)
                / std::sqrt((y2 - y0) * (y2 - y0) + (x2 - x0) * (x2 - x0));
            if (d < 7) {
                id_valid[0] = 1;
                id_valid[4] = 1;
                id_valid[8] = 1;
            }
        }
        if (id_pts[2] != -1 && id_pts[4] != -1 && id_pts[6] != -1) {
            x0 = xCorners[id_pts[2]];
            y0 = yCorners[id_pts[2]];
            x1 = xCorners[id_pts[4]];
            y1 = yCorners[id_pts[4]];
            x2 = xCorners[id_pts[6]];
            y2 = yCorners[id_pts[6]];

            d = std::abs((y2 - y0) * x1 - (x2 - x0) * y1 + x2 * y0 - y2 * x0)
                / std::sqrt((y2 - y0) * (y2 - y0) + (x2 - x0) * (x2 - x0));
            if (d < 7) {
                id_valid[2] = 1;
                id_valid[4] = 1;
                id_valid[6] = 1;
            }
        }

        for (int i = 0; i < 9; i++)
            if (id_valid[i] == 0 || id_valid[4] == 0)
                id_pts[i] = -1;

        //get distances from center
        std::vector<float> dist(9);
        std::vector<float> dist_const(9);
        if (id_pts[4] == -1) {
            for (int i = 1; i < 9; i++) {
                id_pts[i] = -1;
            }
            return;
        }
        float xm = xCorners[id_pts[4]];
        float ym = yCorners[id_pts[4]];
        for (int i = 0; i < 9; i++) {
            dist[i] = 0;
            if (id_pts[i] != -1) {
                dist[i] = sqrt(
                        (xCorners[id_pts[i]] - xm) * (xCorners[id_pts[i]] - xm)
                        + (yCorners[id_pts[i]] - ym)
                          * (yCorners[id_pts[i]] - ym));
            }
        }

        for (int i = 1; i < 9; i++)
            dist_const[i] = dist[i];

        std::sort(dist.begin(), dist.end());
        float th_dist = 500000;
        for (int i = 1; i < 9; i++) {
            if (dist[i - 1] != 0) {
                if (dist[i] / dist[i - 1] > 3) {
                    th_dist = dist[i];
                    break;
                }
            }
        }
        for (int i = 1; i < 9; i++) {
            if (dist_const[i] >= th_dist)
                id_pts[i] = -1;
        }
    }

    cv::Mat GLManager::getBoundingQuad(cv::Mat &locMat_this) {
        cv::Mat quad;
        quad = cv::Mat::zeros(4, 2, CV_32FC1);
        float x[4], y[4];
        x[0] = locMat_this.at<float>(0, 0)
               + (locMat_this.at<float>(0, 0) - locMat_this.at<float>(4, 0)) / 2.0;
        x[1] = locMat_this.at<float>(6, 0)
               + (locMat_this.at<float>(6, 0) - locMat_this.at<float>(4, 0)) / 2.0;
        x[2] = locMat_this.at<float>(8, 0)
               + (locMat_this.at<float>(8, 0) - locMat_this.at<float>(4, 0)) / 2.0;
        x[3] = locMat_this.at<float>(2, 0)
               + (locMat_this.at<float>(2, 0) - locMat_this.at<float>(4, 0)) / 2.0;

        y[0] = locMat_this.at<float>(0, 1)
               + (locMat_this.at<float>(0, 1) - locMat_this.at<float>(4, 1)) / 2.0;
        y[1] = locMat_this.at<float>(6, 1)
               + (locMat_this.at<float>(6, 1) - locMat_this.at<float>(4, 1)) / 2.0;
        y[2] = locMat_this.at<float>(8, 1)
               + (locMat_this.at<float>(8, 1) - locMat_this.at<float>(4, 1)) / 2.0;
        y[3] = locMat_this.at<float>(2, 1)
               + (locMat_this.at<float>(2, 1) - locMat_this.at<float>(4, 1)) / 2.0;

        for (int i = 0; i < 4; i++) {
            quad.at<float>(i, 0) = x[i];
            quad.at<float>(i, 1) = y[i];
        }

        return quad;
    }

    bool GLManager::getPatternPoints(float *xCorners, float *yCorners, int *votes,
                                     int nCorners, cv::Mat &locMat, size_t w_img, size_t h_img,
                                     cv::Mat &frameMetadataF, cv::Mat &frameMetadataI,
                                     bool trackMultiPattern, int frameNo, int debug, bool saveOutput,
                                     const char *fname) {
            int ret, err;
            // assessing votes
            uchar valid_ptSel[] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
                                    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0,
                                    0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1,
                                    1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 0, 1, 0, 0,
                                    0, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1,
                                    1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0,
                                    0, 0, 0, 0, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 0, 1, 0,
                                    1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1,
                                    0, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0,
                                    0, 1, 1, 1, 1, 1, 1, 0, 1, 0, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                    1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 0, 1,
                                    1, 1, 0, 0, 0, 0, 0, 0, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0,
                                    0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 0, 0, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1,
                                    1, 1, 1, 1, 0, 0, 0, 1, 0, 1, 1, 1, 0, 0, 1, 1, 1, 1, 1, 1, 0, 0, 1,
                                    1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1,
                                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0,
                                    0, 0, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 1, 1, 1, 1,
                                    1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 0, 0, 1, 0, 1,
                                    1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                    1, 1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1 };

            // detect class and purity of each detected point
            int *ptClass = (int *) malloc(nCorners * sizeof(int));
            float *ptPurity = (float *) malloc(nCorners * sizeof(float));
            int *ptValid = (int*) malloc(nCorners * sizeof(int));

            for (int i = 0; i < nCorners; i++)
                ptClass[i] = -1;
            for (int i = 0; i < nCorners; i++)
                ptPurity[i] = 0;
            for (int i = 0; i < nCorners; i++)
                ptValid[i] = 1;

            for (int i = 0; i < nCorners; i++) {
                int id_max = -1;
                int maxvote = -1;
                for (int j = 0; j < 9; j++) {
                    print_out("votes[%d,%d]=%d",i,j,votes[i * 9 + j]);
                    if (votes[i * 9 + j] > maxvote) {
                        maxvote = votes[i * 9 + j];
                        id_max = j;
                    }
                }

                // getting the second best
                int id_max2 = -1;
                int maxvote2 = -1;
                for (int j = 0; j < 9; j++) {
                    if (j == id_max)
                        continue;
                    if (votes[i * 9 + j] > maxvote2) {
                        maxvote2 = votes[i * 9 + j];
                        id_max2 = j;
                    }
                }

                ptPurity[i] = maxvote - maxvote2;
                ptClass[i] = id_max;
            }

            int Pow2[9];
            Pow2[0] = 1;
            Pow2[1] = 2;
            Pow2[2] = 4;
            Pow2[3] = 8;
            Pow2[4] = 16;
            Pow2[5] = 32;
            Pow2[6] = 64;
            Pow2[7] = 128;
            Pow2[8] = 256;
            cl_uchar *ptValidity = (cl_uchar *) malloc(nCorners * sizeof(cl_uchar));
            for (int i = 0; i < nCorners; i++) {
                ptValidity[i] = 0;
            }

            print_out("points: %d", nCorners);
    //        for (int t = 0; t < nCorners; t++) {
    //            print_out("purity[%d]=%f , class=%d",t,ptPurity[t], ptClass[t]);
    //        }

            // center point is class-4
            int count_patterns = 0;
            for (int t = 0; t < nCorners; t++) {
                if (count_patterns >= 6)
                    break;

                if (ptClass[t] == 4 && ptPurity[t] >= 1) {
                    float ptDist[9];
                    std::vector<int> ptLoc(9);
                    for (int i = 0; i < 9; i++) {
                        ptLoc[i] = -1;
                        ptDist[i] = 50000;
                    }
                    ptLoc[4] = t;

                    for (int j = 0; j < nCorners; j++) {
                        if (ptClass[j] != 4 && ptClass[j] != -1 && ptPurity[j] >= 1) {
                            float d = std::abs(xCorners[j] - xCorners[t])
                                      + std::abs(yCorners[j] - yCorners[t]);
                            if (d < ptDist[ptClass[j]] + 5) {
                                if (ptLoc[ptClass[j]] == -1) {
                                    ptDist[ptClass[j]] = d;
                                    ptLoc[ptClass[j]] = j;
                                } else {
                                    if (d < ptDist[ptClass[j]] - 7
                                        || ptPurity[j]
                                           > ptPurity[ptLoc[ptClass[j]]]) {
                                        ptDist[ptClass[j]] = d;
                                        ptLoc[ptClass[j]] = j;
                                    }
                                }
                            }
                        }
                    }

                    print_out("ptLoc: %d %d %d  %d %d %d  %d %d %d",ptLoc[0],ptLoc[1],ptLoc[2],ptLoc[3],ptLoc[4],ptLoc[5],ptLoc[6],ptLoc[7],ptLoc[8]);
                    validateCorners(ptLoc, xCorners, yCorners);
                    print_out("ptLoc: %d %d %d  %d %d %d  %d %d %d",ptLoc[0],ptLoc[1],ptLoc[2],ptLoc[3],ptLoc[4],ptLoc[5],ptLoc[6],ptLoc[7],ptLoc[8]);

                    int id_selection = 0;
                    for (int i = 0; i < 9; i++) {
                        if (ptLoc[i] != -1) {
                            id_selection += Pow2[i];
                        }
                    }

                    if (valid_ptSel[id_selection] == 1) {
                        cv::Mat locMat_this;
                        locMat_this = cv::Mat::zeros(9, 2, CV_32FC1);
                        for (int i = 0; i < 9; i++) {
                            if (ptLoc[i] != -1) {
                                locMat_this.at<float>(i, 0) = yCorners[ptLoc[i]];
                                locMat_this.at<float>(i, 1) = xCorners[ptLoc[i]];
                            } else {
                                locMat_this.at<float>(i, 0) = -1;
                                locMat_this.at<float>(i, 1) = -1;
                            }
                        }

                        float err_reproj_max = 10;
                        // get homography and find missing points
                        print_out("9pts-1");
                        for (int i = 0; i < 9; i++)
                            print_out("%f %f", locMat_this.at<float>(i, 0),
                                      locMat_this.at<float>(i, 1));

                        err = getReprojectionAndErrorForPattern(locMat_this,
                                                                           err_reproj_max, false);
                        print_out("9pts-repr");
                        for (int i = 0; i < 9; i++)
                            print_out("%f %f", locMat_this.at<float>(i, 0),
                                      locMat_this.at<float>(i, 1));

                        cv::Mat frameMetadataF_this;
                        frameMetadataF_this = cv::Mat::zeros(1, 1, CV_32FC1); //frameMetadataF(cv::Rect(0,count_patterns,1,1));
                        cv::Mat frameMetadataI_this;
                        frameMetadataI_this = cv::Mat::zeros(1, 2, CV_32FC1); //frameMetadataI(cv::Rect(0,count_patterns,1,1));
                        frameMetadataF_this.at<float>(0, 0) = -1;
                        frameMetadataI_this.at<int>(0, 0) = -1;
                        frameMetadataI_this.at<int>(0, 1) = -1;
                        //print_out("metadata 1: %d %d %f ",frameMetadataI_this.at<int>(0,0), frameMetadataI_this.at<int>(0,1), frameMetadataF_this.at<float>(0,0));
                        if (err <= err_reproj_max) {
                            getRefinedCornersForPattern(locMat_this,
                                                                   frameMetadataF_this, frameMetadataI_this, frameNo,
                                                                   true, -1, debug);
                            //print_out("refining finished");
                            //print_out("9pts-refine");
                            //for(int i=0;i<9;i++)print_out("%f %f",locMat_this.at<float>(i,0),locMat_this.at<float>(i,1));
                        }

                        // cleanup
                        if (frameMetadataI_this.at<int>(0, 0) != -1) {
                            for (int i = 0; i < 9; i++) {
                                locMat.at<float>(
                                        frameMetadataI_this.at<int>(0, 0) * 9 + i, 0) =
                                        locMat_this.at<float>(i, 0);
                                locMat.at<float>(
                                        frameMetadataI_this.at<int>(0, 0) * 9 + i, 1) =
                                        locMat_this.at<float>(i, 1);
                            }

                            frameMetadataF.at<float>(frameMetadataI_this.at<int>(0, 0),
                                                     0) = frameMetadataF_this.at<float>(0, 0);
                            frameMetadataI.at<int>(frameMetadataI_this.at<int>(0, 0), 0) =
                                    frameMetadataI_this.at<int>(0, 0);
                            frameMetadataI.at<int>(frameMetadataI_this.at<int>(0, 0), 1) =
                                    -1;
                            count_patterns++;

                            if ((trackMultiPattern && count_patterns == 2)
                                || (!trackMultiPattern && count_patterns == 1))
                                break;

                            cv::Mat quad = getBoundingQuad(locMat_this);
                            //for(int i=0;i<4;i++)print_out("quad %f %f",quad.at<float>(i,0),quad.at<float>(i,1));

                            for (int i = 0; i < nCorners; i++) {
                                if (ptPurity[i] >= 1) {
                                    if (isInsideQuad(quad, yCorners[i], xCorners[i])) {
                                        ptPurity[i] = 0;
                                    }
                                }
                            }

                        }
                    }
                }
            }

            // check for the most relevant configuration
            print_out("countPatterns before: %d", count_patterns);
            if ((trackMultiPattern && count_patterns < 2)
                || (!trackMultiPattern && count_patterns < 1)) {
                GLManager::findNewPattern(ptPurity, ptClass, count_patterns, locMat,
                                          nCorners, Pow2, valid_ptSel, xCorners, yCorners, frameNo, debug,
                                          frameMetadataF, frameMetadataI);
            }

            print_out("countPatterns: %d", count_patterns);
            /*
             print_out("54pts-refine");
             for(int i=0;i<54;i++)
             {
             print_out("%f %f",locMat.at<float>(i,0),locMat.at<float>(i,1));
             }
             */
            if (saveOutput) {
                FILE *fid = fopen("/storage/sdcard0/votePurity.txt", "w");
                for (int i = 0; i < nCorners; i++)
                    fprintf(fid, "%f %d\n", ptPurity[i], ptClass[i]);
                fclose(fid);

                // votes
                fid = fopen("/storage/sdcard0/votes.txt", "w");
                for (int i = 0; i < nCorners; i++) {
                    for (int j = 0; j < 9; j++) {
                        fprintf(fid, "%d ", votes[i * 9 + j]);
                    }
                    fprintf(fid, "\n");
                }
                fclose(fid);
            }

            // free
            free(ptValidity);
            free(ptPurity);
            free(ptClass);
            free(votes);
            free(xCorners);
            free(yCorners);
            free(ptValid);
            print_out("countPatterns 2: %d", count_patterns);
            return true;
    }

    void GLManager::findNewPattern(float *ptPurity, int *ptClass, int &count_patterns,
                                   cv::Mat &locMat, int nCorners, int Pow2[], uchar valid_ptSel[],
                                   float *xCorners, float *yCorners, int frameNo, int debug,
                                   cv::Mat &frameMetadataF, cv::Mat &frameMetadataI) {
        int err;
        std::vector < std::vector<int> > classCandidates(9);
        for (int t = 0; t < nCorners; t++) {
            if (ptPurity[t] >= 1) {
                classCandidates[ptClass[t]].push_back(t);
            }
        }

        for (int i = 0; i < 9; i++)
            if (classCandidates[i].size() == 0)
                classCandidates[i].push_back(-1);

        print_out("#class: %d %d %d  %d %d %d  %d %d %d", classCandidates[0].size(),
                  classCandidates[1].size(), classCandidates[2].size(),
                  classCandidates[3].size(), classCandidates[4].size(),
                  classCandidates[5].size(), classCandidates[6].size(),
                  classCandidates[7].size(), classCandidates[8].size());

        int id_selection = 0;
        for (int i = 0; i < 9; i++) {
            if (classCandidates[i][classCandidates[i].size() - 1] != -1)
                id_selection += Pow2[i];
        }
        if (valid_ptSel[id_selection] == 0)
            return;

        int count = 0;
        std::vector<int> ptLoc(9);
        int id[9], sel[9];
        for (id[0] = 0; id[0] < classCandidates[0].size(); id[0]++)
            for (id[1] = 0; id[1] < classCandidates[1].size(); id[1]++)
                for (id[2] = 0; id[2] < classCandidates[2].size(); id[2]++)
                    for (id[3] = 0; id[3] < classCandidates[3].size(); id[3]++)
                        for (id[4] = 0; id[4] < classCandidates[4].size(); id[4]++)
                            for (id[5] = 0; id[5] < classCandidates[5].size();
                                 id[5]++)
                                for (id[6] = 0; id[6] < classCandidates[6].size();
                                     id[6]++)
                                    for (id[7] = 0;
                                         id[7] < classCandidates[7].size();
                                         id[7]++)
                                        for (id[8] = 0;
                                             id[8] < classCandidates[8].size();
                                             id[8]++) {
                                            count++;

                                            if (count >= 10)
                                                return;

                                            for (int i = 0; i < 9; i++) {
                                                ptLoc[i] =
                                                        classCandidates[i][id[i]];
                                            }
                                            GLManager::validateCorners(ptLoc,
                                                                       xCorners, yCorners);
                                            id_selection = 0;
                                            for (int i = 0; i < 9; i++) {
                                                if (ptLoc[i] != -1)
                                                    id_selection += Pow2[i];
                                            }
                                            if (valid_ptSel[id_selection] == 0)
                                                continue;
                                            print_out("found-valid-inside");

                                            if (valid_ptSel[id_selection] == 1) {
                                                cv::Mat locMat_this;
                                                locMat_this = cv::Mat::zeros(9, 2,CV_32FC1);
                                                for (int i = 0; i < 9; i++) {
                                                    if (ptLoc[i] != -1) {
                                                        locMat_this.at<float>(i, 0) =
                                                                yCorners[ptLoc[i]];
                                                        locMat_this.at<float>(i, 1) =
                                                                xCorners[ptLoc[i]];
                                                    } else {
                                                        locMat_this.at<float>(i, 0) =
                                                                -1;
                                                        locMat_this.at<float>(i, 1) =
                                                                -1;
                                                    }
                                                }

                                                float err_reproj_max = 10;
                                                // get homography and find missing points
                                                print_out("9pts-1 in");
                                                for (int i = 0; i < 9; i++)
                                                    print_out("%f %f",
                                                              locMat_this.at<float>(i,
                                                                                    0),
                                                              locMat_this.at<float>(i,
                                                                                    1));

                                                err =
                                                        GLManager::getReprojectionAndErrorForPattern(
                                                                locMat_this,
                                                                err_reproj_max,
                                                                false);
                                                print_out("9pts-repr in");
                                                for (int i = 0; i < 9; i++)
                                                    print_out("%f %f",
                                                              locMat_this.at<float>(i,
                                                                                    0),
                                                              locMat_this.at<float>(i,
                                                                                    1));

                                                cv::Mat frameMetadataF_this;
                                                frameMetadataF_this = cv::Mat::ones(1, 1,CV_32FC1) * -1;
                                                cv::Mat frameMetadataI_this;
                                                frameMetadataI_this = cv::Mat::ones(1, 2,CV_32SC1) * -1;
                                                if (err <= err_reproj_max) {
                                                    GLManager::getRefinedCornersForPattern(
                                                            locMat_this,
                                                            frameMetadataF_this,
                                                            frameMetadataI_this,
                                                            frameNo, true, -1,
                                                            debug);
                                                    print_out("9pts-refine in");
                                                    for (int i = 0; i < 9; i++)
                                                        print_out("%f %f",
                                                                  locMat_this.at<float>(
                                                                          i, 0),
                                                                  locMat_this.at<float>(
                                                                          i, 1));
                                                }

                                                // cleanup
                                                if (frameMetadataI_this.at<int>(0,
                                                                                0) != -1) {
                                                    cv::Mat quad = getBoundingQuad(
                                                            locMat_this);

                                                    for (int i = 0; i < nCorners;
                                                         i++) {
                                                        if (ptPurity[i] >= 1) {
                                                            if (isInsideQuad(quad,
                                                                             yCorners[i],
                                                                             xCorners[i])) {
                                                                ptPurity[i] = 0;
                                                            }
                                                        }
                                                    }
                                                    for (int i = 0; i < 9; i++)
                                                        classCandidates[i].resize(
                                                                0);
                                                    for (int t = 0; t < nCorners;
                                                         t++) {
                                                        if (ptPurity[t] >= 1) {
                                                            classCandidates[ptClass[t]].push_back(
                                                                    t);
                                                        }
                                                    }
                                                    for (int i = 0; i < 9; i++)
                                                        if (classCandidates[i].size()
                                                            == 0)
                                                            classCandidates[i].push_back(
                                                                    -1);

                                                    for (int i = 0; i < 9; i++) {
                                                        locMat.at<float>(
                                                                frameMetadataI_this.at<
                                                                        int>(0, 0)
                                                                * 9 + i, 0) =
                                                                locMat_this.at<float>(
                                                                        i, 0);
                                                        locMat.at<float>(
                                                                frameMetadataI_this.at<
                                                                        int>(0, 0)
                                                                * 9 + i, 1) =
                                                                locMat_this.at<float>(
                                                                        i, 1);
                                                    }
                                                    frameMetadataF.at<float>(
                                                            frameMetadataI_this.at<
                                                                    int>(0, 0), 0) =
                                                            frameMetadataF_this.at<
                                                                    float>(0, 0);
                                                    frameMetadataI.at<int>(
                                                            frameMetadataI_this.at<
                                                                    int>(0, 0), 0) =
                                                            frameMetadataI_this.at<
                                                                    int>(0, 0);
                                                    frameMetadataI.at<int>(
                                                            frameMetadataI_this.at<
                                                                    int>(0, 0), 1) =
                                                            -1;
                                                    count_patterns++;
                                                    return;
                                                }
                                            }
                                        }
    }

    bool GLManager::isInsideQuad(cv::Mat &quad, float x, float y) {
        for (int i = 0; i < quad.rows; i++) {
            float vx = quad.at<float>((i + 1) % 4, 0) - quad.at<float>(i, 0);
            float vy = quad.at<float>((i + 1) % 4, 1) - quad.at<float>(i, 1);
            float pvx = x - quad.at<float>(i, 0);
            float pvy = y - quad.at<float>(i, 1);

            if (vx * pvy - vy * pvx < 0)
                return false;
        }
        return true;
    }

    bool GLManager::extractCornersAndPatternVotes(cl_kernel & knl_resetNCorners,
                                                  cl_kernel & knl_getNCorners, cl_kernel & knl_getCorners,
                                                  cl_kernel &knl_getLinePtAssignment, cl_mem &memobj_in,
                                                  cl_mem &memobj_corners, float **pxCorners, float **pyCorners,
                                                  int **pvotes, int &nCorners, size_t w_img, size_t h_img,
                                                  bool saveOutput, const char *fname) {
        //get number of points
        int ret;
        int err;

        // initialize corners
        cl_mem memobj_nCorners;
        int inVals[1];
        inVals[0] = 0;
        createBuffer_SSBO_mem(memobj_nCorners, 1 * sizeof(int),inVals);
        glFinish();

        nCorners = 0;

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, memobj_corners);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, memobj_nCorners);
        glUseProgram(knl_getNCorners);assertNoGLErrors("using program");
        glDispatchCompute(w_img/4, h_img/2, 1);assertNoGLErrors("dispatch compute");
        glFinish();

        glBindBuffer( GL_SHADER_STORAGE_BUFFER, memobj_nCorners );
        int *data_gpu= (int *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, 1 * sizeof(int), GL_MAP_READ_BIT);
        std::memcpy( &nCorners,data_gpu, 1 * sizeof(int));
        glFinish();
        glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
        glFinish();

        print_out("nCorners:%d",nCorners);

        // reset memobj_nCorners
        inVals[0] = 0;
        glBindBuffer( GL_SHADER_STORAGE_BUFFER, memobj_nCorners );
        data_gpu= (int *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, 1 * sizeof(int), GL_MAP_WRITE_BIT|GL_MAP_INVALIDATE_BUFFER_BIT);
        std::memcpy( data_gpu, inVals, 1 * sizeof(int));
        glFinish();
        glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
        glFinish();

        // allocating memory for pairwise equations
        float *xCorners = (float *) malloc(nCorners * sizeof(float));
        float *yCorners = (float *) malloc(nCorners * sizeof(float));

        // create memory objects for pts
        cl_mem memobj_cornersX, memobj_cornersY;
        createBuffer_SSBO(memobj_cornersX, nCorners * sizeof(float));
        createBuffer_SSBO(memobj_cornersY, nCorners * sizeof(float));
        glFinish();

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, memobj_cornersX);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, memobj_cornersY);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, memobj_corners);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, memobj_nCorners);
        glUseProgram(knl_getCorners);assertNoGLErrors("using program");
        glDispatchCompute(w_img/4, h_img/2, 1);assertNoGLErrors("dispatch compute");
        glFinish();

        int deg_eq = 27; // to store some intermediate and debug information

        int *votes = (int*) malloc(nCorners * 9 * sizeof(int)); // for each detected corner, how many votes each of the 9 pattern corners get
        float *eqns = (float*) malloc(
                nCorners * nCorners * deg_eq * sizeof(float));

        for (int i = 0; i < nCorners * 9; i++)
            votes[i] = 0;

        print_out("debug 7 nCorners:%d\n",nCorners);
        // allocate nCorners*9 array indicating votes for each point
        cl_mem memobj_cornersVote, memobj_lineEqns;
        createBuffer_SSBO_mem(memobj_cornersVote, nCorners * 9 * sizeof(int), votes);
        createBuffer_SSBO(memobj_lineEqns, nCorners * nCorners * deg_eq * sizeof(int));
        glFinish();

        if(nCorners>0){
            // get point assignments
            cl_mem memobj_inVals;
            int inVals2[2];
            inVals2[0] = h_img;
            inVals2[1] = w_img;
            createBuffer_SSBO_mem(memobj_inVals, 2 * sizeof(int),inVals2);
            glFinish();


            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, memobj_cornersVote);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, memobj_in);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, memobj_lineEqns);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, memobj_cornersY);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, memobj_cornersX);
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, memobj_inVals);
            glUseProgram(knl_getLinePtAssignment);assertNoGLErrors("using program");
            glDispatchCompute(nCorners, nCorners, 1);assertNoGLErrors("dispatch compute");
            glFinish();
            glDeleteBuffers(1,&memobj_inVals);

        // read back corners

            glBindBuffer( GL_SHADER_STORAGE_BUFFER, memobj_lineEqns );
            float * data_gpuf= (float *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, nCorners * nCorners * deg_eq * sizeof(float), GL_MAP_READ_BIT);
            std::memcpy( eqns, data_gpuf, nCorners * nCorners * deg_eq * sizeof(float));
            glFinish();
            glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
            glFinish();

            glBindBuffer( GL_SHADER_STORAGE_BUFFER, memobj_cornersVote );
            data_gpu= (int *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, nCorners * 9 * sizeof(int), GL_MAP_READ_BIT);
            std::memcpy( votes, data_gpu, nCorners * 9 * sizeof(int));
            glFinish();
            glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
            glFinish();

            glBindBuffer( GL_SHADER_STORAGE_BUFFER, memobj_cornersX );
            data_gpuf= (float *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, nCorners * sizeof(float), GL_MAP_READ_BIT);
            std::memcpy( xCorners, data_gpuf, nCorners * sizeof(float));
            glFinish();
            glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
            glFinish();

            glBindBuffer( GL_SHADER_STORAGE_BUFFER, memobj_cornersY );
            data_gpuf= (float *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, nCorners * sizeof(float), GL_MAP_READ_BIT);
            std::memcpy( yCorners, data_gpuf, nCorners * sizeof(float));
            glFinish();
            glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
            glFinish();
        }

        print_out("debug 10\n");

        *pxCorners = xCorners;
        *pyCorners = yCorners;
        *pvotes = votes;

        if (saveOutput) {
            FILE *fid = fopen("/storage/sdcard0/corners.txt", "w");
            for (int i = 0; i < nCorners; i++)
                fprintf(fid, "%f %f\n", xCorners[i], yCorners[i]);
            fclose(fid);

            fid = fopen("/storage/sdcard0/lineEqns.txt", "w");
            for (int i = 0; i < nCorners * nCorners; i++) {
                for (int j = 0; j < deg_eq; j++) {
                    fprintf(fid, "%f ", eqns[i * deg_eq + j]);
                }
                fprintf(fid, "\n");
            }
            fclose(fid);
        }

        free(eqns);

        print_out("debug 11\n");

        glDeleteBuffers(1,&memobj_lineEqns);
        glDeleteBuffers(1,&memobj_cornersVote);
        glDeleteBuffers(1,&memobj_cornersX);
        glDeleteBuffers(1,&memobj_cornersY);
        glDeleteBuffers(1,&memobj_nCorners);

        return true;
    }

    int GLManager::getBlockCorners(cl_kernel &knl_cornersZero,
                                   cl_kernel &knl_getBlockCorners, cl_kernel &knl_refineCornerPoints,
                                   cl_mem &memobj_in, cl_mem &memobj_purity, cl_mem &memobj_corners,
                                   cl_mem & memobj_cornersNew, int w_img, int h_img, bool saveOutput,
                                   const char *fname, const char *fname2) {
        int ret;
        int err;

        // initialize corners
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, memobj_corners);
        glUseProgram(knl_cornersZero);assertNoGLErrors("using program");
        glDispatchCompute(w_img/4, h_img/2, 1);assertNoGLErrors("dispatch compute");
        glFinish();

        cl_mem mem_params_input;
        int inVals[1];
        inVals[0] = m_sz_blk;
        createBuffer_SSBO_mem(mem_params_input, 1 * sizeof(int),inVals);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, memobj_in);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, memobj_corners);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, memobj_purity);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, mem_params_input);
        glUseProgram(knl_getBlockCorners);assertNoGLErrors("using program");
        glDispatchCompute(w_img/m_sz_blk/4, h_img/m_sz_blk /2, 1);assertNoGLErrors("dispatch compute");
        glFinish();
        glDeleteBuffers(1,&mem_params_input);

        // refine corner points
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, memobj_cornersNew);
        glUseProgram(knl_cornersZero);assertNoGLErrors("using program");
        glDispatchCompute(w_img/4, h_img/2, 1);assertNoGLErrors("dispatch compute");
        glFinish();

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, memobj_corners);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, memobj_cornersNew);
        glUseProgram(knl_refineCornerPoints);assertNoGLErrors("using program");
        glDispatchCompute(w_img/4, h_img/2, 1);assertNoGLErrors("dispatch compute");
        glFinish();

        if (saveOutput) {
            int *tempVec = (int*) malloc(w_img * h_img * sizeof(int));
            glBindBuffer( GL_SHADER_STORAGE_BUFFER, memobj_corners );
            int *data_gpu= (int *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, w_img * h_img * sizeof(int), GL_MAP_READ_BIT);
            std::memcpy( tempVec,data_gpu, w_img * h_img * sizeof(int));
            glFinish();
            glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
            glFinish();

            FILE *fid = fopen(fname, "w");
            for (int i = 0; i < w_img * h_img; i++)
                fprintf(fid, "%d ", (int) tempVec[i]);
            fclose(fid);

            glBindBuffer( GL_SHADER_STORAGE_BUFFER, memobj_cornersNew );
            data_gpu= (int *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, w_img * h_img * sizeof(int), GL_MAP_READ_BIT);
            std::memcpy( tempVec,data_gpu, w_img * h_img * sizeof(int));
            glFinish();
            glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
            glFinish();

            fid = fopen(fname2, "w");
            for (int i = 0; i < w_img * h_img; i++)
                fprintf(fid, "%d ", (int) tempVec[i]);
            fclose(fid);
            free(tempVec);
        }

        return 1;
    }


    bool GLManager::getColorPurity(cl_kernel &knl_purity, cl_mem &memobj_purity,
                                   int w_img, int h_img, bool saveOutput, const char *fname) {
        int err, ret;

        size_t w_purity = w_img / m_sz_blk;
        size_t h_purity = h_img / m_sz_blk;
        int skip = 2;

        cl_mem mem_params_input;
        int inVals[2];
        inVals[0] = m_sz_blk;
        inVals[1] = skip;
        createBuffer_SSBO_mem(mem_params_input, 2 * sizeof(int),inVals);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, memobj_purity);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mem_params_input);
        glUseProgram(kernels["getColorPurity"]);assertNoGLErrors("using program");
        glDispatchCompute(w_purity/4, h_purity/2, 1);assertNoGLErrors("dispatch compute");
        glFinish();
        glDeleteBuffers(1,&mem_params_input);

        if (saveOutput) {
            cl_uchar *tempVec = (cl_uchar*) malloc(
                    w_purity * h_purity * sizeof(cl_uchar));

            glBindBuffer( GL_SHADER_STORAGE_BUFFER, memobj_purity );
            cl_uchar *data_gpu= (cl_uchar *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, w_purity * h_purity * sizeof(cl_uchar), GL_MAP_READ_BIT);
            std::memcpy( tempVec,data_gpu, w_purity * h_purity * sizeof(cl_uchar));
            glFinish();
            glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
            glFinish();

            FILE *fid = fopen(fname, "w");
            for (int i = 0; i < w_purity * h_purity; i++)
                fprintf(fid, "%d ", (int) tempVec[i]);
            fclose(fid);

            char fname_bin[256];
            strcpy(fname_bin, fname);
            strcat(fname_bin, ".bin");
            fid = fopen(fname_bin, "w");
            fwrite(tempVec, sizeof(cl_uchar), w_purity * h_purity, fid);
            fclose(fid);

            free(tempVec);
        }

        return true;
    }

bool GLManager::colorConversion(cl_kernel &knl_colConversion, cl_mem &memobj_in,
                                int w_img, int h_img, bool saveOutput, const char *fname) {
    // comment:
    int n_pixels = img_size.x * img_size.y;
    int err;
        // color conversion kernel
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, memobj_in);
        glUseProgram(knl_colConversion);assertNoGLErrors("using program");
        glDispatchCompute(w_img/4, h_img/2, 1);assertNoGLErrors("dispatch compute");
        glFinish();

    if (saveOutput) {
        cl_uchar *tempVec = (cl_uchar*) malloc(
                w_img * h_img * sizeof(cl_uchar));

        glBindBuffer( GL_SHADER_STORAGE_BUFFER, memobj_in );
        cl_uchar *data_gpu= (cl_uchar *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, w_img * h_img * sizeof(cl_uchar), GL_MAP_READ_BIT);
        std::memcpy( tempVec,data_gpu, w_img * h_img * sizeof(cl_uchar));
        glFinish();
        glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );
        glFinish();

        FILE *fid = fopen(fname, "w");
        for (int i = 0; i < w_img * h_img; i++)
            fprintf(fid, "%d ", (int) tempVec[i]);
        fclose(fid);

        char fname_bin[256];
        strcpy(fname_bin, fname);
        strcat(fname_bin, ".bin");
        fid = fopen(fname_bin, "w");
        fwrite(tempVec, sizeof(cl_uchar), w_img * h_img, fid);
        fclose(fid);

        free(tempVec);
    }

    return true;
}

    void GLManager::initParameters(GLuint in_tex, GLuint out_tex, int width, int height){
    }

    bool GLManager::initGL(GLuint in_tex, GLuint out_tex, int width, int height) {
        input_texture = in_tex;
        output_texture = out_tex;

        img_size.x = width;
        img_size.y = height;
        m_sz_blk = 5;
        setCornerRefinementParameters();

        // kernels
        initializeGPUKernels();

        // memory
        initializeGPUMemoryBuffers();

        return true;
    }


    void GLManager::setParamsCenterToCorner(int mLocRecFromCornerData[][7]) {
        // 0-> id of the base corner point relative to which this location will be computed
        // 1-> line id (first index) of the horizontal mid point which will give the horizontal shift
        // 2-> pt id (second index) of the horizontal mid point which will give the horizontal shift
        // 3-> line id (first index) of the vertical mid point which will give the vertical shift
        // 4-> pt id (second index) of the vertical mid point which will give the vertical shift
        // 5-> 1 if desired horz. direction is [base point] - [horz. mid pt], -1 if it is [horz. mid pt] - [base point]
        // 6-> 1 if desired vert. direction is [base point] - [vert. mid pt], -1 if it is [vert. mid pt] - [base point]
        mLocRecFromCornerData[0][0] = 0;
        mLocRecFromCornerData[0][1] = 0;
        mLocRecFromCornerData[0][2] = 0;
        mLocRecFromCornerData[0][3] = 3;
        mLocRecFromCornerData[0][4] = 0;
        mLocRecFromCornerData[0][5] = 1;
        mLocRecFromCornerData[0][6] = 1;

        mLocRecFromCornerData[1][0] = 1;
        mLocRecFromCornerData[1][1] = 1;
        mLocRecFromCornerData[1][2] = 0;
        mLocRecFromCornerData[1][3] = 3;
        mLocRecFromCornerData[1][4] = 0;
        mLocRecFromCornerData[1][5] = 1;
        mLocRecFromCornerData[1][6] = -1;

        mLocRecFromCornerData[2][0] = 2;
        mLocRecFromCornerData[2][1] = 2;
        mLocRecFromCornerData[2][2] = 0;
        mLocRecFromCornerData[2][3] = 3;
        mLocRecFromCornerData[2][4] = 1;
        mLocRecFromCornerData[2][5] = 1;
        mLocRecFromCornerData[2][6] = -1;

        mLocRecFromCornerData[3][0] = 2;
        mLocRecFromCornerData[3][1] = 2;
        mLocRecFromCornerData[3][2] = 0;
        mLocRecFromCornerData[3][3] = 3;
        mLocRecFromCornerData[3][4] = 1;
        mLocRecFromCornerData[3][5] = 1;
        mLocRecFromCornerData[3][6] = 1;

        mLocRecFromCornerData[4][0] = 3;
        mLocRecFromCornerData[4][1] = 0;
        mLocRecFromCornerData[4][2] = 0;
        mLocRecFromCornerData[4][3] = 4;
        mLocRecFromCornerData[4][4] = 0;
        mLocRecFromCornerData[4][5] = -1;
        mLocRecFromCornerData[4][6] = 1;

        mLocRecFromCornerData[5][0] = 4;
        mLocRecFromCornerData[5][1] = 1;
        mLocRecFromCornerData[5][2] = 0;
        mLocRecFromCornerData[5][3] = 4;
        mLocRecFromCornerData[5][4] = 0;
        mLocRecFromCornerData[5][5] = -1;
        mLocRecFromCornerData[5][6] = -1;

        mLocRecFromCornerData[6][0] = 5;
        mLocRecFromCornerData[6][1] = 2;
        mLocRecFromCornerData[6][2] = 0;
        mLocRecFromCornerData[6][3] = 4;
        mLocRecFromCornerData[6][4] = 1;
        mLocRecFromCornerData[6][5] = -1;
        mLocRecFromCornerData[6][6] = -1;

        mLocRecFromCornerData[7][0] = 5;
        mLocRecFromCornerData[7][1] = 2;
        mLocRecFromCornerData[7][2] = 0;
        mLocRecFromCornerData[7][3] = 4;
        mLocRecFromCornerData[7][4] = 1;
        mLocRecFromCornerData[7][5] = -1;
        mLocRecFromCornerData[7][6] = 1;

        mLocRecFromCornerData[8][0] = 6;
        mLocRecFromCornerData[8][1] = 0;
        mLocRecFromCornerData[8][2] = 1;
        mLocRecFromCornerData[8][3] = 5;
        mLocRecFromCornerData[8][4] = 0;
        mLocRecFromCornerData[8][5] = -1;
        mLocRecFromCornerData[8][6] = 1;

        mLocRecFromCornerData[9][0] = 7;
        mLocRecFromCornerData[9][1] = 1;
        mLocRecFromCornerData[9][2] = 1;
        mLocRecFromCornerData[9][3] = 5;
        mLocRecFromCornerData[9][4] = 0;
        mLocRecFromCornerData[9][5] = -1;
        mLocRecFromCornerData[9][6] = -1;

        mLocRecFromCornerData[10][0] = 8;
        mLocRecFromCornerData[10][1] = 2;
        mLocRecFromCornerData[10][2] = 1;
        mLocRecFromCornerData[10][3] = 5;
        mLocRecFromCornerData[10][4] = 1;
        mLocRecFromCornerData[10][5] = -1;
        mLocRecFromCornerData[10][6] = -1;

        mLocRecFromCornerData[11][0] = 8;
        mLocRecFromCornerData[11][1] = 2;
        mLocRecFromCornerData[11][2] = 1;
        mLocRecFromCornerData[11][3] = 5;
        mLocRecFromCornerData[11][4] = 1;
        mLocRecFromCornerData[11][5] = -1;
        mLocRecFromCornerData[11][6] = 1;

        mLocRecFromCornerData[12][0] = 6;
        mLocRecFromCornerData[12][1] = 0;
        mLocRecFromCornerData[12][2] = 1;
        mLocRecFromCornerData[12][3] = 5;
        mLocRecFromCornerData[12][4] = 0;
        mLocRecFromCornerData[12][5] = 1;
        mLocRecFromCornerData[12][6] = 1;

        mLocRecFromCornerData[13][0] = 7;
        mLocRecFromCornerData[13][1] = 1;
        mLocRecFromCornerData[13][2] = 1;
        mLocRecFromCornerData[13][3] = 5;
        mLocRecFromCornerData[13][4] = 0;
        mLocRecFromCornerData[13][5] = 1;
        mLocRecFromCornerData[13][6] = -1;

        mLocRecFromCornerData[14][0] = 8;
        mLocRecFromCornerData[14][1] = 2;
        mLocRecFromCornerData[14][2] = 1;
        mLocRecFromCornerData[14][3] = 5;
        mLocRecFromCornerData[14][4] = 1;
        mLocRecFromCornerData[14][5] = 1;
        mLocRecFromCornerData[14][6] = -1;

        mLocRecFromCornerData[15][0] = 8;
        mLocRecFromCornerData[15][1] = 2;
        mLocRecFromCornerData[15][2] = 1;
        mLocRecFromCornerData[15][3] = 5;
        mLocRecFromCornerData[15][4] = 1;
        mLocRecFromCornerData[15][5] = 1;
        mLocRecFromCornerData[15][6] = 1;
    }

    void GLManager::getCrossIds(int crossIds[]) {
        int count = 0;
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 4; j++) {
                crossIds[2 * count] = j * 4 + i;
                crossIds[2 * count + 1] = j * 4 + i + 1;
                count++;
            }
        }
        for (int i = 0; i < 3; i++) {
            for (int j = 0; j < 4; j++) {
                crossIds[2 * count] = i * 4 + j;
                crossIds[2 * count + 1] = i * 4 + j + 4;
                count++;
            }
        }
    }

    bool GLManager::setCornerRefinementParameters() {
        setParamsCenterToCorner(cornerParams.mLocRecFromCornerData);
        getCrossIds(cornerParams.crossIDs);

        cornerParams.ptsTM = cv::Mat::ones(9, 1, CV_32FC2);
        cornerParams.ptsTM_homo = cv::Mat::ones(9, 3, CV_32FC1);
        int nCorners = 0;
        for (int j = 0; j < 3; j++) {
            for (int i = 0; i < 3; i++) {
                cornerParams.ptsTM.at < cv::Vec2f > (nCorners) = cv::Vec2f(
                        6 * (j + 1), 4 * (i + 1));
                cornerParams.ptsTM_homo.at<float>(nCorners, 0) = 6 * (j + 1);
                cornerParams.ptsTM_homo.at<float>(nCorners++, 1) = 4 * (i + 1);
            }
        }
        return true;
    }

    bool GLManager::copyColor() {

        int w_img = img_size.x;
        int h_img = img_size.y;

        // copy to output Frame
        glUseProgram(kernels["copyInGL"]);assertNoGLErrors("using program");
        glDispatchCompute(w_img/4, h_img/2, 1);assertNoGLErrors("dispatch compute");
        glFinish();

        return true;
    }

    void GLManager::initializeGPUKernels(){
        createShader("colConversionGL", colConversion_kernel);
        createShader("getColorPurity", getColorPurity_kernel);
        createShader("copyInGL", copyInGL_kernel);
        createShader("getTransValidity", getTransValidity_kernel);
        createShader("cornersZero", cornersZero_kernel);
        createShader("getBlockCorners", getBlockCorners_kernel);
        createShader("refineCorners", refineCorners_kernel);
        createShader("getLineCrossing", getLineCrossing_kernel);
        createShader("getPatternIndicator", getPatternIndicator_kernel);
        createShader("plotCorners", plotCorners_kernel);
        createShader("markDetectedCorners", markDetectedCorners_kernel);
        createShader("getNCorners", getNCorners_kernel);
        createShader("getCorners", getCorners_kernel);
        createShader("getLinePtAssignment", getLinePtAssignment_kernel);
    }

    void GLManager::initializeGPUMemoryBuffers(){
        createBuffer("img", img_size.x * img_size.y * sizeof(int));

        // RGB image
        createBuffer("imgc", img_size.x * img_size.y * sizeof(unsigned char) * 3);

        // color purity
        createBuffer("purity", (img_size.x / m_sz_blk) * (img_size.y / m_sz_blk) * sizeof(int));

        // corner indicator
        createBuffer("corners",img_size.x * img_size.y * sizeof(int));

        // refined corner indicator
        createBuffer("cornersNew",img_size.x * img_size.y * sizeof(int));

        // size variable 1
        createBuffer("sz1",sizeof(int));

        // size variable 2
        createBuffer("sz2",2*sizeof(int));

        // size variable 3
        createBuffer("sz3",3*sizeof(int));

        // size variable 4
        createBuffer("sz4",4*sizeof(int));

        // val variable 1
        createBuffer("v1",sizeof(float));

        // val variable 2
        createBuffer("v2",2*sizeof(float));

        // val variable 3
        createBuffer("v3",3*sizeof(float));

        // val variable 4
        createBuffer("v4",4*sizeof(float));

        // val variable 5
        createBuffer("v5",5*sizeof(float));

        // corner refinement parameters
        createBuffer_SSBO_mem(cornerParams.mem_crossIds, 24 * 2 * sizeof(int), cornerParams.crossIDs);
        createBuffer_SSBO(cornerParams.mem_16Pts, 16 * 2 * sizeof(float));
        createBuffer_SSBO(cornerParams.mem_crossPts, 24 * 2 * sizeof(float));
        createBuffer_SSBO(cornerParams.mem_col16Pts, 16 * 3 * sizeof(float));
        glFinish();
    }

    void GLManager::createBuffer(const char * str, int size){
        cl_mem ssbo;
        createBuffer_SSBO(ssbo, size);
        mems[str] = ssbo;
    }

    void GLManager::createBuffer_SSBO(cl_mem &ssbo, int size){
        glGenBuffers(1, &ssbo);assertNoGLErrors("create buffer 1");
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);assertNoGLErrors("create buffer 2");
        glBufferData(GL_SHADER_STORAGE_BUFFER,size, NULL, GL_STATIC_DRAW);assertNoGLErrors("create buffer 3");
    }

    void GLManager::createBuffer_SSBO_mem(cl_mem &ssbo, int size, void *memptr){
        glGenBuffers(1, &ssbo);assertNoGLErrors("create buffer 1 mem");
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo);assertNoGLErrors("create buffer 2 mem");
        glBufferData(GL_SHADER_STORAGE_BUFFER,size, memptr, GL_STATIC_DRAW);assertNoGLErrors("create buffer 3 mem");
    }

    bool GLManager::createShader(const char *progName, const char * kernelString){
        print_out(progName);
        cl_kernel compute_prog = glCreateProgram();
        assertNoGLErrors("create program");

        GLint shader = glCreateShader(GL_COMPUTE_SHADER);
        assertNoGLErrors("create shader");
        const GLchar* sources = { kernelString };
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
            return false;
        }

        glAttachShader(compute_prog, shader);
        assertNoGLErrors("attach shader");
        glLinkProgram(compute_prog);
        assertNoGLErrors("link shader");
        ALOGV("Program linked");

        kernels[progName] = compute_prog;
        return true;
    }

    void GLManager::releaseGL() {

    }
}
