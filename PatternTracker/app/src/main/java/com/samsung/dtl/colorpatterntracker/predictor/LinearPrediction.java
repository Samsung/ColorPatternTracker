package com.samsung.dtl.colorpatterntracker.predictor;

import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;

import com.samsung.dtl.colorpatterntracker.util.TUtil;

// TODO: Auto-generated Javadoc
/**
 * The Class LinearPrediction.
 */
public class LinearPrediction {
	
	private Mat mPrevCorners, mCornerVel; /*!<  The corner velocity. */
	
	private boolean [] mPrevCornersAvailable; /*!<  The indicator to show which previous corners are available. */
	
	int mNPatterns; /*!<  The number of patterns. */

	/**
	 * Instantiates a new linear prediction.
	 *
	 * @param _mNPatterns the number of patterns
	 */
	public LinearPrediction(int _mNPatterns){
		mNPatterns = _mNPatterns;
		
		mPrevCornersAvailable = new boolean[mNPatterns];
		for(int i=0;i<mNPatterns;i++)mPrevCornersAvailable[i]=false;
		
		mPrevCorners = Mat.ones(9*mNPatterns, 2, CvType.CV_32FC1);
		mCornerVel = Mat.zeros(9*mNPatterns, 2, CvType.CV_32FC1);
	}
	
	/**
	 * Estimate image corners velocity.
	 *
	 * @param dt the time spent since last frame
	 * @param mCorners the corner coordinates
	 * @param frameMetadataI the integer frame metadata 
	 */
	public void estimateImageCornersVel(double dt, Mat mCorners, Mat frameMetadataI){
		Core.subtract(mCorners, mPrevCorners, mCornerVel);
		mCornerVel = mCornerVel.mul(Mat.ones(9*mNPatterns, 2,CvType.CV_32FC1),1/(double)dt);
		for(int i=0;i<mNPatterns;i++){
			if(mPrevCornersAvailable[i] && frameMetadataI.get(i, 0)[0]>=0)frameMetadataI.put(i, 1, 1); // prev available && current available -> velocity available
		}		
	}
	
	/**
	 * Update previous corners.
	 *
	 * @param mCorners the detected corners
	 * @param frameMetadataI the integer frame metadata
	 */
	public void updatePrevCorners(Mat mCorners, Mat frameMetadataI){
		mCorners.copyTo(mPrevCorners);
		for(int i=0;i<mNPatterns;i++){
			if(frameMetadataI.get(i,0)[0]>=0){
				mPrevCornersAvailable[i]=true;
			}else{
				mPrevCornersAvailable[i]=false;
				frameMetadataI.put(i, 1, -1); // velocity not available
			}
		}
	}
	
	/**
	 * Estimate.
	 *
	 * @param dt the time spent since last frame
	 * @param mCorners the corners
	 * @param frameMetadataI the frame metadata
	 */
	public void Estimate(double dt, Mat mCorners, Mat frameMetadataI){
		estimateImageCornersVel(dt, mCorners, frameMetadataI);
		updatePrevCorners(mCorners, frameMetadataI);		
	}
	
	/**
	 * Predict image corners.
	 *
	 * @param dt the time spent since last frame
	 * @param mCorners the detected corners
	 * @param frameMetadataI the integer frame metadata
	 * @return the mat
	 */
	public Mat predictImageCorners(double dt, Mat mCorners, Mat frameMetadataI){
		long ts = System.nanoTime();   

		for(int i=0;i<mNPatterns;i++){
			if(frameMetadataI.get(i, 1)[0]==1){ // velocity available
				Mat cornersNew = new Mat();
				Core.addWeighted(mCorners.submat(i*9,(i+1)*9,0,2), 1, mCornerVel.submat(i*9,(i+1)*9,0,2), dt, 0, cornersNew);
				cornersNew.copyTo(mCorners.submat(i*9,(i+1)*9,0,2));
			}
		}
		TUtil.spent(ts,"predict corners");
		return mCorners;
	}
	
	/**
	 * Resets the predictor.
	 */
	public void clear(){
		for(int i=0;i<mNPatterns;i++)mPrevCornersAvailable[i] = false;
	}
}
