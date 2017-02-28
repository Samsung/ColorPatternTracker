package com.samsung.dtl.patterntracker.stereo;

import org.opencv.core.Core;
import org.opencv.core.CvType;
import org.opencv.core.Mat;

import com.samsung.dtl.patterntracker.util.TUtil;


// TODO: Auto-generated Javadoc
/**
 * The Class CameraModelParams.
 */
public class CameraModelParams {
    
    
    public Mat [] mRotAll; /*!< The absolute rotation matrices for different patterns. */
    public Mat [] mTrAll; /*!< The absolute translation matrices for different patterns. */
    public boolean [][] mTransfAvailableIndicator; /*!< The relative transformation available indicator. */
    public Mat [][] mRotRelAll; /*!< The relative rotation matrices for different patterns. */
    public Mat [][] mTrRelAll; /*!< The relative translation matrices for different patterns. */
    public Mat mKK, mKKInv; /*!< The internal camera calibration matrix. */
    public float mKc1, mKc2; /*!< The camera distortion parameters. */
    int mNPatterns; /*!< The number of patterns being tracked. */
    
    /**
     * Instantiates a new camera model parameters.
     *
     * @param _mNPatterns the number of patterns
     */
    public CameraModelParams(int _mNPatterns){
    	mNPatterns = _mNPatterns;
	    mTransfAvailableIndicator = new boolean[mNPatterns][mNPatterns];
		mRotAll = new Mat[mNPatterns];
		mTrAll = new Mat[mNPatterns];
		mRotRelAll = new Mat[mNPatterns][mNPatterns];
		mTrRelAll = new Mat[mNPatterns][mNPatterns];
				
		for(int i=0;i<mNPatterns;i++){
			mRotAll[i] = Mat.zeros(0,0,CvType.CV_32FC1);
			mTrAll[i]  = Mat.zeros(0,0,CvType.CV_32FC1);
			for(int j=0;j<mNPatterns;j++){
				mTransfAvailableIndicator[i][j] = false;
				mRotRelAll[i][j] = Mat.zeros(3,3,CvType.CV_32FC1);
				mTrRelAll[i][j]  = Mat.zeros(3,1,CvType.CV_32FC1);
			}
		}
		
        mKK=Mat.zeros(3, 3, CvType.CV_32FC1);

    	mKK.put(0,0,1424.2);
    	mKK.put(1,1,1424.9);
    	mKK.put(0,2,537.5344);
    	mKK.put(1,2,938.9930);
    	mKK.put(2,2,1);
    	mKKInv = mKK.inv();
    	
    	mKc1 = 0.1544f;
    	mKc2 =-0.2669f;
    }
    
    /**
     * Update absolute transformation.
     *
     * @param frameMetadataI the integer valued frame metadata
     */
    public void updateAbsTransf(Mat frameMetadataI){
    	// populate R and T
    	for(int i=0;i<mNPatterns;i++){
    		for(int j=0;j<mNPatterns;j++){
    			if(frameMetadataI.get(i,0)[0]>=0 && frameMetadataI.get(j,0)[0]<0 && mTransfAvailableIndicator[i][j]){
	    			Core.gemm(mRotAll[i], mRotRelAll[i][j], 1, new Mat(), 0, mRotAll[j],0);
	    			Core.gemm(mRotAll[i], mTrRelAll[i][j], 1, mTrAll[i], 1, mTrAll[j],0);
    			}
    		}
    	}
    }
    
    /**
     * Update relative transformation.
     *
     * @param frameMetadataI the integer valued frame metadata
     * @param mUpdateRelTransf the update relative transformation
     */
    public void updateRelTransf(Mat frameMetadataI, boolean mUpdateRelTransf){
    	long ts = System.nanoTime();
    	
    	for(int i=0;i<mNPatterns;i++){
    		for(int j=i+1;j<mNPatterns;j++){ 
	    		if(frameMetadataI.get(i,0)[0]>=0 && frameMetadataI.get(j,0)[0]>=0 && (!mTransfAvailableIndicator[i][j] || mUpdateRelTransf)){
	    			setTransfAvailable(i,j);
	    				    			
	    			Core.gemm(mRotAll[i].t(), mRotAll[j], 1, new Mat(), 0, mRotRelAll[i][j],0);
	    			Mat Tj_m_Ti = new Mat(); // Tj-Ti
	    			Core.subtract(mTrAll[j], mTrAll[i], Tj_m_Ti);
	    			Core.gemm(mRotAll[i].t(), Tj_m_Ti, 1, new Mat(), 0, mTrRelAll[i][j],0);
	    			
	    			setTransfAvailable(j,i);
	    			mRotRelAll[i][j].t().copyTo(mRotRelAll[j][i]);
	    			Core.gemm(mRotRelAll[i][j].t(), mTrRelAll[i][j], -1, new Mat(), 0, mTrRelAll[j][i],0);
	    				    			
	    			
	    	    	for(int icon=0;icon<mNPatterns;icon++){ 
	    				if(icon==i || icon==j)continue;
	    				if(mTransfAvailableIndicator[icon][i] && !mTransfAvailableIndicator[icon][j]){
	    					setTransfAvailable(icon,j);
	    					Core.gemm(mRotRelAll[icon][i], mRotRelAll[i][j], 1, new Mat(), 0, mRotRelAll[icon][j],0);
	    	    			Core.gemm(mRotRelAll[icon][i], mTrRelAll[i][j], 1, mTrRelAll[icon][i], 1, mTrRelAll[icon][j],0);
	    	    				    	    			
	    	    			setTransfAvailable(j,icon);
	    	    			mRotRelAll[icon][j].t().copyTo(mRotRelAll[j][icon]);
	    	    			Core.gemm(mRotRelAll[icon][j].t(), mTrRelAll[icon][j], -1, new Mat(), 0, mTrRelAll[j][icon],0);
	    				
		    				for(int jcon=0;jcon<mNPatterns;jcon++){
		    					if(jcon==j || jcon==i)continue;
			    				if(mTransfAvailableIndicator[j][jcon] && !mTransfAvailableIndicator[icon][jcon]){
			    					setTransfAvailable(icon,jcon);
			    	    			Core.gemm(mRotRelAll[icon][j], mRotRelAll[j][jcon], 1, new Mat(), 0, mRotRelAll[icon][jcon],0);
			    	    			Core.gemm(mRotRelAll[icon][j], mTrRelAll[j][jcon], 1, mTrRelAll[icon][j], 1, mTrRelAll[icon][jcon],0);
			    	    			
			    	    			setTransfAvailable(jcon,icon);
			    	    			mRotRelAll[icon][jcon].t().copyTo(mRotRelAll[jcon][icon]);
			    	    			Core.gemm(mRotRelAll[icon][jcon].t(), mTrRelAll[icon][jcon], -1, new Mat(), 0, mTrRelAll[jcon][icon],0);
			    				}	    					
		    				}
	    				}
	    			}
	    		}
    		}
    	}
    	
    	TUtil.spent(ts,"populate Rel Transf");
    }
    
    public void updateRelAndAbsTransf(Mat frameMetadataI, boolean mUpdateRelTransf){
    	updateRelTransf(frameMetadataI, mUpdateRelTransf);
    	updateAbsTransf(frameMetadataI);
    }
    
    /**
     * Sets the transformation as available.
     *
     * @param i the first pattern
     * @param j the second pattern
     */
    private void setTransfAvailable(int i, int j){
    	mTransfAvailableIndicator[i][j] = true;
    }
}
