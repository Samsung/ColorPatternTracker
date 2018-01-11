package com.samsung.dtl.colorpatterntracker.camera;

import java.io.IOException;
import java.lang.reflect.Array;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

import org.opencv.core.Mat;

import com.samsung.dtl.colorpatterntracker.ColorGridTracker;
import com.samsung.dtl.colorpatterntracker.R;

import android.graphics.Point;
import android.graphics.Rect;
import android.graphics.SurfaceTexture;
import android.hardware.Camera;
import android.hardware.Camera.AutoFocusCallback;
import android.util.Log;
import android.widget.Button;

// TODO: Auto-generated Javadoc
/**
 * The Class CameraManager.
 */
public class CameraManager {

	public Camera mCamera; /*!<  The camera. */
	public int frameNo; /*!<  The frame number. */
	public int lastTrackedFrameNo; /*!<  The last tracked frame number.. */
	public boolean allowFocusUpdate;
	public boolean allowExposureUpdate;
	public boolean allowWBUpdate;
	public int mMaxExposure, mMinExposure, mExposureSeekbarProgress;
	public int colorPixelDiff; /*!<  Difference of pattern pixel values from the required values */
	public List<String> supportedWBList;
	public int [][] colorPixelDiffAll;
	public int nColorPixelDiffVals;
	public int nWBCaptureDone;
	public Button button_wb;

	private float mFocusZ; /*!<  The z component of the last tracked frame for focus setting. */
	private int patternBoundingBoxLeft,patternBoundingBoxTop,patternBoundingBoxRight,patternBoundingBoxBottom; /*!<  The pattern bounding box coordinates. */
	private int mExposureChangeDir; /*!<  Exposure change direction indicating increase or decrease in exposure. 1:increasing -1:decreasing. */
	
	private final String TAG = "tracker";
	
	/**
	 * Initialize camera.
	 *
	 * @param camera_res the camera resolution
	 * @param mSTexture the camera surface texture
	 */
	public void initializeCamera(Point camera_res, SurfaceTexture mSTexture){
		frameNo = 0;
		lastTrackedFrameNo = 0;
		allowFocusUpdate = true;
		allowExposureUpdate = false;
		mExposureSeekbarProgress = 0;
		colorPixelDiff = -1;
		nColorPixelDiffVals = 10;
		allowWBUpdate = false;
		nWBCaptureDone = 0;
		
		mFocusZ=-1;
		mExposureChangeDir=1;

		//doWhiteBalance = true;
		
		mCamera = Camera.open();
		initCameraParametersForVideo(camera_res);
		
		try {
			mCamera.setPreviewTexture(mSTexture);
		} catch ( IOException ioe ) {}

		Camera.Parameters param;
		param = mCamera.getParameters();
		mMaxExposure = param.getMaxExposureCompensation();
		mMinExposure = param.getMinExposureCompensation();
		int exposureVal = param.getExposureCompensation();
		mExposureSeekbarProgress = (int)Math.round((exposureVal-mMinExposure)/(double)(mMaxExposure-mMinExposure)*100);

		supportedWBList = param.getSupportedWhiteBalance();

		colorPixelDiffAll = new int[supportedWBList.size()][nColorPixelDiffVals];

		Log.d(TAG, "Camera Resolution: " + mCamera.getParameters().getPictureSize().width + "x" + mCamera.getParameters().getPictureSize().height);
	}
	
	/**
	 * Update camera parameters.
	 *
	 * @param mCgTrack the Tracker instance
	 * @param origin the origin coordinate
	 * @param camera_res the camera resolution
	 */
	public void updateCameraParams(ColorGridTracker mCgTrack, Mat origin, Point camera_res){
		if(origin.rows()!=0){ // pattern detected
			lastTrackedFrameNo=frameNo;
			if(allowFocusUpdate) {
				updateFocus(mCgTrack, origin, camera_res);
			}
			//refineExposure(mCgTrack);
			if(allowWBUpdate){
				updateWhiteBalance();
			}
		}else{
			if(frameNo-lastTrackedFrameNo>10){ // long time since anything tracked
				//scanExposure();
			}
		}
		if(allowExposureUpdate) {
			setSeekbarBasedExposure();
			//refineExposure(mCgTrack);
		}
	}

	private void updateWhiteBalance(){
		if(colorPixelDiff==-1)return;
		Camera.Parameters param = mCamera.getParameters();

		int id_WB = nWBCaptureDone/nColorPixelDiffVals;
		int nWBCapturePerSetting =nWBCaptureDone%nColorPixelDiffVals;
		if(nWBCapturePerSetting==0){
			param.setWhiteBalance(supportedWBList.get(id_WB));
			colorPixelDiffAll[id_WB][nWBCapturePerSetting] = 0;
		}else{
			colorPixelDiffAll[id_WB][nWBCapturePerSetting] = colorPixelDiff;
			Log.e("cgt", "DiffVals "+colorPixelDiff);
		}
		nWBCaptureDone++;

		if(nWBCaptureDone==supportedWBList.size()*nColorPixelDiffVals){

			int [] diffVals = new int[nColorPixelDiffVals];

			int val_min=1000000;
			int id_min=-1;
			for(int i=0;i<supportedWBList.size();i++){
				if(supportedWBList.get(i).equals(param.WHITE_BALANCE_CLOUDY_DAYLIGHT) || supportedWBList.get(i).equals(param.WHITE_BALANCE_DAYLIGHT) || supportedWBList.get(i).equals(param.WHITE_BALANCE_FLUORESCENT) || supportedWBList.get(i).equals(param.WHITE_BALANCE_INCANDESCENT) || supportedWBList.get(i).equals(param.WHITE_BALANCE_SHADE) || supportedWBList.get(i).equals(param.WHITE_BALANCE_TWILIGHT) || supportedWBList.get(i).equals(param.WHITE_BALANCE_WARM_FLUORESCENT)) {

					for (int j = 0; j < nColorPixelDiffVals; j++) {
						diffVals[j] = colorPixelDiffAll[i][j];
					}
					Arrays.sort(diffVals);
					int val = diffVals[nColorPixelDiffVals / 2];
					//Log.e("cgt", "DiffVals" + supportedWBList.get(i) + ":[" +val +","+ colorPixelDiffAll[i][0] + "," + colorPixelDiffAll[i][1] + "," + colorPixelDiffAll[i][2] + "," + colorPixelDiffAll[i][3] + "," + colorPixelDiffAll[i][4] + "," + colorPixelDiffAll[i][5] + "," + colorPixelDiffAll[i][6] + "," + colorPixelDiffAll[i][7] + "," + colorPixelDiffAll[i][8] + "," + colorPixelDiffAll[i][9]+"]");
					if (val < val_min) {
						val_min = val;
						id_min = i;
					}
				}
			}
			param.setWhiteBalance(supportedWBList.get(id_min));
			//Log.e("cgt", "DiffVals selected" + supportedWBList.get(id_min));
			allowWBUpdate=false;
			nWBCaptureDone=0;
			//button_wb.setText("Mod WB: Off");

		}
		mCamera.setParameters(param);
	}

	private void setSeekbarBasedExposure(){
		Camera.Parameters param = mCamera.getParameters();
		int exposure = (int)Math.round(mExposureSeekbarProgress/(double)100*(mMaxExposure-mMinExposure)+mMinExposure);

		param.setExposureCompensation(exposure);

		mCamera.setParameters(param);
	}

    /**
     * Do focus based on pattern bounding box.
     */
    // focus on bounding box
    public void doTouchFocus() {
    	final Rect targetFocusRect = new Rect(patternBoundingBoxLeft,patternBoundingBoxTop,patternBoundingBoxRight,patternBoundingBoxBottom);
        try {
            List<Camera.Area> focusList = new ArrayList<Camera.Area>();
            focusList.add(new Camera.Area(targetFocusRect, 1000));
      
            Camera.Parameters param = mCamera.getParameters();
            param.setFocusAreas(focusList);
            //param.setMeteringAreas(focusList);
            mCamera.setParameters(param);
      
            mCamera.autoFocus(myAutoFocusCallback);
            Log.i(TAG, "focus changed");
        } catch (Exception e) {
            e.printStackTrace();
            Log.i(TAG, "Unable to focus");
        }
    }
    
	/**
	 * Sets the pattern bounding box.
	 *
	 * @param mCgTrack the tracker instance
	 * @param camera_res the camera resolution
	 */
	public void setPatternBoundingBox(ColorGridTracker mCgTrack, Point camera_res){
		float minx, miny, maxx, maxy;
		minx=Float.MAX_VALUE;
		maxx=0;
		miny=Float.MAX_VALUE;
		maxy=0;
		for(int i=0;i<9;i++){
			if(mCgTrack.mCorners.get(mCgTrack.mCornerDetected*9+i,0)[0]<minx)minx=(float) mCgTrack.mCorners.get(mCgTrack.mCornerDetected*9+i,0)[0];
			if(mCgTrack.mCorners.get(mCgTrack.mCornerDetected*9+i,0)[0]>maxx)maxx=(float) mCgTrack.mCorners.get(mCgTrack.mCornerDetected*9+i,0)[0];
			
			if(mCgTrack.mCorners.get(mCgTrack.mCornerDetected*9+i,1)[0]<miny)miny=(float) mCgTrack.mCorners.get(mCgTrack.mCornerDetected*9+i,1)[0];
			if(mCgTrack.mCorners.get(mCgTrack.mCornerDetected*9+i,1)[0]>maxy)maxy=(float) mCgTrack.mCorners.get(mCgTrack.mCornerDetected*9+i,1)[0];			
		}
		int top = (int) Math.floor(minx/(float)camera_res.y*2000)-1000;
		int bot = (int) Math.floor(maxx/(float)camera_res.y*2000)-1000;
		
		int left = (int) Math.floor(miny/(float)camera_res.x*2000)-1000;
		int right = (int) Math.floor(maxy/(float)camera_res.x*2000)-1000;
		
		patternBoundingBoxLeft = left;
		patternBoundingBoxRight = right;
		
		patternBoundingBoxTop = -bot;
		patternBoundingBoxBottom = -top;
				
		Log.i("focus", " Left:"+patternBoundingBoxLeft+" Right:"+patternBoundingBoxRight+" Top:"+patternBoundingBoxTop+" Bottom"+patternBoundingBoxBottom );
	}
		
	/**
	 * Refine exposure.
	 *
	 * @param mCgTrack the tracker instance
	 */
	private void refineExposure(ColorGridTracker mCgTrack){
		if(mCgTrack.frameMetadataF.get(mCgTrack.mCornerDetected,0)[0]<0.4){
			changeExposure(1);
		}
		if(mCgTrack.frameMetadataF.get(mCgTrack.mCornerDetected,0)[0]>0.6){
			changeExposure(-1);
		}
	}
	
	/**
	 * Increase/decrease exposure.
	 */
	private void scanExposure(){
		if(mExposureChangeDir==1){
			if(!changeExposure(1)){
				mExposureChangeDir=-1;
				changeExposure(-2);
			}
		}else{
			if(!changeExposure(-1)){
				mExposureChangeDir=1;
				changeExposure(2);
			}
		}
	}
	
	/**
	 * Update focus.
	 *
	 * @param mCgTrack the tracker instance
	 * @param origin the origin coordinate
	 * @param camera_res the camera resolution
	 */
	private void updateFocus(ColorGridTracker mCgTrack, Mat origin, Point camera_res){
    	if(mFocusZ==-1 || Math.abs((mFocusZ-(float)origin.get(2,0)[0])/(float)origin.get(2,0)[0])>0.3){
    		setPatternBoundingBox(mCgTrack,camera_res);
            doTouchFocus();
            mFocusZ = (float)origin.get(2,0)[0];
    	}
	}
	
	/**
	 * Change exposure.
	 *
	 * @param val the change val
	 * @return true, if successful
	 */
	private boolean changeExposure(int val){
		Camera.Parameters param;
		int minExposure, maxExposure;
		param = mCamera.getParameters();
		int exposure = param.getExposureCompensation();
		exposure+=val;
		if(val>0){
			maxExposure = param.getMaxExposureCompensation();
			if(exposure<maxExposure){
				param.setExposureCompensation(exposure);
			}else{
				return false;
			}
		}else{
			minExposure = param.getMinExposureCompensation();
			if(exposure>minExposure){
				param.setExposureCompensation(exposure);
			}else{
				return false;
			}
		}
		mCamera.setParameters(param);
		return true;
	}
	
    /**
     * Initializes the camera parameters for video.
     *
     * @param camera_res the camera resolution
     */
    private void initCameraParametersForVideo(Point camera_res){
		Camera.Parameters param = mCamera.getParameters();				
		param.setWhiteBalance(Camera.Parameters.WHITE_BALANCE_AUTO);
		param.setFocusMode(Camera.Parameters.FOCUS_MODE_AUTO);
		
		param.setPictureSize(camera_res.x, camera_res.y);
		param.setPreviewSize(camera_res.x, camera_res.y);

		param.setJpegQuality(100);
		param.set("vrmode", 1);
		param.setRecordingHint(true);
		mCamera.setParameters(param);

		try {
			param.set("fast-fps-mode", 2);
			param.setPreviewFpsRange(120000, 120000);
			param.set("no-display-mode", "true");
			mCamera.setParameters(param);
		} catch (Exception e){
			Log.e("camera", e.getMessage());
		}
    }
    
    /** The auto focus callback. */
    AutoFocusCallback myAutoFocusCallback = new AutoFocusCallback(){
        @Override
        public void onAutoFocus(boolean arg0, Camera arg1) {
            if (arg0){mCamera.cancelAutoFocus();}
        }};
}
