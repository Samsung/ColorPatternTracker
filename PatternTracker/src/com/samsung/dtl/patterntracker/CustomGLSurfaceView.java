package com.samsung.dtl.patterntracker;

import android.content.Context;
import android.graphics.Point;
import android.opengl.GLSurfaceView;
import android.util.AttributeSet;
import android.view.SurfaceHolder;

// TODO: Auto-generated Javadoc
/**
 * The Class MyGLSurfaceView.
 */
public class CustomGLSurfaceView extends GLSurfaceView {

	/** The renderer. */
	public final CustomGLRenderer mRenderer;

	/**
	 * Instantiates a new gl surface view.
	 *
	 * @param context the current context
	 * @param attrs the attributes
	 */
	public CustomGLSurfaceView(Context context, AttributeSet attrs) {
	   super(context, attrs);
		mRenderer = new CustomGLRenderer(this);
		setEGLContextClientVersion (2);
		setRenderer(mRenderer);
		setRenderMode(GLSurfaceView.RENDERMODE_CONTINUOUSLY);
	}

	/* (non-Javadoc)
	 * @see android.opengl.GLSurfaceView#surfaceCreated(android.view.SurfaceHolder)
	 */
	public void surfaceCreated ( SurfaceHolder holder ) {
		super.surfaceCreated ( holder );
	}

	/* (non-Javadoc)
	 * @see android.opengl.GLSurfaceView#surfaceDestroyed(android.view.SurfaceHolder)
	 */
	public void surfaceDestroyed ( SurfaceHolder holder ) {
		mRenderer.close();
		super.surfaceDestroyed ( holder );
	}

	/* (non-Javadoc)
	 * @see android.opengl.GLSurfaceView#surfaceChanged(android.view.SurfaceHolder, int, int, int)
	 */
	public void surfaceChanged ( SurfaceHolder holder, int format, int w, int h ) {
		super.surfaceChanged ( holder, format, w, h );
	}

	/**
	 * Sets the display dimension.
	 *
	 * @param displayDim the new display dimension
	 */
	public void setDisplayDim(Point displayDim) {
		mRenderer.setDisplayDim(displayDim);
	}
}
