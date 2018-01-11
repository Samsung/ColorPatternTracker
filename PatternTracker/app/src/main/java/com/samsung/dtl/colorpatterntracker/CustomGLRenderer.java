package com.samsung.dtl.colorpatterntracker;

import javax.microedition.khronos.egl.EGL10;
import javax.microedition.khronos.egl.EGLConfig;
import javax.microedition.khronos.egl.EGLContext;
import javax.microedition.khronos.egl.EGLDisplay;
import javax.microedition.khronos.egl.EGLSurface;
import javax.microedition.khronos.opengles.GL10;

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
    
	/**
	 * Instantiates a new glRenderer.
	 *
	 * @param view the view
	 */
	CustomGLRenderer (CustomGLSurfaceView view) {
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
		mSTexture = mShaderManager.initTex(camera_res);
		mCameraManager.initializeCamera(camera_res, mSTexture);
		ColorGridTracker.initCL(camera_res.x, camera_res.y, mShaderManager.glTextures[0], mShaderManager.glTextures[1]);
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
		long captureTime = mShaderManager.cameraToTexture(mSTexture, camera_res);
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
		Mat origin = mCgTrack.trackGrid(camera_res.y, camera_res.x,frameIdForTracker(), captureTime);

		// send
		if(origin.rows()!=0 && sendBT){
			mPositionComm.sendData((float)origin.get(0,0)[0],(float)origin.get(1,0)[0],(float)origin.get(2,0)[0],(float)origin.get(3,0)[0],(float)origin.get(4,0)[0],(float)origin.get(5,0)[0]);
			Log.e("Track", "x:"+(float)origin.get(0,0)[0]+" y:"+(float)origin.get(1,0)[0]+" z:"+(float)origin.get(2,0)[0]);
		}
		
		// update
		if(mCameraManager.allowWBUpdate && origin.rows()!=0){
			getColorPixelDiff();
		}
		mCameraManager.updateCameraParams(mCgTrack,origin, camera_res);
		
		// debug
		if(mCgTrack.mDebugLevel==1){
			mShaderManager.renderFromTexture(mShaderManager.glTextures[1], display_dim);
		}else{
			mShaderManager.renderFromTexture(mShaderManager.glTextures[0], display_dim);
		}
	}

	private int getColorPixelDiff(){
		GLES31.glBindFramebuffer(GLES31.GL_FRAMEBUFFER, mShaderManager.targetFramebuffer.get(0));

		ByteBuffer byteBuffer1 = ByteBuffer.allocate(4);
		byteBuffer1.order(ByteOrder.nativeOrder());

		int[][] order={{0,1,4,3},{1,2,5,4},{4,5,8,7},{3,4,7,6}};

		double[] x = new double[4];
		double[] y = new double[4];

		int [][] colors = new int[4][3];
		int count_valid=0;
		for(int i=0;i<4;i++) {
			x[i] = 0;
			y[i]=0;
			for(int j=0;j<4;j++) {
				x[i] += mCgTrack.mCorners.get(order[i][j], 0)[0];
				y[i] += mCgTrack.mCorners.get(order[i][j], 1)[0];
			}
			x[i] /=4;
			y[i] /=4;

			int xi,yi;
			xi = (int)Math.round(x[i]);
			yi = (int)Math.round(y[i]);

			if(xi>=0 && xi<1080 && yi>=0 && yi<1920) {
				GLES31.glReadPixels(yi, xi, 1, 1, GLES31.GL_RGBA, GLES31.GL_UNSIGNED_BYTE, byteBuffer1);
				byte[] array = byteBuffer1.array();
				//Log.e("cgt","DiffVals camPix:"+i+"["+byteToUnsignedInt(array[0])+","+byteToUnsignedInt(array[1])+","+byteToUnsignedInt(array[2])+","+byteToUnsignedInt(array[3])+"]"+xi+","+yi);
				colors[i][0] =  byteToUnsignedInt(array[0]);
				colors[i][1] =  byteToUnsignedInt(array[1]);
				colors[i][2] =  byteToUnsignedInt(array[2]);
				count_valid++;
			}
		}

		int colDiff=0;
		if(count_valid==4){
			colDiff += 255-colors[0][0]+colors[0][1]+colors[0][2];
			colDiff += colors[1][0]+255-colors[1][1]+colors[1][2];
			colDiff += colors[2][0]+colors[2][1]+255-colors[2][2];
			colDiff += colors[3][0]+255-colors[3][1]+colors[3][2];
			mCameraManager.colorPixelDiff = colDiff;
		}else{
			mCameraManager.colorPixelDiff = -1;
		}


		/*
		{
			ByteBuffer byteBuffer = ByteBuffer.allocate(1920 * 1080 * 4);
			byteBuffer.order(ByteOrder.nativeOrder());

			GLES31.glReadPixels(0, 0, 1920, 1080, GLES31.GL_RGBA, GLES31.GL_UNSIGNED_BYTE, byteBuffer);
			int error14 = GLES31.glGetError();
			byte[] array = byteBuffer.array();

			java.io.FileOutputStream outputStream = null;
			try {
				String diskstate = Environment.getExternalStorageState();
				if(diskstate.equals("mounted")){
					java.io.File picFolder = Environment.getExternalStoragePublicDirectory(Environment.DIRECTORY_PICTURES);
					java.io.File picFile = new java.io.File(picFolder,"imgc.bin");
					outputStream = new java.io.FileOutputStream(picFile);
				}
			} catch (FileNotFoundException e) {
				e.printStackTrace();
			}

			try {
				outputStream.write(array);
			} catch (IOException e) {
				e.printStackTrace();
			}

			try {
				outputStream.close();
			} catch (IOException e) {
				e.printStackTrace();
			}
		}
*/
		return 0;
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