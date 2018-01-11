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
            int id_best = frameMetadataI.at<int>(i, 2);
            if (id_best >= 0) {
                cv::Mat locMat_this = locMat(cv::Rect(0, id_best * nPtsPat, 2, nPtsPat));

                int patID_prev = frameMetadataI.at<int>(id_best,0);
                print_out("Tracking for %d", patID_prev);
                GLManager::getRefinedCornersForPattern(locMat_this, frameMetadataF.at<float>(id_best,0), frameMetadataI.at<int>(id_best,0),
                                                       checkTrans, patID_prev, debug, frameNo);
                print_out("refining finished 1");
                if (frameMetadataI.at<int>(id_best, 0) == patID_prev) {
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

        int nx_check = 5;//2
        int ny_check = 5;//2
        int nt_check = 2;
        int n_check = (2 * nx_check + 1) * (2 * ny_check + 1) * (2 * nt_check + 1);

        int * transValidity = (int*) malloc(n_check * sizeof(int));

        cl_mem mem_transValidity;
        createBuffer_SSBO(mem_transValidity, n_check * sizeof(int));

        cl_mem mem_inputVals;
        float inVals[9];
        inVals[0] = x_mid;
        inVals[1] = y_mid;
        inVals[2] = angle;
        inVals[3] = d_w;
        inVals[4] = d_h;
        inVals[5] = n_check;
        inVals[6] = nx_check;
        inVals[7] = ny_check;
        inVals[8] = nt_check;
        createBuffer_SSBO_mem(mem_inputVals, 9 * sizeof(float),inVals);

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mem_transValidity);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, mem_16Pts);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, mem_inputVals);
        glUseProgram(kernels["getTransValidity"]);assertNoGLErrors("using program");
        int n_workgroups = std::ceil(n_check/128.0f);
        glDispatchCompute(n_workgroups, 1, 1);assertNoGLErrors("dispatch compute");
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
                                                float &intensity,
                                                int &patID,
                                                bool checkTrans, int patId_prev, int debug, int frameNo) {
        cv::Mat locMat_old;
        locMat.copyTo(locMat_old);

        int err, ret;

        int w_img = img_size.x;
        int h_img = img_size.y;

        cv::Mat mLocations;
        mLocations = cv::Mat::zeros(16, 2, CV_32FC1);
        getCentersFromCorners(mLocations, locMat, cornerParams.mLocRecFromCornerData);

        glBindBuffer( GL_SHADER_STORAGE_BUFFER, cornerParams.mem_16Pts );
        float *points = (float *) glMapBufferRange( GL_SHADER_STORAGE_BUFFER, 0, 16 * 2 * sizeof(float), GL_MAP_WRITE_BIT|GL_MAP_INVALIDATE_BUFFER_BIT );
        std::memcpy( points, mLocations.data, 16 * 2 * sizeof(float));
        glFinish();
        glUnmapBuffer( GL_SHADER_STORAGE_BUFFER );

        if (checkTrans) {
            bool ptsValid = getValidTrans(mLocations, locMat,
                                          cornerParams.mem_16Pts);

            if (!ptsValid) {
                patID=-1;
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
        err = getReprojectionAndErrorForPattern(locMat, 10, true, cornerParams);
        getPatternIdAndIntensity(locMat, patID, intensity, patId_prev, debug);

        // write points on image
        // debug

            print_out("running end marking\n");
            if (patID == -1)
                return false;

            // marking detected points
            print_out("running marking\n");
            cl_mem mem_9Pts;
            createBuffer_SSBO_mem(mem_9Pts,9 * 2 * sizeof(float),locMat.data);
            glFinish();


            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, mem_9Pts);
        if (debug > 0) {
            glUseProgram(kernels["markDetectedCorners"]);
        }else{
            glUseProgram(kernels["markDetectedCornersInput"]);
        }
        assertNoGLErrors("using program");
            glDispatchCompute(9, 1, 1);assertNoGLErrors("dispatch compute");
            glFinish();

            glDeleteBuffers(1,&mem_9Pts);

// debug

        return true;
    }

    bool GLManager::getPatternIdAndIntensity(cv::Mat &locMat, int &patID,
                                             float &intensity, int patId_prev, int debug) {
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

        getPatternIdAndIntensityFromGrayVals(patternGrayVal, patId_prev, patID, intensity);


            int patternId = patID;
            if (patternId == -1)
                return true;

            cl_mem mem_12Pts2;
            createBuffer_SSBO_mem(mem_12Pts2,12 * 2 * sizeof(float),endPtsXY.data);
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
        if (debug > 0) {
            glUseProgram(kernels["plotCorners"]);
        }else{
            glUseProgram(kernels["plotCornersInput"]);
        }
        assertNoGLErrors("using program");
            glDispatchCompute(12, 1, 1);assertNoGLErrors("dispatch compute");
            glFinish();

            glDeleteBuffers(1,&mem_12Pts2);
            glDeleteBuffers(1,&mem_colVals);

        return true;
    }

    void GLManager::processFrame(cv::Mat &locMat, cv::Mat &frameMetadataF,
                                 cv::Mat &frameMetadataI, int frameNo, double dt,
                                 bool trackMultiPattern) {

        long time_start, time_end, total_time_start, total_time_end;
        time_start =  currentTimeInNanos();
        total_time_start =  currentTimeInNanos();
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
            time_start =  currentTimeInNanos();
            copyColor();
            time_end =  currentTimeInNanos();
            print_out("Time taken for copyColor : %f ms",double(time_end-time_start)/1000000);
        }

        // debug
        print_out("refining corners\n");
        int flag = 0;
        time_start =  currentTimeInNanos();
        GLManager::getRefinedCorners(locMat, frameMetadataF, frameMetadataI, frameNo, true, debug, trackMultiPattern);
        time_end =  currentTimeInNanos();
        print_out("Time taken for refining corners: %f ms",double(time_end-time_start)/1000000);

        int count_tracked=0;
        for (int i = 0; i < 6; i++)
            if (frameMetadataI.at<int>(i, 2) != -1) {
                //flag = 1;
                count_tracked++;
                //break;
            }
        flag=0;
        if(trackMultiPattern && count_tracked>=2)flag=1;
        if(!trackMultiPattern && count_tracked>=1)flag=1;
        if (flag == 0) {
            for (int i = 0; i < 6; i++) {
                frameMetadataF.at<float>(i, 0) = -1;
                frameMetadataI.at<int>(i, 0) = -1;
                frameMetadataI.at<int>(i, 1) = -1;
            }

            // convert Color
            print_out("color conversion\n");
            time_start =  currentTimeInNanos();
            colorConversion(kernels["colConversionGL"], mems["img"], w_img,
                                       h_img, false, "/storage/sdcard0/img.txt");
            //print_out("image: %d %d",w_img, h_img);
            time_end =  currentTimeInNanos();
            print_out("Time taken for col conversion : %f ms",double(time_end-time_start)/1000000);

            // get color purity in mems["purity"]
            print_out("color purity\n");
            time_start =  currentTimeInNanos();
            getColorPurity(kernels["getColorPurity"], mems["purity"],
                                      w_img, h_img, false, "/storage/sdcard0/purity.txt");
            time_end =  currentTimeInNanos();
            print_out("Time taken for purity : %f ms",double(time_end-time_start)/1000000);

            // get Corners in mems["cornersNew"]
            print_out("block corners\n");
            time_start =  currentTimeInNanos();
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
            time_start =  currentTimeInNanos();
            extractCornersAndPatternVotes(kernels["resetNCorners"],
                                                     kernels["getNCorners"], kernels["getCorners"],
                                                     kernels["getLinePtAssignment"], mems["img"], mems["cornersNew"],
                                                     &xCorners, &yCorners, &votes, nCorners, w_img, h_img, false,
                                                     "/storage/sdcard0/lines.txt");
            time_end =  currentTimeInNanos();
            print_out("Time taken for votes : %f ms",double(time_end-time_start)/1000000);

            time_start =  currentTimeInNanos();
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

        total_time_end =  currentTimeInNanos();
        print_out("Time taken : %f ms",double(total_time_end-total_time_start)/1000000);
        return;//omp_get_wtime() - start;
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
                    //print_out("votes[%d,%d]=%d",i,j,votes[i * 9 + j]);
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

                        err = getReprojectionAndErrorForPattern(locMat_this,err_reproj_max, false, cornerParams);
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
                            getRefinedCornersForPattern(locMat_this, frameMetadataF_this.at<float>(0,0), frameMetadataI_this.at<int>(0,0), true, -1, debug,frameNo);
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
                                            validateCorners(ptLoc,
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

                                                err =getReprojectionAndErrorForPattern(
                                                                locMat_this,
                                                                err_reproj_max,
                                                                false, cornerParams);
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
                                                            frameMetadataF_this.at<float>(0,0),
                                                            frameMetadataI_this.at<int>(0,0), true, -1,
                                                            debug,
                                                            frameNo);
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
        glDispatchCompute(w_img/128, h_img/8, 1);assertNoGLErrors("dispatch compute");
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
        glDispatchCompute(w_img/128, h_img/8, 1);assertNoGLErrors("dispatch compute");
        glFinish();

        int deg_eq = 27; // to store some intermediate and debug information

        int *votes = (int*) malloc(nCorners * 9 * sizeof(int)); // for each detected corner, how many votes each of the 9 pattern corners get
        float *eqns = (float*) malloc(
                nCorners * nCorners * deg_eq * sizeof(float));

        for (int i = 0; i < nCorners * 9; i++)
            votes[i] = 0;

        //print_out("nCorners:%d\n",nCorners);
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

        //print_out("debug 10\n");

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

        //print_out("debug 11\n");

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
        glDispatchCompute(w_img/128, h_img/8, 1);assertNoGLErrors("dispatch compute");
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
        glDispatchCompute(w_img/m_sz_blk/128, h_img/m_sz_blk /8, 1);assertNoGLErrors("dispatch compute");
        glFinish();
        glDeleteBuffers(1,&mem_params_input);

        // refine corner points
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, memobj_cornersNew);
        glUseProgram(knl_cornersZero);assertNoGLErrors("using program");
        glDispatchCompute(w_img/128, h_img/8, 1);assertNoGLErrors("dispatch compute");
        glFinish();

        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, memobj_corners);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, memobj_cornersNew);
        glUseProgram(knl_refineCornerPoints);assertNoGLErrors("using program");
        glDispatchCompute(w_img/128, h_img/8, 1);assertNoGLErrors("dispatch compute");
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
        glDispatchCompute(w_purity/128, h_purity/8, 1);assertNoGLErrors("dispatch compute");
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
    long time_end;

    // comment:
    int n_pixels = img_size.x * img_size.y;
    int err;
        // color conversion kernel
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, memobj_in);
        glUseProgram(knl_colConversion);assertNoGLErrors("using program");
        glDispatchCompute(w_img/64, h_img/8, 1);assertNoGLErrors("dispatch compute");
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
        setCornerRefinementParameters(cornerParams);

        // kernels
        initializeGPUKernels();

        // memory
        initializeGPUMemoryBuffers();

        return true;
    }

    bool GLManager::copyColor() {

        int w_img = img_size.x;
        int h_img = img_size.y;

        // copy to output Frame
        glUseProgram(kernels["copyInGL"]);assertNoGLErrors("using program");
        glDispatchCompute(w_img/128, h_img/8, 1);assertNoGLErrors("dispatch compute");
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
        createShader("plotCornersInput", plotCornersInput_kernel);
        createShader("markDetectedCorners", markDetectedCorners_kernel);
        createShader("markDetectedCornersInput", markDetectedCornersInput_kernel);
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
