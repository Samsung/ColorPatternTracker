// Tracker_openmp.cpp : Defines the entry point for the console application.
//

// TODO: votes casting, Non-4 nomination starting

#include "Tracker_openmp.h"

int main(int argc, char ** argv)
{
	cv::Mat pat;
	createIdealPattern(pat);

	if (argc == 1) {
		printf("Usage: Tracker_openmp bmp_input_file [bmp_output_file] \n bmp_output_file has keypoints marked. By default keypoints detected are stored in bmp_input_file.txt file.");
	}
	
	char fname_out[1024];
	if (argc == 3) {
		strcpy(fname_out, argv[2]);
	}
	else {
		sprintf(fname_out, "");
	}

	// load image
	char fname_corners[256];
	sprintf(fname_corners, "%s.txt", argv[1]);
	cv::Mat im = cv::imread(argv[1]);
	cv::cvtColor(im, im, cv::COLOR_RGB2BGR);
	processImage(im, fname_corners, fname_out);

    return 0;
}

void processImage(cv::Mat &im, char *fname_corners, char *fname_outImg) {
	cv::Mat corners;
	extractCorners(im, corners);
	writeCorners(fname_corners, corners);
	if (strcmp(fname_outImg, "") != 0) {
		displayCornersOnImage(corners, im, fname_outImg);
	}
}

void extractCorners(cv::Mat &img_CV, cv::Mat &corners) {
	double ts, te;
	int w = img_CV.cols;
	int h = img_CV.rows;

	float th_black = 20;

	std::vector<unsigned char> D(w * h);
	std::vector<unsigned char> C(w * h);
	std::vector<unsigned char> CNew(w * h);
	std::vector<unsigned char> P(w / m_sz_blk * h / m_sz_blk);

	cv::Mat img_gray;
	cv::cvtColor(img_CV, img_gray, CV_BGR2GRAY);

	// color conversion
	colConversionGL(img_CV.data, D, w, h, th_black, img_CV.channels());
	cv::Mat D_cv(cv::Size(w, h), CV_8UC1, D.data());

	// color purity
	int skip = 2;
	getColorPurity(img_CV.data, P, m_sz_blk, skip, w, h, th_black, img_CV.channels());
	cv::Mat P_cv(cv::Size(w / m_sz_blk, h / m_sz_blk), CV_8UC1, P.data());
	cv::Mat P_resize;
	cv::resize(P_cv, P_resize, cv::Size(img_CV.cols, img_CV.rows));

	//  get block corners
	getBlockCorners10(D, C, P, m_sz_blk, w, h);
	cv::Mat C_cv(cv::Size(w, h), CV_8UC1, C.data());

	// refine corners
	refineCorners(C, CNew, w, h);
	cv::Mat CNew_cv(cv::Size(w, h), CV_8UC1, CNew.data());

	// get corners
	ts = omp_get_wtime();
	int nCorners = 0;
	std::vector<float> xCorners;
	std::vector<float> yCorners;
	xCorners.reserve(1000);
	yCorners.reserve(1000);

	for (int i = 0; i < w; i++) {
		for (int j = 0; j < h; j++) {
			if (CNew[j*w + i] > 0) {
				xCorners.push_back(i);
				yCorners.push_back(j);
			}
		}
	}
	nCorners = xCorners.size();
	te = omp_get_wtime();
	printf("Corners read in %lf ms\n", (te - ts) * 1000);

	int deg_eq = 27; // to store some intermediate and debug information

	ts = omp_get_wtime();
	std::vector<int>votes(nCorners * 9); // for each detected corner, how many votes each of the 9 pattern corners get
	std::vector<float>eqns(nCorners * nCorners * deg_eq);

	for (int i = 0; i < nCorners * 9; i++)
		votes[i] = 0;

	te = omp_get_wtime();
	printf("votes alloc in %lf ms\n", (te - ts) * 1000);

	ts = omp_get_wtime();
	getLinePtAssignment(img_CV, votes, D, eqns, yCorners, xCorners, h, w);
	te = omp_get_wtime();
	printf("votes in %lf ms\n", (te - ts) * 1000);

	// show points and votes
	//showPointsAndVotes(img_CV, votes, yCorners, xCorners);

	std::vector<int> ptLoc(9);
	ts = omp_get_wtime();
	getPatternPoints(xCorners, yCorners, votes, ptLoc, nCorners, w, h);

	corners = cv::Mat::zeros(9, 2, CV_32FC1);
	for (int i = 0; i < 9; i++) {
		if (ptLoc[i] != -1) {
			corners.at<float>(i, 0) = xCorners[ptLoc[i]];
			corners.at<float>(i, 1) = yCorners[ptLoc[i]];
		}
		else {
			corners.at<float>(i, 0) = -1;
			corners.at<float>(i, 1) = -1;
		}
	}
}

bool fSortFunctionDescending(ElFSort i, ElFSort j) { return (i.v>j.v); }

void batchEvaluate() {
	// Flow control
	unsigned char doWriteImages = 0;
	unsigned char doProcessImages = 1;

	int w = 1920;
	int h = 1080;

	for (int i = 593; i < 594; i++) {
		char fname[1024];
		sprintf(fname, "C:/Research/data/Tracker/imgc%d.bin", i);

		if (doWriteImages) {
			char fname_im[1024];
			sprintf(fname_im, "C:/Research/data/Tracker/imgc%d.bmp", i);
			saveGPUDataAsImage(fname, fname_im, w, h);
		}

		if (doProcessImages) {
			printf("Running %s\n", fname);
			char fname_out[1024];
			sprintf(fname_out, "C:/Research/data/Tracker/resultImages/imgc%d.jpg", i);
			runProcess_batchImage(fname, fname_out, w, h);
			printf("Ran %s\n", fname);
		}
	}
}

void loadGPUDataAsImage(char *fname, int w, int h, cv::Mat &img_CV) {
	std::vector<unsigned char> image_in_t(w * h * 4);
	std::vector<unsigned char> input_image(w * h * 4);

	// load image
	FILE *fid = fopen(fname, "rb");
	fread(image_in_t.data(), 1, w * h * 4, fid);
	fclose(fid);

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			input_image[j * h * 4 + i * 4 + 0] = image_in_t[(h - i - 1) * w * 4 + j * 4 + 2];
			input_image[j * h * 4 + i * 4 + 1] = image_in_t[(h - i - 1) * w * 4 + j * 4 + 1];
			input_image[j * h * 4 + i * 4 + 2] = image_in_t[(h - i - 1) * w * 4 + j * 4 + 0];
			input_image[j * h * 4 + i * 4 + 3] = image_in_t[(h - i - 1) * w * 4 + j * 4 + 3];
		}
	}

	img_CV = cv::Mat(cv::Size(h, w), CV_8UC4, input_image.data());
}

void saveGPUDataAsImage(char *fname, char *fname_im, int w, int h) {
	cv::Mat img_CV;
	loadGPUDataAsImage(fname, w, h, img_CV);
	
	cv::imwrite(fname_im, img_CV);
}

// create Ideal Pattern
void createIdealPattern(cv::Mat &patImg) {
	int w_pat = 600;
	int h_pat = 400;
	patImg = cv::Mat::zeros(h_pat, w_pat, CV_8UC3);

	int colset[4][4];

	colset[0][0] = 1;
	colset[0][1] = 2;
	colset[0][2] = 3;
	colset[0][3] = 1;

	colset[1][0] = 3;
	colset[1][1] = 1;
	colset[1][2] = 2;
	colset[1][3] = 3;

	colset[2][0] = 1;
	colset[2][1] = 2;
	colset[2][2] = 3;
	colset[2][3] = 1;

	colset[3][0] = 2;
	colset[3][1] = 3;
	colset[3][2] = 1;
	colset[3][3] = 2;

	uchar linet[6][4];
	uchar lineb[6][4];

	// colors for top and bottom of each of the horizontal and vertical lines
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 4; j++) {
			// horizontal lines
			linet[i][j] = colset[i][j];
			lineb[i][j] = colset[i + 1][j];

			// vertical lines
			linet[3 + i][j] = colset[j][i + 1];
			lineb[3 + i][j] = colset[j][i];
		}
	}

	int linePts[6][3];

	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			linePts[i][j] = j * 3 + i;
			linePts[3 + i][j] = j + 3 * i;
		}
	}

	int w_blk = w_pat / 4;
	int h_blk = h_pat / 4;
	float ptsXY[9][2];
	for (int i = 0; i < 3; i++) {
		for (int j = 0; j < 3; j++) {
			int id_x = w_blk*(j + 1);
			int id_y = h_blk*(i + 1);
			ptsXY[j * 3 + i][0] = id_x;
			ptsXY[j * 3 + i][1] = id_y;
		}
	}

	for (int i = 0; i < 4; i++) {
		for (int j = 0; j < 4; j++) {
			int id_col = colset[i][j];
			int col[3];
			col[0] = 0; col[1] = 0; col[2] = 0;
			col[id_col - 1] = 255;
			for (int i1 = 0; i1 < h_blk; i1++) {
				for (int j1 = 0; j1 < w_blk; j1++) {
					patImg.at<cv::Vec3b>(i*h_blk+i1, j*w_blk+j1) = cv::Vec3b(col[2],col[1],col[0]);
				}
			}
		}
	}

	if (true) {
		// horizontal lines
		int linePtsXY[6][3][2];
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				int id_x = w_blk*(j+1);
				int id_y = h_blk*(i+1);
				linePtsXY[i][j][0] = id_x;
				linePtsXY[i][j][1] = id_y;
			}
		}
		for (int i = 0; i < 3; i++) {
			for (int j = 0; j < 3; j++) {
				int id_x = w_blk*(i+1);
				int id_y = h_blk*(j+1);
				linePtsXY[i+3][j][0] = id_x;
				linePtsXY[i+3][j][1] = id_y;
			}
		}

		for (int i = 0; i < 6; i++) {
			cv::line(patImg, cv::Point(linePtsXY[i][0][0], linePtsXY[i][0][1]), cv::Point(linePtsXY[i][2][0], linePtsXY[i][2][1]), cv::Scalar(0), 2);
			cv::circle(patImg, cv::Point(linePtsXY[i][0][0], linePtsXY[i][0][1]), 10, cv::Scalar(0));
			char buf[256];
			sprintf(buf, " Line %d", i);
			cv::String strTxt(buf);
			cv::putText(patImg, strTxt, cv::Point(linePtsXY[i][0][0], linePtsXY[i][0][1]), CV_FONT_HERSHEY_COMPLEX, 1, cv::Scalar(0));
			sprintf(buf, " %d", linePts[i][0]);
			strTxt=buf;
			cv::putText(patImg, strTxt, cv::Point(linePtsXY[i][0][0]+10, linePtsXY[i][0][1]+30), CV_FONT_HERSHEY_COMPLEX, 1, cv::Scalar(255,255,255));
			sprintf(buf, " %d", linePts[i][1]);
			strTxt=buf;
			cv::putText(patImg, strTxt, cv::Point(linePtsXY[i][1][0] + 10, linePtsXY[i][1][1] + 30), CV_FONT_HERSHEY_COMPLEX, 1, cv::Scalar(255, 255, 255));
			sprintf(buf, " %d", linePts[i][2]);
			strTxt=buf;
			cv::putText(patImg, strTxt, cv::Point(linePtsXY[i][2][0] + 10, linePtsXY[i][2][1] + 30), CV_FONT_HERSHEY_COMPLEX, 1, cv::Scalar(255, 255, 255));
		}
	}
}

//void getCPTCorners(cv::Mat &img_CV, cv::Mat &corners) {
//	cv::Mat img_gray;
//	cv::cvtColor(img_CV, img_gray, CV_BGR2GRAY);
//
//	// color conversion
//	colConversionGL(input_image, D, w, h, th_black);
//	cv::Mat D_cv(cv::Size(w, h), CV_8UC1, D.data());
//
//	// color purity
//	int skip = 2;
//	getColorPurity(input_image, P, m_sz_blk, skip, w, h, th_black);
//	cv::Mat P_cv(cv::Size(w / m_sz_blk, h / m_sz_blk), CV_8UC1, P.data());
//	cv::Mat P_resize;
//	cv::resize(P_cv, P_resize, cv::Size(img_CV.cols, img_CV.rows));
//
//	//  get block corners
//	getBlockCorners10(D, C, P, m_sz_blk, w, h);
//	cv::Mat C_cv(cv::Size(w, h), CV_8UC1, C.data());
//
//	// refine corners
//	refineCorners(C, CNew, w, h);
//	cv::Mat CNew_cv(cv::Size(w, h), CV_8UC1, CNew.data());
//
//	// get corners
//	ts = omp_get_wtime();
//	int nCorners = 0;
//	std::vector<float> xCorners;
//	std::vector<float> yCorners;
//	xCorners.reserve(1000);
//	yCorners.reserve(1000);
//
//	for (int i = 0; i < w; i++) {
//		for (int j = 0; j < h; j++) {
//			if (CNew[j*w + i] > 0) {
//				xCorners.push_back(i);
//				yCorners.push_back(j);
//			}
//		}
//	}
//	nCorners = xCorners.size();
//	te = omp_get_wtime();
//	printf("Corners read in %lf ms\n", (te - ts) * 1000);
//
//	int deg_eq = 27; // to store some intermediate and debug information
//
//	ts = omp_get_wtime();
//	std::vector<int>votes(nCorners * 9); // for each detected corner, how many votes each of the 9 pattern corners get
//	std::vector<float>eqns(nCorners * nCorners * deg_eq);
//
//	for (int i = 0; i < nCorners * 9; i++)
//		votes[i] = 0;
//
//	te = omp_get_wtime();
//	printf("votes alloc in %lf ms\n", (te - ts) * 1000);
//
//	ts = omp_get_wtime();
//	getLinePtAssignment(img_CV, votes, D, eqns, yCorners, xCorners, h, w);
//	te = omp_get_wtime();
//	printf("votes in %lf ms\n", (te - ts) * 1000);
//
//	// show points and votes
//	showPointsAndVotes(img_CV, votes, yCorners, xCorners);
//
//	std::vector<int> ptLoc(9);
//	ts = omp_get_wtime();
//	getPatternPoints(xCorners, yCorners, votes, ptLoc, nCorners, w, h);
//
//	for (int i = 0; i < 9; i++) {
//		if (ptLoc[i] != -1) {
//			corners.at<float>(i, 0) = xCorners[ptLoc[i]];
//			corners.at<float>(i, 1) = yCorners[ptLoc[i]];
//		}
//		else {
//			corners.at<float>(i, 0) = -1;
//			corners.at<float>(i, 1) = -1;
//		}
//	}
//}

void runProcess_batchImage(char *fname, char *fname_out, int w, int h){
	// Activity to be timed
	double ts, te;
	std::vector<unsigned char> image_in_t(w * h * 4);
	std::vector<unsigned char> input_image(w * h * 4);

	// load image
	FILE *fid = fopen(fname, "rb");
	fread(image_in_t.data(), 1, w * h * 4, fid);
	fclose(fid);

	for (int i = 0; i < h; i++) {
		for (int j = 0; j < w; j++) {
			input_image[j * h * 4 + i * 4 + 0] = image_in_t[(h - i - 1) * w * 4 + j * 4 + 0];
			input_image[j * h * 4 + i * 4 + 1] = image_in_t[(h - i - 1) * w * 4 + j * 4 + 1];
			input_image[j * h * 4 + i * 4 + 2] = image_in_t[(h - i - 1) * w * 4 + j * 4 + 2];
			input_image[j * h * 4 + i * 4 + 3] = image_in_t[(h - i - 1) * w * 4 + j * 4 + 3];
		}
	}

	cv::Mat img_CV(cv::Size(h, w), CV_8UC4, input_image.data());

	cv::Mat corners;
	extractCorners(img_CV, corners);

	displayCornersOnImage(corners, img_CV, fname_out);
}

void showPointsAndVotes(cv::Mat &img_CV, std::vector<int> &votes, std::vector<float> &xCorners, std::vector<float> &yCorners) {
	cv::Mat img;
	img_CV.copyTo(img);

	for (int i = 0; i < xCorners.size(); i++) {
		cv::circle(img, cv::Point(yCorners[i], xCorners[i]), 5, cv::Scalar(0));
		char buf[256];
		char buf2[256];
		sprintf(buf, "%d",i);
		printf("%d:[", i);
		int nVotes = 0;
		for (int j = 0; j < 9; j++) {
			printf("%d,", votes[i * 9 + j]);
			nVotes += votes[i * 9 + j];
		}
		printf("]\n");
		if (nVotes > 0) {
			cv::String str = buf;
			cv::putText(img, str, cv::Point(yCorners[i], xCorners[i]), CV_FONT_HERSHEY_COMPLEX, 1, cv::Scalar(0));
		}
	}
	cv::imwrite("result.bmp", img);
}

bool getPatternPoints(std::vector<float> &xCorners, std::vector<float> &yCorners, std::vector<int> &votes, std::vector<int> &ptLoc, int nCorners, size_t w_img, size_t h_img) {

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
	int *ptClass = (int *)malloc(nCorners * sizeof(int));
	float *ptPurity = (float *)malloc(nCorners * sizeof(float));
	int *ptValid = (int*)malloc(nCorners * sizeof(int));

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
	uchar *ptValidity = (uchar *)malloc(nCorners * sizeof(uchar));
	for (int i = 0; i < nCorners; i++) {
		ptValidity[i] = 0;
	}

	printf("points: %d", nCorners);

	// center point is class-4
	int count_patterns = 0;
	for (int t = 0; t < nCorners; t++) {
		if (count_patterns >= 6)
			break;

		if (ptClass[t] == 4 && ptPurity[t] >= 1) {
			float ptDist[9];
			
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
						}
						else {
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

			//print_out("ptLoc: %d %d %d  %d %d %d  %d %d %d",ptLoc[0],ptLoc[1],ptLoc[2],ptLoc[3],ptLoc[4],ptLoc[5],ptLoc[6],ptLoc[7],ptLoc[8]);
			validateCorners(ptLoc, xCorners, yCorners, xCorners[ptLoc[4]], yCorners[ptLoc[4]]);
			//print_out("ptLoc: %d %d %d  %d %d %d  %d %d %d",ptLoc[0],ptLoc[1],ptLoc[2],ptLoc[3],ptLoc[4],ptLoc[5],ptLoc[6],ptLoc[7],ptLoc[8]);

			int id_selection = 0;
			for (int i = 0; i < 9; i++) {
				if (ptLoc[i] != -1) {
					id_selection += Pow2[i];
				}
			}
			if (valid_ptSel[id_selection] == 1) {
				count_patterns++;
				break;
			}
		}
	}

	// detect other points - do 4C3
	if (count_patterns == 0 && nCorners>=4) {

		// sort on point purity
		std::vector<ElFSort> sortedPurity(nCorners);
		std::vector<int>sortID(nCorners);
		for (int t = 0; t < nCorners; t++) {
			sortedPurity[t].v = ptPurity[t];
			sortedPurity[t].id = t;
		}
		std::sort(sortedPurity.begin(), sortedPurity.end(), fSortFunctionDescending);

		int topClass[4];
		int count = 0;
		for (int t = 0; t < nCorners; t++) {
			int flag_classNoted = 0;
			for (int i = 0; i < count; i++) {
				if (ptClass[sortedPurity[t].id] == topClass[i]) {
					flag_classNoted = 1;
					break;
				}
			}
			if (flag_classNoted == 0 && ptPurity[sortedPurity[t].id]>0) {
				sortID[count] = sortedPurity[t].id;
				topClass[count] = ptClass[sortedPurity[t].id];
				count++;
				if (count == 4)break;
			}
		}

		// top 4C3 ransac
		if (count == 4) {
			// get ideal points 
			int w_blk = 600 / 4;
			int h_blk = 400 / 4;
			float ptsXY[9][2];
			for (int i = 0; i < 3; i++) {
				for (int j = 0; j < 3; j++) {
					int id_x = w_blk*(j + 1);
					int id_y = h_blk*(i + 1);
					ptsXY[j * 3 + i][0] = id_x;
					ptsXY[j * 3 + i][1] = id_y;
				}
			}

			for (int id_out = 0; id_out < 4; id_out++) {
				// estimate center location using the remaining three
				// ref: http://stackoverflow.com/questions/22954239/given-three-points-compute-affine-transformation
				cv::Mat XMat(6, 1, CV_32FC1);
				cv::Mat XTemplateMat(6, 6, CV_32FC1);
				int count = 0;
				for (int t = 0; t < 4; t++) {
					if (t != id_out) {
						int id_corner = sortID[t];
						int cls = ptClass[id_corner];
						float x = xCorners[id_corner];
						float y = yCorners[id_corner];
						float x_template = ptsXY[cls][0];
						float y_template = ptsXY[cls][1];

						XTemplateMat.at<float>(count, 0) = x_template;
						XTemplateMat.at<float>(count, 1) = y_template;
						XTemplateMat.at<float>(count, 2) = 1;
						XTemplateMat.at<float>(count, 3) = 0;
						XTemplateMat.at<float>(count, 4) = 0;
						XTemplateMat.at<float>(count, 5) = 0;
						XMat.at<float>(count) = x;
						count++;
						XTemplateMat.at<float>(count, 0) = 0;
						XTemplateMat.at<float>(count, 1) = 0;
						XTemplateMat.at<float>(count, 2) = 0;
						XTemplateMat.at<float>(count, 3) = x_template;
						XTemplateMat.at<float>(count, 4) = y_template;
						XTemplateMat.at<float>(count, 5) = 1;
						XMat.at<float>(count) = y;
						count++;
					}
				}
				cv::Mat A;
				cv::solve(XTemplateMat, XMat, A);
				cv::Mat affineMat(3, 3, CV_32FC1);
				affineMat.at<float>(0, 0) = A.at<float>(0);
				affineMat.at<float>(0, 1) = A.at<float>(1);
				affineMat.at<float>(0, 2) = A.at<float>(2);
				affineMat.at<float>(1, 0) = A.at<float>(3);
				affineMat.at<float>(1, 1) = A.at<float>(4);
				affineMat.at<float>(1, 2) = A.at<float>(5);
				affineMat.at<float>(2, 2) = 0;
				affineMat.at<float>(2, 2) = 0;
				affineMat.at<float>(2, 2) = 1;

				// get the center point
				float x_template = ptsXY[4][0];
				float y_template = ptsXY[4][1];

				count = 0;
				XTemplateMat.at<float>(count, 0) = x_template;
				XTemplateMat.at<float>(count, 1) = y_template;
				XTemplateMat.at<float>(count, 2) = 1;
				XTemplateMat.at<float>(count, 3) = 0;
				XTemplateMat.at<float>(count, 4) = 0;
				XTemplateMat.at<float>(count, 5) = 0;
				count++;
				XTemplateMat.at<float>(count, 0) = 0;
				XTemplateMat.at<float>(count, 1) = 0;
				XTemplateMat.at<float>(count, 2) = 0;
				XTemplateMat.at<float>(count, 3) = x_template;
				XTemplateMat.at<float>(count, 4) = y_template;
				XTemplateMat.at<float>(count, 5) = 1;

				cv::Mat resMat = XTemplateMat*A;
				float x_4 = resMat.at<float>(0);
				float y_4 = resMat.at<float>(1);

				// detect point based on center (4)
				float ptDist[9];

				for (int i = 0; i < 9; i++) {
					ptLoc[i] = -1;
					ptDist[i] = 50000;
				}
				//ptLoc[4] = t;

				for (int j = 0; j < nCorners; j++) {
					if (ptClass[j] != 4 && ptClass[j] != -1 && ptPurity[j] >= 1) {
						float d = std::abs(xCorners[j] - x_4)
							+ std::abs(yCorners[j] - y_4);
						if (d < ptDist[ptClass[j]] + 5) {
							if (ptLoc[ptClass[j]] == -1) {
								ptDist[ptClass[j]] = d;
								ptLoc[ptClass[j]] = j;
							}
							else {
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

				//print_out("ptLoc: %d %d %d  %d %d %d  %d %d %d",ptLoc[0],ptLoc[1],ptLoc[2],ptLoc[3],ptLoc[4],ptLoc[5],ptLoc[6],ptLoc[7],ptLoc[8]);
				validateCorners(ptLoc, xCorners, yCorners, x_4, y_4);
				//print_out("ptLoc: %d %d %d  %d %d %d  %d %d %d",ptLoc[0],ptLoc[1],ptLoc[2],ptLoc[3],ptLoc[4],ptLoc[5],ptLoc[6],ptLoc[7],ptLoc[8]);

				int id_selection = 0;
				for (int i = 0; i < 9; i++) {
					if (ptLoc[i] != -1) {
						id_selection += Pow2[i];
					}
				}
				if (valid_ptSel[id_selection] == 1) {
					count_patterns++;
					break;
				}
			}
		}
	}

	// free
	free(ptValidity);
	free(ptPurity);
	free(ptClass);
	free(ptValid);

	return true;
}

void validateCorners(std::vector<int> &id_pts, std::vector<float> &xCorners, std::vector<float> &yCorners, float x_mid, float y_mid) {

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
		if (id_valid[i] == 0)
			id_pts[i] = -1;

	//get distances from center
	std::vector<float> dist(9);
	std::vector<float> dist_const(9);

	float xm = x_mid;
	float ym = y_mid;
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

void refineCorners(std::vector<unsigned char> &C, std::vector<unsigned char> &CNew, int w, int h) {
	
	double ts = omp_get_wtime();
int sz_win = 5;
#pragma omp parallel for
	for (int y = sz_win; y < h - sz_win; y++) {
		for (int x = sz_win; x < w - sz_win; x++) {
			size_t id = y*w + x;

			if (C[id] == 0)continue;
			
			float mx = 0;
			float my = 0;
			int count = 0;
			for (int i = -sz_win; i < sz_win; i++)
			{
				for (int j = -sz_win; j < sz_win - 1; j++) {
					id = (y + j)*w + (x + i);
					if (C[id] == 1) {
						mx += (x + i);
						my += (y + j);
						count++;
					}
				}
			}

			mx /= count;
			my /= count;

			int xNew = floor(mx);
			int yNew = floor(my);

			id = yNew*w + xNew;
			CNew[id] = 1;
		}
	}

	double te = omp_get_wtime();
	printf("Corners refined in %lf ms\n", (te - ts) * 1000);
}

unsigned char getColVal_colMajor(int x, int y, int w, int h, std::vector<unsigned char> &D)
{
	uchar c = 4;
	if (x >= 0 && x<w && y >= 0 && y<h)
	{
		int id = x*h + y;
		c = D[id];
	}
	return c;
}


void getLinePtAssignment(cv::Mat &img_CV, std::vector<int> &ptsVote, std::vector<unsigned char> &D, std::vector<float> &lineEqns, std::vector<float> &x, std::vector<float> &y, int w_img, int h_img) {
	int debug = 0;
	size_t nPts = x.size();

	std::vector<int> ptsValid(nPts);
	for (int p0 = 0; p0 < nPts; p0++) {
		ptsValid[p0] = 1;
	}
	for (int p0 = 0; p0 < nPts; p0++) {
		for (int p1 = p0 + 1; p1 < nPts; p1++) {
			int deg_eq = 27;

			if (debug == 1) {
				for (int i = 0; i < deg_eq; i++) {
					lineEqns[(p0*nPts + p1)*deg_eq + i] = 0;
				}
			}

			// getting the line equation
			float x1 = x[p0];
			float y1 = y[p0];
			float x2 = x[p1];
			float y2 = y[p1];

			float a = (y2 - y1);
			float b = (x1 - x2);
			float c = (x2*y1 - x1*y2);
			float d = sqrt(a*a + b*b);
			if (d < 5) {
				ptsValid[p0] = 0;
			}
		}
	}
	cv::Mat img_copy_pts;
	/*
	img_CV.copyTo(img_copy_pts);
	
	for (int p0 = 0; p0 < nPts; p0++) {
		if (ptsValid[p0] == 1) {
			cv::circle(img_copy_pts, cv::Point(y[p0], x[p0]), 5, cv::Scalar(255));
			char buff[32];
			sprintf(buff, "%d", p0);
			cv::putText(img_copy_pts, buff, cv::Point(y[p0], x[p0]), CV_FONT_HERSHEY_COMPLEX, 1, cv::Scalar(0));
		}
		else {
			//cv::circle(img_copy, cv::Point(y[p0], x[p0]), 5, cv::Scalar(0));
		}
	}
	*/
//#pragma omp parallel for
	for (int p0 = 0; p0 < nPts; p0++) {
		for (int p1 = p0 + 1; p1 < nPts; p1++) {
			if (ptsValid[p0] == 0 || ptsValid[p1] == 0)continue;
			cv::Mat img_copy;
			//img_CV.copyTo(img_copy);

			int deg_eq = 27;

			if (debug == 1) {
				for (int i = 0; i < deg_eq; i++) {
					lineEqns[(p0*nPts + p1)*deg_eq + i] = 0;
				}
			}

			// getting the line equation
			float x1 = x[p0];
			float y1 = y[p0];
			float x2 = x[p1];
			float y2 = y[p1];

			float a = (y2 - y1);
			float b = (x1 - x2);
			float c = (x2*y1 - x1*y2);
			float d = sqrt(a*a + b*b);
			float th_dist = d/10;
			if (th_dist < 5)th_dist = 5;

			// if points are too close, they may represent the same point
			if (d <= 5)continue;

			a /= d;
			b /= d;
			c /= d;

			float d_pts = d;//(x2-x1)*(x2-x1)+(y2-y1)*(y2-y1);

							// direction of line is (-b, a) from p0 to p1
							// get points on line at 4 equidistant places spread across the two points
			float xm[4];
			float ym[4];
			int xmt[4], ymt[4], xmb[4], ymb[4];
			xm[0] = x1 - d_pts / 4.0*(-b);
			ym[0] = y1 - d_pts / 4.0*(a);
			xm[1] = x1 + d_pts / 4.0*(-b);
			ym[1] = y1 + d_pts / 4.0*(a);
			xm[2] = x1 + 3 * d_pts / 4.0*(-b);
			ym[2] = y1 + 3 * d_pts / 4.0*(a);
			xm[3] = x1 + 5 * d_pts / 4.0*(-b);
			ym[3] = y1 + 5 * d_pts / 4.0*(a);

			// get points and colors just top and bottom of the line
			for (int i = 0; i < 4; i++) {
				xmt[i] = floor(xm[i] + 5 * a);
				ymt[i] = floor(ym[i] + 5 * b);

				xmb[i] = floor(xm[i] - 5 * a);
				ymb[i] = floor(ym[i] - 5 * b);
			}

			uchar ct[4], cb[4];
			for (int i = 0; i < 4; i++) {
				ct[i] = getColVal_colMajor(xmt[i], ymt[i], w_img, h_img, D);
				cb[i] = getColVal_colMajor(xmb[i], ymb[i], w_img, h_img, D);
			}

			for (int i = 0; i < 4; i++)
			{
				if (debug == 1) {
					lineEqns[(p0*nPts + p1)*deg_eq + i * 4 + 0] = xmt[i];
					lineEqns[(p0*nPts + p1)*deg_eq + i * 4 + 1] = ymt[i];
					lineEqns[(p0*nPts + p1)*deg_eq + i * 4 + 2] = xmb[i];
					lineEqns[(p0*nPts + p1)*deg_eq + i * 4 + 3] = ymb[i];
					lineEqns[(p0*nPts + p1)*deg_eq + 16 + 2 * i] = ct[i];
					lineEqns[(p0*nPts + p1)*deg_eq + 16 + 2 * i + 1] = cb[i];
				}
			}

			if (ct[1] == 0 || cb[1] == 0 || ct[2] == 0 || cb[2] == 0) {
				if (debug == 1) {
					lineEqns[(p0*nPts + p1)*deg_eq + 24] = 0;
					lineEqns[(p0*nPts + p1)*deg_eq + 25] = 0;
					lineEqns[(p0*nPts + p1)*deg_eq + 26] = 1;
				}
				continue;
			}
			if (ct[0] != 0 && cb[0] != 0 && ct[0] == cb[0]) {
				if (debug == 1) {
					lineEqns[(p0*nPts + p1)*deg_eq + 24] = 1;
					lineEqns[(p0*nPts + p1)*deg_eq + 25] = 0;
					lineEqns[(p0*nPts + p1)*deg_eq + 26] = 1;
				}
				continue;
			}
			if (ct[3] != 0 && cb[3] != 0 && ct[3] == cb[3]) {
				if (debug == 1) {
					lineEqns[(p0*nPts + p1)*deg_eq + 24] = 2;
					lineEqns[(p0*nPts + p1)*deg_eq + 25] = 0;
					lineEqns[(p0*nPts + p1)*deg_eq + 26] = 1;
				}
				continue;
			}
			if (ct[1] == cb[1] || ct[2] == cb[2] || ct[1] == ct[2] || cb[1] == cb[2]) {
				if (debug == 1) {
					lineEqns[(p0*nPts + p1)*deg_eq + 24] = 3;
					lineEqns[(p0*nPts + p1)*deg_eq + 25] = 0;
					lineEqns[(p0*nPts + p1)*deg_eq + 26] = 1;
				}
				continue;
			}
			
			// find the line ID(s)
			// colors of 4x4 grid pattern
			uchar colset[4][4];
			/*
			colset[0][0] = 1;
			colset[0][1] = 2;
			colset[0][2] = 1;
			colset[0][3] = 3;

			colset[1][0] = 2;
			colset[1][1] = 3;
			colset[1][2] = 2;
			colset[1][3] = 1;

			colset[2][0] = 1;
			colset[2][1] = 2;
			colset[2][2] = 1;
			colset[2][3] = 3;

			colset[3][0] = 2;
			colset[3][1] = 3;
			colset[3][2] = 2;
			colset[3][3] = 1;
			*/
			colset[0][0] = 1;
			colset[0][1] = 2;
			colset[0][2] = 3;
			colset[0][3] = 1;

			colset[1][0] = 3;
			colset[1][1] = 1;
			colset[1][2] = 2;
			colset[1][3] = 3;

			colset[2][0] = 1;
			colset[2][1] = 2;
			colset[2][2] = 3;
			colset[2][3] = 1;

			colset[3][0] = 2;
			colset[3][1] = 3;
			colset[3][2] = 1;
			colset[3][3] = 2;

			uchar linet[6][4];
			uchar lineb[6][4];

			// colors for top and bottom of each of the horizontal and vertical lines
			for (int i = 0; i < 3; i++) {
				for (int j = 0; j < 4; j++) {
					// horizontal lines
					linet[i][j] = colset[i][j];
					lineb[i][j] = colset[i + 1][j];

					// vertical lines
					linet[3 + i][j] = colset[j][i + 1];
					lineb[3 + i][j] = colset[j][i];
				}
			}

			int linePts[6][3];

			for (int i = 0; i < 3; i++) {
				for (int j = 0; j < 3; j++) {
					linePts[i][j] = j * 3 + i;
					linePts[3 + i][j] = j + 3 * i;
				}
			}

			// check lines
			for (int i = 0; i < 6; i++)
			{
				//printf("Checking line#%d for pts %d and %d \n",i, p0, p1);
				int flag = 1;
				for (int j = 0; j < 4; j++) {
					if (ct[j] != 0 && cb[j] != 0 && (ct[j] != linet[i][j] || cb[j] != lineb[i][j])) {
						flag = 0;
						break;
					}
				}
				if (flag == 0) { // reverse lin
					flag = 2;
					for (int j = 0; j < 4; j++) {
						if (ct[j] != 0 && cb[j] != 0 && (ct[j] != lineb[i][3 - j] || cb[j] != linet[i][3 - j])) {
							flag = 0;
							break;
						}
					}
				}

				// found matching line
				if (flag > 0) {
					// find middle point
					// binary search for crossing
					int x1_mid, x2_mid, y1_mid, y2_mid;
					int *px1_mid, *px2_mid, *py1_mid, *py2_mid;
					px1_mid = &x1_mid;
					px2_mid = &x2_mid;
					py1_mid = &y1_mid;
					py2_mid = &y2_mid;

					binarySearch(xmt[1], xmt[2], ymt[1], ymt[2], px1_mid, py1_mid, w_img, h_img, D);
					binarySearch(xmb[1], xmb[2], ymb[1], ymb[2], px2_mid, py2_mid, w_img, h_img, D);

					x1_mid = *px1_mid;
					x2_mid = *px2_mid;
					y1_mid = *py1_mid;
					y2_mid = *py2_mid;

					// find intersection point - alpha is described in definition of x_mid and y_mid - put these values in eq of the line and find the value of alpha
					float alpha = (a*x2_mid + b*y2_mid + c) / (a*x2_mid + b*y2_mid - a*x1_mid - b*y1_mid);
					int x_mid = floor(x1_mid + alpha*(x2_mid - x1_mid));
					int y_mid = floor(y1_mid + alpha*(y2_mid - y1_mid));

					if (debug == 1) {
						lineEqns[(p0*nPts + p1)*deg_eq + 16] = p0;
						lineEqns[(p0*nPts + p1)*deg_eq + 17] = p1;
						lineEqns[(p0*nPts + p1)*deg_eq + 18] = x1;
						lineEqns[(p0*nPts + p1)*deg_eq + 19] = y1;
						lineEqns[(p0*nPts + p1)*deg_eq + 20] = x2;
						lineEqns[(p0*nPts + p1)*deg_eq + 21] = y2;
						lineEqns[(p0*nPts + p1)*deg_eq + 22] = x_mid;
						lineEqns[(p0*nPts + p1)*deg_eq + 23] = y_mid;
						lineEqns[(p0*nPts + p1)*deg_eq + 24] = i;
						lineEqns[(p0*nPts + p1)*deg_eq + 25] = flag;
						lineEqns[(p0*nPts + p1)*deg_eq + 26] = 2;
					}

					// find all points near mid
					int id_pt0 = -1;
					int id_pt1 = -1;
					int id_pt2 = -1;
					for (int t = 0; t < nPts; t++) {
						if (ptsValid[t] == 0)continue;
						if (fabs(x[t] - x_mid) < th_dist && fabs(y[t] - y_mid) < th_dist) {
							//atomic_inc(&ptsVote[t * 9 + linePts[i][1]]);
							//#pragma omp atomic
								ptsVote[t * 9 + linePts[i][1]]++;
								id_pt1 = t;
								//cv::circle(img_copy, cv::Point(y[t], x[t]), 5, cv::Scalar(128));
								//printf("Pt %d is %d\n ", t, linePts[i][1]);
						}
						if (fabs(x[t] - x1) < th_dist && fabs(y[t] - y1) < th_dist) {
							if (flag == 1) {
								//atomic_inc(&ptsVote[t * 9 + linePts[i][0]]);
								//#pragma omp atomic
								ptsVote[t * 9 + linePts[i][0]]++;
								id_pt0 = t;
								//cv::circle(img_copy, cv::Point(y[t], x[t]), 5, cv::Scalar(0));
								//printf("Pt %d is %d\n ", t, linePts[i][0]);
							}
							else {
								//atomic_inc(&ptsVote[t * 9 + linePts[i][2]]);
								//#pragma omp atomic
								ptsVote[t * 9 + linePts[i][2]]++;
								id_pt2 = t;
								//cv::circle(img_copy, cv::Point(y[t], x[t]), 5, cv::Scalar(256));
								//printf("Pt %d is %d\n ", t, linePts[i][2]);
							}
						}
						if (fabs(x[t] - x2) < th_dist && fabs(y[t] - y2) < th_dist) {
							if (flag == 1) {
								//atomic_inc(&ptsVote[t * 9 + linePts[i][2]]);
								//#pragma omp atomic
								ptsVote[t * 9 + linePts[i][2]]++;
								id_pt2 = t;
								//cv::circle(img_copy, cv::Point(y[t], x[t]), 5, cv::Scalar(256));
								//printf("Pt %d is %d\n ", t, linePts[i][2]);
							}
							else {
								//atomic_inc(&ptsVote[t * 9 + linePts[i][0]]);
								//#pragma omp atomic
								ptsVote[t * 9 + linePts[i][0]]++;
								id_pt0 = t;
								//cv::circle(img_copy, cv::Point(y[t], x[t]), 5, cv::Scalar(0));
								//printf("Pt %d is %d\n ", t, linePts[i][0]);
							}
						}
					}
					if (id_pt1 != -1) {
						for (int t = 0; t < nPts; t++) {
							if (ptsValid[t] == 0)continue;
							if (fabs(x[t] - x_mid) < th_dist && fabs(y[t] - y_mid) < th_dist) {
								//atomic_inc(&ptsVote[t * 9 + linePts[i][1]]);
								//#pragma omp atomic
								ptsVote[t * 9 + linePts[i][1]]++;
								id_pt1 = t;
								//cv::circle(img_copy, cv::Point(y[t], x[t]), 5, cv::Scalar(128));
								//printf("Pt %d is %d\n ", t, linePts[i][1]);
							}
							if (fabs(x[t] - x1) < th_dist && fabs(y[t] - y1) < th_dist) {
								if (flag == 1) {
									//atomic_inc(&ptsVote[t * 9 + linePts[i][0]]);
									//#pragma omp atomic
									ptsVote[t * 9 + linePts[i][0]]++;
									id_pt0 = t;
									//cv::circle(img_copy, cv::Point(y[t], x[t]), 5, cv::Scalar(0));
									//printf("Pt %d is %d\n ", t, linePts[i][0]);
								}
								else {
									//atomic_inc(&ptsVote[t * 9 + linePts[i][2]]);
									//#pragma omp atomic
									ptsVote[t * 9 + linePts[i][2]]++;
									id_pt2 = t;
									//cv::circle(img_copy, cv::Point(y[t], x[t]), 5, cv::Scalar(256));
									//printf("Pt %d is %d\n ", t, linePts[i][2]);
								}
							}
							if (fabs(x[t] - x2) < th_dist && fabs(y[t] - y2) < th_dist) {
								if (flag == 1) {
									//atomic_inc(&ptsVote[t * 9 + linePts[i][2]]);
									//#pragma omp atomic
									ptsVote[t * 9 + linePts[i][2]]++;
									id_pt2 = t;
									cv::circle(img_copy, cv::Point(y[t], x[t]), 5, cv::Scalar(256));
									//printf("Pt %d is %d\n ", t, linePts[i][2]);
								}
								else {
									//atomic_inc(&ptsVote[t * 9 + linePts[i][0]]);
									//#pragma omp atomic
									ptsVote[t * 9 + linePts[i][0]]++;
									id_pt0 = t;
									//cv::circle(img_copy, cv::Point(y[t], x[t]), 5, cv::Scalar(0));
									//printf("Pt %d is %d\n ", t, linePts[i][0]);
								}
							}
						}
					}
					//printf("lines checked\n");
				}
			}

		}
	}
}

void binarySearch(int x1, int x2, int y1, int y2, int *x_ret, int *y_ret, int w, int h, std::vector<unsigned char> &D) {
	int x_mid = (x1 + x2) / 2;
	int y_mid = (y1 + y2) / 2;

	int id1 = x1*h + y1;
	int id2 = x2*h + y2;

	uchar c1 = D[id1];
	uchar c2 = D[id2];

	int id_mid = x_mid*h + y_mid;
	uchar c_mid = D[id_mid];

	while (abs(x1 - x2) + abs(y1 - y2) >2) {
		if (c_mid == c1) {
			x1 = x_mid;
			y1 = y_mid;
			id1 = x1*h + y1;
		}
		if (c_mid == c2) {
			x2 = x_mid;
			y2 = y_mid;
			id2 = x2*h + y2;
		}
		if (c_mid == 0) { *x_ret = -1; *y_ret = -1; return; }
		if (c_mid != c1 && c_mid != c2 && c_mid != 0) { *x_ret = x_mid; *y_ret = y_mid; return; }
		x_mid = (x1 + x2) / 2;
		y_mid = (y1 + y2) / 2;
		id_mid = x_mid*h + y_mid;
		c_mid = D[id_mid];
	}
	*x_ret = x_mid; *y_ret = y_mid;
}

void checkRimCorner(std::vector<unsigned char> &D, int idx, int idy, int sz_x, int r_rim, uchar *pcol1, uchar *pcol2, uchar *pcol3)
{
	size_t id, idbx, idby;
	uchar c;
	for (int i = 0; i<2 * r_rim; i++) {
		idbx = idx + i;
		idby = idy;

		id = idby*sz_x + idbx;
		c = D[id];
		if (c == 1)*pcol1 = 1;
		if (c == 2)*pcol2 = 1;
		if (c == 3)*pcol3 = 1;

		idbx = idx + i;
		idby = idy + 2 * r_rim - 1;

		id = idby*sz_x + idbx;
		c = D[id];
		if (c == 1)*pcol1 = 1;
		if (c == 2)*pcol2 = 1;
		if (c == 3)*pcol3 = 1;
	}

	for (int i = 1; i<2 * r_rim - 1; i++) {
		idbx = idx;
		idby = idy + i;

		id = idby*sz_x + idbx;
		c = D[id];
		if (c == 1)*pcol1 = 1;
		if (c == 2)*pcol2 = 1;
		if (c == 3)*pcol3 = 1;

		idbx = idx + 2 * r_rim - 1;
		idby = idy + i;

		id = idby*sz_x + idbx;
		c = D[id];
		if (c == 1)*pcol1 = 1;
		if (c == 2)*pcol2 = 1;
		if (c == 3)*pcol3 = 1;
	}
}

bool checkRimCornerBool(std::vector<unsigned char> &D, int idx, int idy, int sz_x, int r_rim)
{
	uchar col1, col2, col3;
	col1 = 0;
	col2 = 0;
	col3 = 0;

	checkRimCorner(D, idx, idy, sz_x, r_rim, &col1, &col2, &col3);

	if ((col1>0 && col2>0) && col3>0) { return true; }

	return false;
}


void getBlockCorners10(std::vector<unsigned char> &D, std::vector<unsigned char> &C, std::vector<unsigned char> &purity, int sz_step, int w, int h) {
	size_t sz_x_org = (int)(w / sz_step);
	size_t sz_y_org = (int)(h / sz_step);

	double ts = omp_get_wtime();

#pragma omp parallel for
	for (int idy_org = 0; idy_org < (int)(h / sz_step); idy_org++) {
		for (int idx_org = 0; idx_org < (int)(w/sz_step); idx_org++) {
			size_t idx = idx_org;
			size_t idy = idy_org;
			size_t sz_x = sz_x_org;
			size_t sz_y = sz_y_org;

			if (idx >= sz_x - 5 || idy >= sz_y - 5) continue;

			size_t id, idbx, idby;
			int colID;

			id = idy*sz_x + idx;
			if (purity[id] == 0) continue;

			idx *= sz_step;
			idy *= sz_step;
			sz_x *= sz_step;
			sz_y *= sz_step;

			int r_rim = 7;

			if (checkRimCornerBool(D, idx, idy, sz_x, r_rim)) {
				// find corner
				shrinkBox(D, &idx, &idy, 2 * r_rim, 2 * r_rim, sz_x);

				if (idx < r_rim || idx >= sz_x - r_rim || idy < r_rim || idy >= sz_y - r_rim)continue;
				idx -= r_rim;
				idy -= r_rim;
				shrinkBox(D, &idx, &idy, 2 * r_rim, 2 * r_rim, sz_x);
				if (idx < r_rim || idx >= sz_x - r_rim || idy < r_rim || idy >= sz_y - r_rim)continue;

				jointDetect(D, &idx, &idy, 2 * r_rim, 2 * r_rim, sz_x);

				id = idy*sz_x + idx;
				C[id] = 1;
			}

			// check diagonally offset blocks
			idx = idx_org;
			idy = idy_org;

			idx = idx*sz_step + r_rim;
			idy = idy*sz_step + r_rim;

			if (checkRimCornerBool(D, idx, idy, sz_x, r_rim)) {
				// find corner
				shrinkBox(D, &idx, &idy, 2 * r_rim, 2 * r_rim, sz_x);

				if (idx < r_rim || idx >= sz_x - r_rim || idy < r_rim || idy >= sz_y - r_rim)continue;

				idx -= r_rim;
				idy -= r_rim;
				shrinkBox(D, &idx, &idy, 2 * r_rim, 2 * r_rim, sz_x);
				if (idx < r_rim || idx >= sz_x - r_rim || idy < r_rim || idy >= sz_y - r_rim)continue;

				jointDetect(D, &idx, &idy, 2 * r_rim, 2 * r_rim, sz_x);

				id = idy*sz_x + idx;
				C[id] = 1;
			}
		}
	}

	double te = omp_get_wtime();
	printf("Corners in %lf ms\n", (te - ts) * 1000);
}

void jointDetect(std::vector<unsigned char> &D, size_t *px, size_t *py, int w, int h, int sz_x) {
	int x, y;
	x = *px;
	y = *py;

	uchar c;
	int sz_win = 5; // it has to be <= r_rim
	int id1, id2;

	uchar grad[11][11];

	// vertical gradient
	for (int i = -sz_win; i<sz_win; i++)
	{
		for (int j = -sz_win; j<sz_win - 1; j++) {
			id1 = (y + j)*sz_x + (x + i);
			id2 = (y + j + 1)*sz_x + (x + i);
			if (D[id1] != D[id2]) {
				grad[sz_win + j][sz_win + i] = 1;
				grad[sz_win + j + 1][sz_win + i] = 1;
			}
		}
	}

	for (int i = -sz_win; i<sz_win - 1; i++)
	{
		for (int j = -sz_win; j<sz_win; j++) {
			if (grad[sz_win + j][sz_win + i] == 1) {
				id1 = (y + j)*sz_x + (x + i);
				id2 = (y + j)*sz_x + (x + i + 1);
				if (D[id1] != D[id2]) {
					grad[sz_win + j][sz_win + i] = 2;
					if (grad[sz_win + j][sz_win + i + 1] == 1)grad[sz_win + j][sz_win + i + 1] = 2;
				}
			}
		}
	}

	uchar col[4];
	col[1] = 0; col[2] = 0; col[3] = 0;
	int i0, j0, flag;
	flag = 0;
	for (int i = -sz_win; i<sz_win; i++)
	{
		for (int j = -sz_win; j<sz_win; j++) {
			if (grad[sz_win + j][sz_win + i] == 2) {
				col[1] = 0; col[2] = 0; col[3] = 0;
				col[D[(y + j - 1)*sz_x + (x + i)]]++;
				col[D[(y + j + 1)*sz_x + (x + i)]]++;
				col[D[(y + j)*sz_x + (x + i - 1)]]++;
				col[D[(y + j)*sz_x + (x + i + 1)]]++;
				col[D[(y + j)*sz_x + (x + i)]]++;
				if (col[1]>0 && col[2]>0 && col[3]>0) { flag = 1; i0 = i; j0 = j; break; }
			}
		}
		if (flag == 1)break;
	}
	if (flag == 1) {
		*px = x + i0;
		*py = y + j0;
	}
}

void shrinkBox(std::vector<unsigned char> &D, size_t *px, size_t *py, int w, int h, int sz_x) {
	uchar newLineCol[4];
	uchar rimColCount[4];
	uchar lineColCount[4][4];
	int x, y;
	x = *px;
	y = *py;

	int id, xt, yt;
	uchar c;
	uchar c_edge1, c_edge2, c_edge1_old, c_edge2_old;

	for (int i = 0; i<4; i++)
	{
		newLineCol[i] = 0;
		rimColCount[i] = 0;
		for (int j = 0; j<4; j++)
		{
			lineColCount[i][j] = 0;
		}
	}

	// getting color counts for all sides
	// top & bot
	for (int i = 0; i<w; i++)
	{
		int j = 0;
		id = (y + j)*sz_x + (x + i);
		c = D[id];
		if (c>0) {
			rimColCount[c]++;
			lineColCount[0][c]++;
		}
		j = h - 1;
		id = (y + j)*sz_x + (x + i);
		c = D[id];
		if (c>0) {
			rimColCount[c]++;
			lineColCount[1][c]++;
		}
	}
	// left & right
	for (int j = 0; j<h; j++)
	{
		int i = 0;
		id = (y + j)*sz_x + (x + i);
		c = D[id];
		if (c>0) {
			if (j>0 && j<h - 1)rimColCount[c]++;
			lineColCount[2][c]++;
		}
		i = w - 1;
		id = (y + j)*sz_x + (x + i);
		c = D[id];
		if (c>0) {
			if (j>0 && j<h - 1)rimColCount[c]++;
			lineColCount[3][c]++;
		}
	}

	// shrinking - top bot left right
	int flag_shrink = 1;
	while (flag_shrink == 1) {
		flag_shrink = 0;

		if (h>1) { // top line
				   // fill new line color
			for (int t = 0; t<4; t++)newLineCol[t] = 0;
			for (int i = 0; i<w; i++)
			{
				int j = 1;
				id = (y + j)*sz_x + (x + i);
				c = D[id];
				if (c>0)newLineCol[c]++;
				if (i == 0)c_edge1 = c;
				if (i == (w - 1))c_edge2 = c;
			}
			{
				int i = 0;
				int j = 0;
				id = (y + j)*sz_x + (x + i);
				c = D[id];
				c_edge1_old = c;
				i = w - 1;
				j = 0;
				id = (y + j)*sz_x + (x + i);
				c = D[id];
				c_edge2_old = c;
			}

			// check if removing line is ok
			int flag = 1;
			for (int t = 1; t<4; t++) {
				if (newLineCol[t] == 0 && lineColCount[0][t]>0 && rimColCount[t] == lineColCount[0][t]) {
					flag = 0; // not ok to remove line
					break;
				}
			}
			// remove line
			if (flag == 1) {
				for (int t = 1; t<4; t++) {
					rimColCount[t] = rimColCount[t] - lineColCount[0][t] + newLineCol[t];
					lineColCount[0][t] = newLineCol[t];
				}
				// removing boundary two points
				rimColCount[c_edge1]--;
				rimColCount[c_edge2]--;

				lineColCount[2][c_edge1_old]--;
				lineColCount[3][c_edge2_old]--;

				y++;
				h--;
				flag_shrink = 1;
			}
		}// top line

		if (h>1) { // bot line
				   // fill new line color
			for (int t = 0; t<4; t++)newLineCol[t] = 0;
			for (int i = 0; i<w; i++)
			{
				int j = h - 2;
				id = (y + j)*sz_x + (x + i);
				c = D[id];
				if (c>0)newLineCol[c]++;
				if (i == 0)c_edge1 = c;
				if (i == w - 1)c_edge2 = c;
			}
			{
				int i = 0;
				int j = h - 1;
				id = (y + j)*sz_x + (x + i);
				c = D[id];
				c_edge1_old = c;
				i = w - 1;
				j = h - 1;
				id = (y + j)*sz_x + (x + i);
				c = D[id];
				c_edge2_old = c;
			}

			// check if removing line is ok
			int flag = 1;
			for (int t = 1; t<4; t++) {
				if (newLineCol[t] == 0 && lineColCount[1][t]>0 && rimColCount[t] == lineColCount[1][t]) {
					flag = 0;
					break;
				}
			}
			// remove line
			if (flag == 1) {
				for (int t = 1; t<4; t++) {
					rimColCount[t] = rimColCount[t] - lineColCount[1][t] + newLineCol[t];
					lineColCount[1][t] = newLineCol[t];
				}
				// removing boundary two points
				rimColCount[c_edge1]--;
				rimColCount[c_edge2]--;

				lineColCount[2][c_edge1_old]--;
				lineColCount[3][c_edge2_old]--;

				h--;
				flag_shrink = 1;
			}
		}// bot line

		if (w>1) { // left line
				   // fill new line color
			for (int t = 0; t<4; t++)newLineCol[t] = 0;
			for (int j = 0; j<h; j++)
			{
				int i = 1;
				id = (y + j)*sz_x + (x + i);
				c = D[id];
				if (c>0)newLineCol[c]++;
				if (j == 0)c_edge1 = c;
				if (j == h - 1)c_edge2 = c;
			}
			{
				int i = 0;
				int j = 0;
				id = (y + j)*sz_x + (x + i);
				c = D[id];
				c_edge1_old = c;
				i = 0;
				j = h - 1;
				id = (y + j)*sz_x + (x + i);
				c = D[id];
				c_edge2_old = c;
			}

			// check if removing line is ok
			int flag = 1;
			for (int t = 1; t<4; t++) {
				if (newLineCol[t] == 0 && lineColCount[2][t]>0 && rimColCount[t] == lineColCount[2][t]) {
					flag = 0;
					break;
				}
			}
			// remove line
			if (flag == 1) {
				for (int t = 1; t<4; t++) {
					rimColCount[t] = rimColCount[t] - lineColCount[2][t] + newLineCol[t];
					lineColCount[2][t] = newLineCol[t];
				}
				// removing boundary two points
				rimColCount[c_edge1]--;
				rimColCount[c_edge2]--;

				lineColCount[0][c_edge1_old]--;
				lineColCount[1][c_edge2_old]--;

				x++;
				w--;
				flag_shrink = 1;
			}
		}// left line

		if (w>1) { // right line
				   // fill new line color
			for (int t = 0; t<4; t++)newLineCol[t] = 0;
			for (int j = 0; j<h; j++)
			{
				int i = w - 2;
				id = (y + j)*sz_x + (x + i);
				c = D[id];
				if (c>0)newLineCol[c]++;
				if (j == 0)c_edge1 = c;
				if (j == h - 1)c_edge2 = c;
			}
			{
				int i = w - 1;
				int j = 0;
				id = (y + j)*sz_x + (x + i);
				c = D[id];
				c_edge1_old = c;
				i = w - 1;
				j = h - 1;
				id = (y + j)*sz_x + (x + i);
				c = D[id];
				c_edge2_old = c;
			}

			// check if removing line is ok
			int flag = 1;
			for (int t = 1; t<4; t++) {
				if (newLineCol[t] == 0 && lineColCount[3][t]>0 && rimColCount[t] == lineColCount[3][t]) {
					flag = 0;
					break;
				}
			}
			// remove line
			if (flag == 1) {
				for (int t = 1; t<4; t++) {
					rimColCount[t] = rimColCount[t] - lineColCount[3][t] + newLineCol[t];
					lineColCount[3][t] = newLineCol[t];
				}
				// removing boundary two points
				rimColCount[c_edge1]--;
				rimColCount[c_edge2]--;

				lineColCount[0][c_edge1_old]--;
				lineColCount[1][c_edge2_old]--;

				w--;
				flag_shrink = 1;
			}
		} // right line

	} // while

	  // get center pixel
	x += floor(w / 2.0);
	y += floor(h / 2.0);

	*px = x;
	*py = y;
}

void getColorPurity(unsigned char *input_image, std::vector<unsigned char> &P, int sz_blk, int skip, int w, int h, float th_black, int nChannels)
{
	unsigned char th = th_black;//5
	
	int sz_x = floor(w / sz_blk);

	cv::Mat P_cv(cv::Size(w / m_sz_blk, h / m_sz_blk), CV_8UC1, P.data());

	double ts = omp_get_wtime();

#pragma omp parallel for
for (int idy = 0; idy < (int)floor(h / sz_blk); idy++) {
	for (int idx = 0; idx < (int)floor(w/sz_blk); idx++) {
			size_t id_blk = idy*sz_x + idx;
			
			float v_avg = 0;
			int count = 0;
			for (int i = 0; i < sz_blk; i += skip) {
				for (int j = 0; j < sz_blk; j += skip) {
					int x = idx*sz_blk + i;
					int y = idy*sz_blk + j;

					int id = y*w + x;
					PixelF pixelf;
					pixelf.x = input_image[id * nChannels + 0];
					pixelf.y = input_image[id * nChannels + 1];
					pixelf.z = input_image[id * nChannels + 2];
					
					float gr;


					if (pixelf.x >= pixelf.y && pixelf.y >= pixelf.z) {
						gr = pixelf.x;
						if (gr < th)continue;
						v_avg += (pixelf.x - pixelf.y) / gr;
					}
					if (pixelf.x >= pixelf.z && pixelf.z >= pixelf.y) {
						gr = pixelf.x;
						if (gr < th)continue;
						v_avg += (pixelf.x - pixelf.z) / gr;
					}
					if (pixelf.y >= pixelf.z && pixelf.z >= pixelf.x) {
						gr = pixelf.y;
						if (gr < th)continue;
						v_avg += (pixelf.y - pixelf.z) / gr;
					}
					if (pixelf.y >= pixelf.x && pixelf.x >= pixelf.z) {
						gr = pixelf.y;
						if (gr < th)continue;
						v_avg += (pixelf.y - pixelf.x) / gr;
					}
					if (pixelf.z >= pixelf.x && pixelf.x >= pixelf.y) {
						gr = pixelf.z;
						if (gr < th)continue;
						v_avg += (pixelf.z - pixelf.x) / gr;
					}
					if (pixelf.z >= pixelf.y && pixelf.y >= pixelf.x) {
						gr = pixelf.z;
						if (gr < th)continue;
						v_avg += (pixelf.z - pixelf.y) / gr;
					}
					count++;
				}
			}
			if (count<2) {
				P[id_blk] = 0;
			}
			else {
				v_avg /= count;

				if (v_avg > 0.3) {
					P[id_blk] = 1;
				}
				else {
					P[id_blk] = 0;
				}
			}
		}
	}
double te = omp_get_wtime();

printf("Purity in %lf ms\n", (te - ts) * 1000);
}


void colConversionGL(unsigned char *image_in, std::vector<unsigned char> &D, int w, int h, float th_black, int nChannels)
{
	double ts = omp_get_wtime();
	unsigned char R = 1, G = 2, B = 3;
	int th = th_black;
	int thr = (int)(th*0.8f / 1.4f);
	int thg = th;
	int thb = (int)(th*0.8f / 0.9f);

#pragma omp parallel for
	for (int j = 0; j < h; j++) {
		for (int i = 0; i < w; i++) {
			int id = j*w + i;
			unsigned char cr = (unsigned char)((float)image_in[id * nChannels + 0] * 0.8f/1.4f); //image_in
			unsigned char cg = (unsigned char)((float)image_in[id * nChannels + 1] * 1); //image_in
			unsigned char cb = (unsigned char)((float)image_in[id * nChannels + 2] * 0.8f/0.9f); //image_in

			unsigned char d = 0;
			if (cr<thr && cg<thg && cb<thb) { // 0.78
				d = 0;
			}
			else {
				if (cr > cg) {
					if (cr > cb) {
						d = R;
					}
					else {
						d = B;
					}
				}
				else {
					if (cg > cb) {
						d = G;
					}
					else {
						d = B;
					}
				}
			}
			D[id] = d;
		}
	}

	double te = omp_get_wtime();
	printf("color in %lf ms\n", (te-ts)*1000);

}

void displayCornersOnImage(cv::Mat &corners, cv::Mat &img_CV, char *fname_out) {
	for (int i = 0; i < 9; i++) {
		if (corners.at<float>(i, 0) != -1 || corners.at<float>(i, 1) != -1) {
			cv::circle(img_CV, cv::Point((int)corners.at<float>(i, 0), (int)corners.at<float>(i, 1)), 2, cv::Scalar(0));
			cv::String str;
			char buf[256];
			sprintf(buf, "%d", i);
			str = buf;
			cv::putText(img_CV, buf, cv::Point((int)corners.at<float>(i, 0), (int)corners.at<float>(i, 1)), CV_FONT_HERSHEY_COMPLEX, 1, cv::Scalar(0));
		}
	}

	cv::imwrite(fname_out, img_CV);
}

void writeCorners(char *fname_corners, cv::Mat &corners) {
	FILE *fid = fopen(fname_corners, "w");
	for (int i = 0; i < 9; i++) {
		fprintf(fid, "%f %f\n", corners.at<float>(i, 0), corners.at<float>(i, 1));
	}
	fclose(fid);
}
