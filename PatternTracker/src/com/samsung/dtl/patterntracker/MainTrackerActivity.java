package com.samsung.dtl.patterntracker;

import com.samsung.dtl.patterntracker.R;
import com.samsung.dtl.patterntracker.util.HandlerExtension;

import android.app.Activity;
import android.content.Context;
import android.graphics.Color;
import android.graphics.Point;
import android.os.Bundle;
import android.os.Handler;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.view.Display;
import android.view.View;
import android.widget.Button;
import android.widget.TextView;

// TODO: Auto-generated Javadoc
/**
 * The Class MainTrackerActivity.
 */
public class MainTrackerActivity extends Activity {
	
	private CustomGLSurfaceView mView; /*!< The view. */
	
	private WakeLock mWL; /*!< The wake lock. */
	
    Button button_debug; /*!< The debug button. */
    
    Button button_updateGeom; /*!< The update geometry button. */
    
    Handler detectedPatternTextHandler; /*!< The text handler to show learned patterns. */
        
	/* (non-Javadoc)
	 * @see android.app.Activity#onCreate(android.os.Bundle)
	 */
	@Override
	public void onCreate(Bundle savedInstanceState) {
		super.onCreate(savedInstanceState);
		setContentView(R.layout.main);
		detectedPatternTextHandler = new HandlerExtension(this);

		// avoid sleep mode
		//mWL = ((PowerManager)getSystemService (Context.POWER_SERVICE)).newWakeLock(PowerManager.PARTIAL_WAKE_LOCK, "WakeLock");
		mWL = ((PowerManager)getSystemService (Context.POWER_SERVICE)).newWakeLock(PowerManager.FULL_WAKE_LOCK, "WakeLock");
		mWL.acquire();
		
		// get display size
		Display display = getWindowManager().getDefaultDisplay();
		Point displayDim = new Point();
		display.getSize(displayDim);
		
		// view
		mView = (CustomGLSurfaceView) findViewById(R.id.surfaceviewclass);
		mView.setDisplayDim(displayDim);
				
		// bluetooth
		mView.mRenderer.initBT(this); 
		
		mView.mRenderer.mCgTrack.mTextDetectedPatterns = (TextView) findViewById(R.id.text_detectedPatterns);
		mView.mRenderer.mCgTrack.mTextDetectedPatterns.setTextColor(Color.rgb(0, 0, 0));
		mView.mRenderer.mCgTrack.textViewHandler = (HandlerExtension) detectedPatternTextHandler;
		
		// debug
		setDebugButton();
	    
	    // update geometry
	    setUpdateGeometryButton();
	}
	
	/**
	 * Sets the debug button.
	 */
	private void setDebugButton(){
	    button_debug = (Button) findViewById(R.id.button_debug);
	    button_debug.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
            	if(mView.mRenderer.mCgTrack.mDebugLevel==0){           		
            		mView.mRenderer.mCgTrack.mDebugLevel=1;
            		button_debug.setText("Debug: On");
            	}else{
            		button_debug.setText("Debug: Off");
            		mView.mRenderer.mCgTrack.mDebugLevel=0;
            	}
            }
        });		
	}
	
	/**
	 * Sets the update geometry button.
	 */
	private void setUpdateGeometryButton(){
	    button_updateGeom = (Button) findViewById(R.id.button_updateGeom);
	    button_updateGeom.setOnClickListener(new View.OnClickListener() {
            public void onClick(View v) {
            	if(mView.mRenderer.mCgTrack.mUpdateRelTransf){           		
            		mView.mRenderer.mCgTrack.mUpdateRelTransf=false;
            		button_updateGeom.setText("Update Geom: Off");
            	}else{
            		mView.mRenderer.mCgTrack.mUpdateRelTransf=true;
            		button_updateGeom.setText("Update Geom: On");
            	}
            }
        });		
	}
	
	/**
	 * Update text.
	 *
	 * @param str the text to be displayed
	 */
	public void updateText(String str){
		mView.mRenderer.mCgTrack.mTextDetectedPatterns.setText(str);
	}
    
	/* (non-Javadoc)
	 * @see android.app.Activity#onPause()
	 */
	@Override
	protected void onPause() {
		if ( mWL.isHeld() )	mWL.release();
		mView.onPause();
		super.onPause();
	}
			
	/* (non-Javadoc)
	 * @see android.app.Activity#onResume()
	 */
	@Override
	protected void onResume() {
		super.onResume();
		mView.onResume();
		mWL.acquire();
	}
	
    /* (non-Javadoc)
     * @see android.app.Activity#onDestroy()
     */
    @Override
    public void onDestroy() {
        mView.mRenderer.onDestroy();
        super.onDestroy();
    }
}