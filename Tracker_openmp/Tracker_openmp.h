#pragma once

#include <stdlib.h>
#include <vector>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
#include <Windows.h>
#include <omp.h>

void colConversionGL(unsigned char *image_in, std::vector<unsigned char> &D, int w, int h, float th_black, int nChannels);
void getColorPurity(unsigned char *input_image, std::vector<unsigned char> &P, int sz_blk, int skip, int w, int h, float th_black, int nChannels);
void getBlockCorners10(std::vector<unsigned char> &D, std::vector<unsigned char> &C, std::vector<unsigned char> &purity, int sz_step, int w, int h);

void checkRimCorner(std::vector<unsigned char> &D, int idx, int idy, int sz_x, int r_rim, uchar *pcol1, uchar *pcol2, uchar *pcol3);
bool checkRimCornerBool(std::vector<unsigned char> &D, int idx, int idy, int sz_x, int r_rim);
void jointDetect(std::vector<unsigned char> &D, size_t *px, size_t *py, int w, int h, int sz_x);
void shrinkBox(std::vector<unsigned char> &D, size_t *px, size_t *py, int w, int h, int sz_x);

void refineCorners(std::vector<unsigned char> &C, std::vector<unsigned char> &CNew, int w, int h);

void getLinePtAssignment(cv::Mat &img_CV, std::vector<int> &ptsVote, std::vector<unsigned char> &D, std::vector<float> &lineEqns, std::vector<float> &x, std::vector<float> &y, int w_img, int h_img);

void binarySearch(int x1, int x2, int y1, int y2, int *x_ret, int *y_ret, int w, int h, std::vector<unsigned char> &D);

bool getPatternPoints(std::vector<float> &xCorners, std::vector<float> &yCorners, std::vector<int> &votes, std::vector<int> &ptLoc, int nCorners, size_t w_img, size_t h_img);

unsigned char getColVal_colMajor(int x, int y, int w, int h, std::vector<unsigned char> &D);
void validateCorners(std::vector<int> &id_pts, std::vector<float> &xCorners, std::vector<float> &yCorners, float x_mid, float y_mid);
void runProcess_batchImage(char *fname, char *fname_out, int w, int h);
void saveGPUDataAsImage(char *fname, char *fname_im, int w, int h);
void createIdealPattern(cv::Mat &patImg);
void showPointsAndVotes(cv::Mat &img_CV, std::vector<int> &votes, std::vector<float> &xCorners, std::vector<float> &yCorners);
void loadGPUDataAsImage(char *fname, int w, int h, cv::Mat &img_CV);
void displayCornersOnImage(cv::Mat &corners, cv::Mat &img_CV, char *fname_out);

void extractCorners(cv::Mat &img_CV, cv::Mat &corners);

void processImage(cv::Mat &im, char *fname_corners, char *fname_outImg);
void writeCorners(char *fname_corners, cv::Mat &corners);

void batchEvaluate();
const int m_sz_blk = 5;

struct PixelF {
	float x, y, z;
};

cv::Mat PatternImg;

struct ElFSort {
	float v;
	int id;
};

bool fSortFunctionDescending(ElFSort i, ElFSort j);