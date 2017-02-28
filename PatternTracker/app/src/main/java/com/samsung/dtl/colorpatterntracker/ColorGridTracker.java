package com.samsung.dtl.colorpatterntracker;

import java.util.Arrays;

import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;

import com.samsung.dtl.colorpatterntracker.predictor.KalamanFilter;
import com.samsung.dtl.colorpatterntracker.predictor.LinearPrediction;
import com.samsung.dtl.colorpatterntracker.stereo.Algo3D;
import com.samsung.dtl.colorpatterntracker.stereo.CameraModelParams;
import com.samsung.dtl.colorpatterntracker.util.HandlerExtension;
import com.samsung.dtl.colorpatterntracker.util.TUtil;

import android.os.Bundle;
import android.os.Message;
import android.util.Log;
import android.widget.TextView;

// TODO: Auto-generated Javadoc
/**
 * The Class ColorGridTracker.
 */
public class ColorGridTracker {
	
	/** The m n patterns. */
	public int mNPatterns; /*!< The number of patterns. */
		
	/** The m corners_tm_full. */
	public Mat mCorners_tm_full; /*!<  The template coordinates of full set of corners. */
	
	/** The m corners. */
	public Mat mCorners; /*!< The detected corner coordinates. */
	
	/** The m corner detected. */
	public int mCornerDetected; /*!< The index of detected corner. */
	
	/** The frame metadata f. */
	public Mat frameMetadataF; /*!< The floating point valued frame metadata containing image intensity. */
	
	/** The frame metadata i. */
	public Mat frameMetadataI; /*!< The integer valued frame metadata containing detected pattern ID 0->patternID[0][<0 -> no corner detected], 1->is velocity comptued. */ 
	
	/** The kalaman filter. */
	public KalamanFilter kalamanFilter; /*!< The kalaman filter. */
    
    /** The lin filter. */
    public LinearPrediction linFilter; /*!< The linear predictor. */
    
    /** The tracked origin. */
    private Mat trackedOrigin; /*!< The tracked origin coordinates. */

    /** The last capture time. */
    long lastCaptureTime; /*!< The last capture time. */
    
    /** The m cam model params. */
    public CameraModelParams mCamModelParams; /*!< The camera model parameters. */
    
    /** The m update rel transf. */
    public boolean mUpdateRelTransf; /*!< The update relative transformation. */
      
    /** The m debug level. */
    public int mDebugLevel; /*!< The debug level. */    
    
    /** The text view handler. */
    public HandlerExtension textViewHandler; /*!< The text view handler to show which patterns have been learned. */
    
    /** The m text detected patterns. */
    public TextView mTextDetectedPatterns; /*!< The text showing the learned patterns. */
        
    // jni

	static {
		//System.load("/vendor/lib/egl/libGLESv1_CM_adreno.so");
		//System.load("/vendor/lib/egl/libGLESv2_adreno.so");
		//System.load("/vendor/lib/egl/libGLES_mali.so");
		System.loadLibrary("cgt");
	}


	/**
	 * Initializes the cl.
	 *
	 * @param width the frame width
	 * @param height the frame height
	 * @param input_texid the input texture ID
	 * @param output_texid the output texture ID
	 */
	public static native void initCL(int width, int height, int input_texid, int output_texid);
	
	/**
	 * Process the frame input from camera.
	 *
	 * @param mCornerAddress the native address of detected corner matrix
	 * @param mMetadataFAddress the native address of frame metadata
	 * @param mMetadataIAddress the native address of frame metadata
	 * @param frameNo the frame number
	 * @param dt the spent since last frame
	 * @param mUpdateRelTransf the switch to update relative transformation
	 */
	public static native void processFrame(long mCornerAddress, long mMetadataFAddress, long mMetadataIAddress, int frameNo, double dt, boolean mUpdateRelTransf);
	
	/**
	 * Destroy cl.
	 */
	public static native void destroyCL();

	/**
	 * Load opencl.
	 */
	public void loadLibrary_opencl(){
		java.io.File file = new java.io.File("/system/vendor/lib/libOpenCL.so");
		if(file.exists()){
			Log.e("TRAC","lib 1");
			System.load("/vendor/lib/egl/libGLESv1_CM_adreno.so");
			//System.load("/vendor/lib/egl/libGLESv2_adreno.so");
			//System.load("/vendor/lib/egl/libGLES_mali.so");
			//System.load("/system/vendor/lib/libOpenCL.so");
			System.loadLibrary("cgt");
			return;
		}

		file = new java.io.File("/system/lib/libOpenCL.so");
		if(file.exists()){
			Log.e("TRAC","lib 2");
			System.load("/system/lib/libOpenCL.so");
			System.loadLibrary("cgt");
			return;
		}

		file = new java.io.File("/system/vendor/lib/egl/libGLES_mali.so");
		if(file.exists()){
			Log.e("TRAC","lib 3");
			System.load("/system/vendor/lib/egl/libGLES_mali.so");
			System.loadLibrary("cgt");
			return;
		}

		file = new java.io.File("/system/lib/egl/libGLES_mali.so");
		if(file.exists()){
			Log.e("TRAC","lib 4");
			System.load("/system/lib/egl/libGLES_mali.so");
			System.loadLibrary("cgt");
			return;
		}

		file = new java.io.File("/vendor/lib/egl/libGLESv1_CM_adreno.so");
		if(file.exists()){
			Log.e("TRAC","lib 5");
			System.load("/vendor/lib/egl/libGLESv1_CM_adreno.so");
			System.loadLibrary("cgt");
			return;
		}

		file = new java.io.File("/vendor/lib/egl/libGLESv2_adreno.so");
		if(file.exists()){
			Log.e("TRAC","lib 6");
			System.load("/vendor/lib/egl/libGLESv2_adreno.so");
			System.loadLibrary("cgt");
			return;
		}

		file = new java.io.File("/system/vendor/lib/libPVROCL.so");
		if(file.exists()){
			Log.e("TRAC","lib 7");
			System.load("/system/vendor/lib/libPVROCL.so");
			System.loadLibrary("cgt");
			return;
		}
	}

	/**
	 * Instantiates a new color grid tracker.
	 */
	public ColorGridTracker(){
		//loadLibrary_opencl();
		initVars();
	}
	
	/**
	 * Track grid.
	 *
	 * @param height_frame the frame height
	 * @param width_frame the frame width
	 * @param frameNo the frame number
	 * @param captureTime the capture time
	 * @return the tracked origin coordinates
	 */
	public Mat trackGrid(int height_frame, int width_frame, int frameNo, long captureTime){		
		double dt = timeSinceLastCapture(captureTime);    
		
		mCorners = linFilter.predictImageCorners(dt, mCorners, frameMetadataI);
		
		processFrame(mCorners.getNativeObjAddr(), frameMetadataF.getNativeObjAddr(), frameMetadataI.getNativeObjAddr(), frameNo, dt, mUpdateRelTransf);

		if(getCornerDetected()==-1){ 
			initializeMetadata();linFilter.clear();			 
			return Mat.zeros(0, 0,CvType.CV_32FC1); 
		}
		
		updateRTAndCornersUsingCorres();
		linFilter.Estimate(dt, mCorners, frameMetadataI);
		
		if(frameMetadataI.get(0, 2)[0]==-1){
			initializeMetadata();
			return Mat.zeros(0, 0,CvType.CV_32FC1);
		}
						
		Mat origin = getLocationOfOrigin();
		if(!updateOrigin_Kalman(dt, origin)){
			kalamanFilter.initializeKalmanJava(dt);
			displayTrackedPatterns();
			return Mat.zeros(0, 0,CvType.CV_32FC1);
		}
		
		displayTrackedPatterns();

		return trackedOrigin;
	}
	
	/**
	 * Update Rotation and Translation and corners using point correspondences.
	 */
	private void updateRTAndCornersUsingCorres(){
		updateRTUsingCorres(mCorners_tm_full, Algo3D.getCalibratedPoints(mCorners,mCamModelParams));
    	populateCornersAndGetFrontalCorner();		
	}
			
	/**
	 * Initializes the variables.
	 */
	private void initVars(){
		// model
		mNPatterns = 6;
		mCamModelParams = new CameraModelParams(mNPatterns);
		mUpdateRelTransf=false;
		
		// corners
		initializeCorners();
		initializeTemplateCorners();
		initializeMetadata();
				
		// tracker
        trackedOrigin = Mat.zeros(6, 1, CvType.CV_64FC1);
		kalamanFilter = new KalamanFilter();
		kalamanFilter.initializeKalmanJava(0);
		linFilter = new LinearPrediction(mNPatterns);
		lastCaptureTime=0;
		
		// debug
		mDebugLevel=1;		
	}
	
	/**
	 * Update kalman filter.
	 *
	 * @param dt the time spent since last frame
	 * @param origin the origin coordinates
	 * @return true, if successful
	 */
	private boolean updateOrigin_Kalman(double dt, Mat origin){
		if(origin.rows()==0)return false;
	    kalamanFilter.kalman_y_n.put(0,0,origin.get(0,0));
	    kalamanFilter.kalman_y_n.put(1,0,origin.get(1,0));
	    kalamanFilter.kalman_y_n.put(2,0,origin.get(2,0));
	    kalamanFilter.updateKalmanJava(dt, null);
	    			
		// update trackedOrigin
		trackedOrigin.put(0, 0, kalamanFilter.kalman_m_n1.get(0,0)[0]);
		trackedOrigin.put(1, 0, kalamanFilter.kalman_m_n1.get(1,0)[0]);
		trackedOrigin.put(2, 0, kalamanFilter.kalman_m_n1.get(2,0)[0]);
		
		return true;
	}
	
	/**
	 * Time spent since last capture.
	 *
	 * @param captureTime the capture time
	 * @return the Time spent since last capture
	 */
	private double timeSinceLastCapture(long captureTime){
		double dt=(captureTime-lastCaptureTime)/1000000000.0;
		lastCaptureTime = captureTime;
		return dt;
	}
	
	/**
	 * Gets the frontal pattern.
	 *
	 * @return the frontal pattern
	 */
	private void getFrontalPattern(){
		// get shortest side
		Paird [] dVec = new Paird[mNPatterns];
		
		for(int i=0;i<mNPatterns; i++){
			if(mCamModelParams.mRotAll[i].rows()>0){
				Mat corners = new Mat();
				mCorners.submat(i*9,(i+1)*9,0,2).assignTo(corners);
				dVec[i] = new Paird(i,Algo3D.getShortestSide(corners.get(0, 0)[0], corners.get(0, 1)[0], corners.get(6, 0)[0], corners.get(6, 1)[0], corners.get(8, 0)[0], corners.get(8, 1)[0], corners.get(2, 0)[0], corners.get(2, 1)[0]));
			}else{
				dVec[i]=new Paird(i,-1.0);
			}
		}
				
		// sort the dVec
		Arrays.sort(dVec);
		
		for(int i=0;i<mNPatterns; i++){
			if(dVec[i].value>0){
				frameMetadataI.put(i, 2, dVec[i].index);
			}else{
				frameMetadataI.put(i, 2, -1);
			}
		}
	}
	
	/**
	 * Populate corners.
	 */
	private void populateCorners(){
		for(int i=0;i<mNPatterns;i++){
			if(frameMetadataI.get(i,0)[0]<0 && mCamModelParams.mRotAll[i].rows()>0){
				Mat corners3 = Mat.ones(9,3, CvType.CV_32FC1);
				Mat corner3_2 = corners3.submat(0,9,0,2);
				mCorners_tm_full.copyTo(corner3_2);
				Mat corners3Trans = new Mat();
				Mat tr = new Mat(); 
				Core.repeat(mCamModelParams.mTrAll[i], 1, corners3.rows(), tr);
				Core.gemm(mCamModelParams.mRotAll[i], corners3.t(), 1, tr, 1, corners3Trans);
				for(int j=0;j<9;j++){
					corners3Trans.put(0, j, corners3Trans.get(0, j)[0]/corners3Trans.get(2, j)[0]);
					corners3Trans.put(1, j, corners3Trans.get(1, j)[0]/corners3Trans.get(2, j)[0]);
				}
				corners3Trans = corners3Trans.t();
				Mat corners2Trans = new Mat();
				corners3Trans.submat(0,9,0,2).assignTo(corners2Trans);
				corners2Trans = Algo3D.getUnCalibratedPoints(corners2Trans,mCamModelParams);
				for(int j=0;j<9;j++){
					mCorners.put(i*9+j,0,corners2Trans.get(j,0)[0]);
					mCorners.put(i*9+j,1,corners2Trans.get(j,1)[0]);
				}
				
				frameMetadataI.put(i,0,i);
				frameMetadataI.put(i,1,-1);
			}
		}
	}
	
	/**
	 * Populate corners and get frontal corner.
	 */
	private void populateCornersAndGetFrontalCorner(){
		populateCorners();
		getFrontalPattern();
	}
		
	/**
	 * Gets the corner detected.
	 *
	 * @return the corner detected
	 */
	private int getCornerDetected(){
		for(int i=0;i<mNPatterns;i++){
			if(frameMetadataI.get(i,0)[0]>=0){
				mCornerDetected=i;
				return i;
			}
		}
		mCornerDetected=-1;
		return -1;
	}
        
    /**
     * Gets the location of origin.
     *
     * @return the location of origin
     */
    public Mat getLocationOfOrigin()
    {	    	
    	Mat Loc_origin = new Mat(0,0,CvType.CV_32FC1);
    	Mat origin = Mat.zeros(3, 1, CvType.CV_32FC1);
    	if(mCamModelParams.mRotAll[0].rows()!=0){
	    	origin.put(0, 0, mCorners_tm_full.get(4, 0)[0]);
	    	origin.put(1, 0, mCorners_tm_full.get(4, 1)[0]);
	    	origin.put(2, 0, 1);
	    	Core.gemm(mCamModelParams.mRotAll[0], origin, 1, mCamModelParams.mTrAll[0], 1, Loc_origin, 0);
    	}
    	return Loc_origin;
    }
    
    /**
     * Updates the absolute Rotation and Translation from point correspondences using Alg. 5.2 - Invitation to 3D Vision Ref:http://vision.ucla.edu//MASKS/MASKS-ch5.pdf
     *
     * @param p the first point set
     * @param q the second point set
     * @return 1 if successful
     */
    public int getRTsFromCorres(Mat p, Mat q){
    	long ts = System.nanoTime();
    	
    	for(int i=0;i<mNPatterns;i++){
    		if(frameMetadataI.get(i,0)[0]>=0){
    			// computing the R and T [Alg. 5.2 - Invitation to 3D Vision Ref:http://vision.ucla.edu//MASKS/MASKS-ch5.pdf]
    			Mat q_this = q.submat(i*9,(i+1)*9,0,2);
    			
    			Mat H=Algo3D.getHFromCorres(p, q_this);
    			
    			Mat TransMat = Algo3D.getRTFromH(H, -1);
    			
    			if(TransMat.get(0,4)[0]!=-1){
    				TransMat.submat(0,3,0,3).copyTo(mCamModelParams.mRotAll[i]);
    				TransMat.submat(0,3,3,4).copyTo(mCamModelParams.mTrAll[i]);
    			}else{ // RT estimation failed
    				frameMetadataI.put(i, 0, -1);
    				frameMetadataI.put(i, 1, -1);
    				mCamModelParams.mRotAll[i] = new Mat(0,0,CvType.CV_32FC1);
    				mCamModelParams.mTrAll[i] = new Mat(0,0,CvType.CV_32FC1);
    			}
    		}else{
    			if(mCamModelParams.mRotAll[i].rows()!=0){
    				mCamModelParams.mRotAll[i] = new Mat(0,0,CvType.CV_32FC1);
    				mCamModelParams.mTrAll[i] = new Mat(0,0,CvType.CV_32FC1);
    			}
    		}
    	}
    	
    	TUtil.spent(ts,"RTsFromCorres");
		return 1;
    }
    
    /**
     * Update rt using corres.
     *
     * @param p the p
     * @param q the q
     */
    private void updateRTUsingCorres(Mat p, Mat q){
    	getRTsFromCorres(p, q);
    	mCamModelParams.updateRelAndAbsTransf(frameMetadataI, mUpdateRelTransf);
    }
    
	/**
	 * Display tracked patterns.
	 */
	private void displayTrackedPatterns(){
		String outStr="0";
		for(int i=1;i<mNPatterns;i++){
			if(mCamModelParams.mTransfAvailableIndicator[0][i]){
				outStr = outStr+", "+i;
			}
		}
		
		String textStr = (String) mTextDetectedPatterns.getText().toString();
		
		if(!outStr.equals(textStr)){
		    Bundle msgBundle = new Bundle();
		    msgBundle.putString("result", outStr);
		    Message msg = new Message();
		    msg.setData(msgBundle);
		    textViewHandler.sendMessage(msg);
		}
	}
	
	/**
	 * The Class Paird for sorting.
	 */
	public class Paird implements Comparable<Paird> {
	    
    	/** The index. */
    	public final int index;
	    
    	/** The value. */
    	public final double value;

	    /**
    	 * Instantiates a new paird.
    	 *
    	 * @param index the index
    	 * @param value the value
    	 */
    	public Paird(int index, double value) {
	        this.index = index;
	        this.value = value;
	    }

	    /* (non-Javadoc)
    	 * @see java.lang.Comparable#compareTo(java.lang.Object)
    	 */
    	@Override
	    public int compareTo(Paird other) {
	        //multiplied to -1 as the author need descending sort order
	        return -1 * Double.valueOf(this.value).compareTo(other.value);
	    }
	}
	
	/**
	 * Initialize corners.
	 */
	private void initializeCorners(){
		mCorners = Mat.ones(9*mNPatterns, 2, CvType.CV_32FC1); 
		mCorners = mCorners.mul(Mat.ones(9*mNPatterns, 2, CvType.CV_32FC1), -1);
		mCornerDetected=-1;
	}
	
	/**
	 * Initialize metadata.
	 */
	private void  initializeMetadata(){
		frameMetadataF = Mat.zeros(mNPatterns, 1, CvType.CV_32FC1);// 1->intensity
		frameMetadataI = Mat.ones(mNPatterns, 3, CvType.CV_32SC1); // 1->detected PatternID 2->velocity available 3->clarity order 
		frameMetadataI = frameMetadataI.mul(frameMetadataI, -1);		
	}
	
	/**
	 * Initialize template corners.
	 */
	private void initializeTemplateCorners(){
    	float w_actual = 10.6f/4.0f;
    	float h_actual = 7.1f/4.0f;

		mCorners_tm_full = new Mat(9,2,CvType.CV_32FC1);
        for(int i=0;i<3;i++){
        	for(int j=0;j<3;j++){
        		mCorners_tm_full.put(j*3+i,0,w_actual*(j+1-2));
        		mCorners_tm_full.put(j*3+i,1,h_actual*(i+1-2));
        	}
        }
	}
}
