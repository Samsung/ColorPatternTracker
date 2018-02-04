package com.samsung.dtl.colorpatterntracker;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL10;

import org.opencv.core.CvType;
import org.opencv.core.Mat;

import android.app.Activity;
import android.content.pm.PackageManager;
import android.graphics.Point;
import android.graphics.SurfaceTexture;
import android.opengl.GLES20;
import android.opengl.GLES31;
import android.opengl.GLSurfaceView;
import android.os.Environment;
import android.util.Log;

import com.samsung.dtl.bluetoothlibrary.profile.BTReceiverClass6;
import com.samsung.dtl.colorpatterntracker.camera.CameraManager;
import com.samsung.dtl.colorpatterntracker.camera.ShaderManager;
import com.samsung.dtl.bluetoothlibrary.profile.BtPosition6f;

import java.io.FileNotFoundException;
import java.io.IOException;
import java.nio.ByteBuffer;
import java.nio.ByteOrder;


// TODO: Auto-generated Javadoc
/**
 * The Class MyGLRenderer.
 */
public class CustomGLRenderer implements GLSurfaceView.Renderer{

	// tracker
	public ColorGridTracker mCgTrack; /*!< The tracker instance. */
	
	// bluetooth
	private BtPosition6f mPositionComm = null; /*!< The bluetooth class that communicates the position of pattern's origin. */
	public BTReceiverClass6 mPositionRecv = null;
	public boolean data_sent=false; /*!< is data_sent. */
	public boolean sendBT = false; /*!< allow data to be sent via bluetooth. */
	public boolean recvBT = true;
	public int count_BTSend = -1;
	public int wait_time=7;
	
	// timing
	public long processedCaptureTime; /*!< The time when last frame's capture is processed. */
	
	// fps
	private int frames = 0; /*!< The number of frames in the second. */	
	private long lastFPScomputeTime = System.nanoTime(); /*!< The time since last FPS was computed. */	
	
	// camera
	CameraManager mCameraManager; /*!< The camera manager. */
	private SurfaceTexture mSTexture; /*!< The camera surface texture. */
	private Point camera_res;	/*!< The camera resolution. */
	
	// graphics
	ShaderManager mShaderManager; /*!< The shader manager. */
	private Point display_dim;	/*!< The dimensions of the Android display. OpenGL viewport should be set to this. */ 
	
	EGL10 mEgl; /*!< The egl instance. */
	EGLDisplay mEglDisplay; /*!< The egl display instance. */
	EGLSurface mEglSurface; /*!< The egl surface instance. */
	EGLConfig mEglConfig; /*!< The egl config instance. */
	EGLContext mEglContext; /*!< The egl context instance. */
	
	// logging
	private final String TAG = "cgt"; /*!< The tag. */	
	int nFBOs = 0;
    
	/**
	 * Instantiates a new glRenderer.
	 *
	 * @param view the view
	 */
	CustomGLRenderer (CustomGLSurfaceView view) {
		nFBOs = 1;
		count_BTSend = -1;
		data_sent = false;

		wait_time = 7;
		mCameraManager = new CameraManager();
		mShaderManager = new ShaderManager();
		camera_res = new Point(1920, 1080);
		
		processedCaptureTime=0;
		
		mCgTrack = new ColorGridTracker();
		
		mShaderManager.initializeCoords();


		mPositionComm = null;
		mPositionRecv =null;
	}

	/* (non-Javadoc)
	 * @see android.opengl.GLSurfaceView.Renderer#onSurfaceCreated(javax.microedition.khronos.opengles.GL10, javax.microedition.khronos.egl.EGLConfig)
	 */
	public void onSurfaceCreated (GL10 unused, EGLConfig config) {        
		mSTexture = mShaderManager.initTex(camera_res, nFBOs);
		mCameraManager.initializeCamera(camera_res, mSTexture);
		//ColorGridTracker.initCL(camera_res.x, camera_res.y, mShaderManager.glTextures[0], mShaderManager.glTextures[1]);
	}
	
	/* (non-Javadoc)
	 * @see android.opengl.GLSurfaceView.Renderer#onSurfaceChanged(javax.microedition.khronos.opengles.GL10, int, int)
	 */
	public void onSurfaceChanged ( GL10 unused, int width, int height ) {
		GLES20.glViewport( 0, 0, width, height);
		mCameraManager.mCamera.startPreview();
	}

	/**
	 * Capture frame.
	 *
	 * @return the long
	 */
	public long captureFrame(){
		long captureTime=0;
		boolean saveFiles = false;
		if(count_BTSend==nFBOs)saveFiles=true;
		if(count_BTSend!=-1 && count_BTSend!=0) {
			captureTime = mShaderManager.cameraToTexture(mSTexture, camera_res, count_BTSend-1, nFBOs, saveFiles);
		}
		//if(processedCaptureTime == captureTime || captureTime==0)return 0;
		//processedCaptureTime = captureTime;
		mCameraManager.frameNo++;
		measureFPS();		
		return captureTime;
	}
	
	/* (non-Javadoc)
	 * @see android.opengl.GLSurfaceView.Renderer#onDrawFrame(javax.microedition.khronos.opengles.GL10)
	 */
	public void onDrawFrame ( GL10 unused ) {
		// capture
		//long captureTime = captureFrame();
		//if(captureTime==0)return;
		
		// track
		Mat origin;
		//origin = mCgTrack.trackGrid(camera_res.y, camera_res.x,frameIdForTracker(), captureTime);
		{
			origin = Mat.zeros(0, 0, CvType.CV_32FC1);



			if(mPositionComm!=null || mPositionRecv!=null) {
				if (sendBT) {
					if (count_BTSend != -1) {
						mPositionComm.sendData(count_BTSend, count_BTSend, count_BTSend, count_BTSend, count_BTSend, count_BTSend);
						count_BTSend++;
						try {
							Thread.sleep(wait_time,0);
						} catch (InterruptedException e) {
							e.printStackTrace();
						}
					}
				} else {
					//if(mPositionRecv.data_dirty)
					{
						Log.e("cgt", "waiting");
						while (!mPositionRecv.data_dirty) {
							int temp_int = 0;
							try {
								Thread.sleep(0, 500000);
							} catch (InterruptedException e) {
								e.printStackTrace();
							}
						}
						Log.e("cgt", "received");
						mPositionRecv.data_dirty = false;
						count_BTSend++;
					}
				}
			}
			long captureTime = captureFrame();
			/*
			if (!mCameraManager.allowExposureUpdate) {
				// send
				mPositionComm.sendData(count_BTSend,count_BTSend,count_BTSend,count_BTSend,count_BTSend,count_BTSend);
			}else{
				mPositionRecv.data_dirty=false;
			}
			*/
		}
		// send
		if(origin.rows()!=0 && sendBT){
			mPositionComm.sendData((float)origin.get(0,0)[0],(float)origin.get(1,0)[0],(float)origin.get(2,0)[0],(float)origin.get(3,0)[0],(float)origin.get(4,0)[0],(float)origin.get(5,0)[0]);
			Log.e("Track", "x:"+(float)origin.get(0,0)[0]+" y:"+(float)origin.get(1,0)[0]+" z:"+(float)origin.get(2,0)[0]);
		}
		
		// update
		if(mCameraManager.allowWBUpdate && origin.rows()!=0){
			//getColorPixelDiff();
		}
		//mCameraManager.updateCameraParams(mCgTrack,origin, camera_res);
		
		// debug
		if(mCgTrack.mDebugLevel==1){
			mShaderManager.renderFromTexture(mShaderManager.glTextures_in[(mCameraManager.frameNo-1+nFBOs)%nFBOs], display_dim);
		}else{
			mShaderManager.renderFromTexture(mShaderManager.glTextures_in[(mCameraManager.frameNo-1+nFBOs)%nFBOs], display_dim);
		}
		if(sendBT) {
			if (count_BTSend == nFBOs) count_BTSend = -1;
		}else{
			if (count_BTSend == nFBOs) count_BTSend = 0;
		}
	}

	public int byteToUnsignedInt(byte b) {
		return 0x00 << 24 | b & 0xff;
	}
	
	/**
	 * modifies the Frame id for opencl tracker for debugging.
	 *
	 * @return the int
	 */
	private int frameIdForTracker(){
		if(mCgTrack.mDebugLevel==0)return -1;
		return  mCameraManager.frameNo;
	}

	/**
	 * Closes camera.
	 */
	public void close() {
		Log.e("camera", "stopping");
		mSTexture.release();
		mCameraManager.mCamera.stopPreview();
		mCameraManager.mCamera.release();
		ColorGridTracker.destroyCL();
		mShaderManager.deleteTex();
	}

	/**
	 * Measure frames per second.
	 */
	public void measureFPS() {
		frames++;
		if(System.nanoTime() - lastFPScomputeTime >= 1000000000) {
			Log.d(TAG, "FPS: " + frames);
			frames = 0;
			lastFPScomputeTime = System.nanoTime();
		}
	}
	
	/**
	 * Sets the display dimension.
	 *
	 * @param displayDim the new display dimension
	 */
	public void setDisplayDim(Point displayDim) {
		display_dim = displayDim;
	}
	
	/**
	 * Initializes the bluetooth.
	 *
	 * @param activity the current activity
	 */
	public void initBT(Activity activity){
		if(sendBT){
			count_BTSend=-1;
	        mPositionComm = BtPosition6f.getInstance(activity);
	        mPositionComm.start();
		}
		if(recvBT){
			count_BTSend=0;
			mPositionRecv = BTReceiverClass6.newInstance(activity);
		}
	}
	
    /**
     * On destroy.
     */
    public void onDestroy() {
    	if(sendBT){
    		mPositionComm.stop();
    	}
    	if(recvBT){
    		mPositionRecv.onDestroy();
		}
    	ColorGridTracker.destroyCL();
    }
}