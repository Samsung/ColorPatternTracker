#include <stddef.h>
#include <stdlib.h>
#include <sys/time.h>
#include <exception>
#include <stdexcept>

#include <time.h>

#include "patternUtil.h"
#include "util.h"

namespace JNICLTracker{

	void getCentersFromCorners(cv::Mat &mLocations, cv::Mat &locMat,
										  int mLocRecFromCornerData[][7]){
		{
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
	}

    void getPatternIdAndIntensityFromGrayVals(float indicator[], int patId_prev, int &id_code, float &intensity) {
        const int nCodes = 6;
        const int nBits = 12;

        int codes[nCodes][nBits] = {{0, 0, 1, 0, 0, 0, 1, 3, 2, 1, 0, 1},
                                    {0, 0, 1, 0, 1, 0, 0, 3, 2, 0, 3, 1},
                                    {0, 3, 0, 0, 0, 2, 1, 3, 2, 0, 3, 0},
                                    {0, 3, 0, 0, 1, 2, 0, 3, 2, 1, 0, 0},
                                    {0, 3, 1, 0, 0, 2, 0, 0, 0, 1, 3, 1},
                                    {0, 3, 1, 0, 1, 2, 1, 0, 0, 0, 0, 1}};

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

        id_code = -1;
        int id_th = -1;
        int nValidBits = 0;

        for (int i = 0; i < nBits; i++)
            if (indicator[i] != -1)
                nValidBits++;

        if (nValidBits < nBits - 3) {
            intensity = -1;
            return;
        }

        const int nTh = 6;
        float th_vec[nTh] = {0.1, 0.15, 0.2, 0.25, 0.3, 0.35};

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

        intensity = 0;
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
            if (count > 0)intensity /= count;
            //if (count == 0)id_code = -1;
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
    }

    float getReprojectionAndErrorForPattern(cv::Mat &locMat, float err_max,
                                                       bool isComplete, CornerRefinementParam &cornerParams) {

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

    void getCornersFromCrossPts(cv::Mat &locMat, float crossPts[]) {
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

    void validateCorners(std::vector<int> &id_pts, float *xCorners, float *yCorners) {

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

    cv::Mat getBoundingQuad(cv::Mat &locMat_this) {
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

    bool isInsideQuad(cv::Mat &quad, float x, float y) {
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

    void setParamsCenterToCorner(int mLocRecFromCornerData[][7]) {
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

    void getCrossIds(int crossIds[]) {
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

    bool setCornerRefinementParameters(CornerRefinementParam &cornerParams) {
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
}
