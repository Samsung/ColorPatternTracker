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
import android.graphics.Point;
import android.graphics.SurfaceTexture;
import android.opengl.GLES20;
import android.opengl.GLSurfaceView;
import android.util.Log;

import com.samsung.dtl.colorpatterntracker.camera.CameraManager;
import com.samsung.dtl.colorpatterntracker.camera.ShaderManager;
import com.samsung.dtl.bluetoothlibrary.profile.BtPosition6f;


// TODO: Auto-generated Javadoc
/**
 * The Class MyGLRenderer.
 */
public class CustomGLRenderer implements GLSurfaceView.Renderer{

	// tracker
	public ColorGridTracker mCgTrack; /*!< The tracker instance. */
	
	// bluetooth
	private BtPosition6f mPositionComm; /*!< The bluetooth class that communicates the position of pattern's origin. */
	public boolean data_sent=false; /*!< is data_sent. */
	private final boolean sendBT = true; /*!< allow data to be sent via bluetooth. */
	
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
		mCameraManager = new CameraManager();
		mShaderManager = new ShaderManager();
		camera_res = new Point(1920, 1080);
		
		processedCaptureTime=0;
		
		mCgTrack = new ColorGridTracker();
		
		mShaderManager.initializeCoords();
		
		data_sent = false;
	}

	/* (non-Javadoc)
	 * @see android.opengl.GLSurfaceView.Renderer#onSurfaceCreated(javax.microedition.khronos.opengles.GL10, javax.microedition.khronos.egl.EGLConfig)
	 */
	public void onSurfaceCreated (GL10 unused, EGLConfig config) {        
		mSTexture = mShaderManager.initTex(camera_res, nFBOs);
		mCameraManager.initializeCamera(camera_res, mSTexture);
		//ColorGridTracker.initCL(camera_res.x, camera_res.y, mShaderManager.glTextures_in[0], mShaderManager.glTextures_out[0]);
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

		// logic for saving certain frame buffers
		boolean saveFiles = false;
		saveFiles=(mCameraManager.frameNo%nFBOs==nFBOs-1);
		if(mCameraManager.frameNo<5000000) saveFiles=false;
		if(mCgTrack.mDebugLevel==0 && mCameraManager.frameNo%nFBOs==(nFBOs-1))saveFiles=true;

		long captureTime = mShaderManager.cameraToTexture(mSTexture, camera_res,mCameraManager.frameNo%nFBOs, nFBOs,saveFiles);

		if(processedCaptureTime == captureTime || captureTime==0)return 0;
		processedCaptureTime = captureTime;
		mCameraManager.frameNo++;
		measureFPS();		
		return captureTime;
	}
	
	/* (non-Javadoc)
	 * @see android.opengl.GLSurfaceView.Renderer#onDrawFrame(javax.microedition.khronos.opengles.GL10)
	 */
	public void onDrawFrame ( GL10 unused ) {
		// capture
		long captureTime = captureFrame();
		if(captureTime==0)return;
		
		// track
		Mat origin = Mat.zeros(0, 0, CvType.CV_32FC1);//mCgTrack.trackGrid(camera_res.y, camera_res.x,frameIdForTracker(), captureTime);

		// send
		if(origin.rows()!=0 && sendBT){
			mPositionComm.sendData((float)origin.get(0,0)[0],(float)origin.get(1,0)[0],(float)origin.get(2,0)[0],(float)origin.get(3,0)[0],(float)origin.get(4,0)[0],(float)origin.get(5,0)[0]);
			Log.e("Track", "x:"+(float)origin.get(0,0)[0]+" y:"+(float)origin.get(1,0)[0]+" z:"+(float)origin.get(2,0)[0]);
		}
		
		// update
		mCameraManager.updateCameraParams(mCgTrack,origin, camera_res);
		
		// debug
		if(mCgTrack.mDebugLevel==1)mShaderManager.renderFromTexture(mShaderManager.glTextures_in[(mCameraManager.frameNo-1+nFBOs)%nFBOs], display_dim);
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
	        mPositionComm = BtPosition6f.getInstance(activity);
	        mPositionComm.start();
		}
	}
	
    /**
     * On destroy.
     */
    public void onDestroy() {
    	if(sendBT){
    		mPositionComm.stop();
    	}
    	ColorGridTracker.destroyCL();
    }
}