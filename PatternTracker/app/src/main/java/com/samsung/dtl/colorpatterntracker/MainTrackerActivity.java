package com.samsung.dtl.colorpatterntracker;

import com.samsung.dtl.colorpatterntracker.R; // don't remove this import, otherwise it breaks something
import com.samsung.dtl.colorpatterntracker.util.HandlerExtension;

import android.app.Activity;
import android.content.Context;
import android.content.pm.PackageManager;
import android.graphics.Color;
import android.graphics.Point;

import android.os.Build;
import android.os.Bundle;
import android.os.Handler;
import android.os.PowerManager;
import android.os.PowerManager.WakeLock;
import android.util.Log;
import android.view.Display;
import android.view.View;
import android.widget.Button;
import android.widget.RadioButton;
import android.widget.SeekBar;
import android.widget.TextView;

import javax.microedition.khronos.opengles.GL10;

/**
 * The Class MainTrackerActivity.
 */
public class MainTrackerActivity extends Activity {
	private CustomGLSurfaceView mView; /*!< The view. */
	
	private WakeLock mWL; /*!< The wake lock. */
	
    Button button_debug; /*!< The debug button. */
    
    Button button_updateGeom; /*!< The update geometry button. */

	Button button_wb; /*!< The update white balance button. */

	Button button_exposure; /*!< The update exposure button. */

	Button button_focus; /*!< The update focus button. */

	SeekBar seekbar_exposure;

	SeekBar seekbar_focus;
    
    Handler detectedPatternTextHandler; /*!< The text handler to show learned patterns. */
        
	/* (non-Javadoc)
	 * @see android.app.Activity#onCreate(android.os.Bundle)
	 */
	@Override
	public void onCreate(Bundle savedInstanceState) {

		if(Build.VERSION.SDK_INT >= Build.VERSION_CODES.M){
			int hasReadPermission = checkSelfPermission(android.Manifest.permission.CAMERA);
			if (hasReadPermission != PackageManager.PERMISSION_GRANTED) {
				requestPermissions(new String[]{android.Manifest.permission.CAMERA},10);
			}

			hasReadPermission = checkSelfPermission(android.Manifest.permission.WRITE_EXTERNAL_STORAGE);
			if (hasReadPermission != PackageManager.PERMISSION_GRANTED) {
				requestPermissions(new String[]{android.Manifest.permission.WRITE_EXTERNAL_STORAGE},10);
			}
			hasReadPermission = checkSelfPermission(android.Manifest.permission.READ_EXTERNAL_STORAGE);
			if (hasReadPermission != PackageManager.PERMISSION_GRANTED) {
				requestPermissions(new String[]{android.Manifest.permission.READ_EXTERNAL_STORAGE},
						10);
			}
		}
		super.onCreate(savedInstanceState);
		setContentView(R.layout.activity_main);
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
		mView.mRenderer.mShaderManager.context = getApplicationContext();
		
		// debug
		setDebugButton();
	    
	    // update geometry
	    setUpdateGeometryButton();

	    // update White Balance
		setModifyWBButton();

		// update exposure
		setModifyExposureButton();

		setModifyFocusButton();
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
	 * Sets the White balance button.
	 */
	private void setModifyWBButton(){
		button_wb = (Button) findViewById(R.id.button_wb);
		button_wb.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				if(mView.mRenderer.mCameraManager.allowWBUpdate){
					//mView.mRenderer.mCameraManager.allowWBUpdate=false;
					//button_wb.setText("Mod WB: Off");
				}else{
					mView.mRenderer.mCameraManager.allowWBUpdate=true;
					button_wb.setText("Mod WB");
				}
			}
		});
		mView.mRenderer.mCameraManager.button_wb = button_wb;
	}

	private void setModifyExposureButton(){
		button_exposure = (Button) findViewById(R.id.button_exposure);
		button_exposure.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				if(mView.mRenderer.mCameraManager.allowExposureUpdate){
					mView.mRenderer.mCameraManager.allowExposureUpdate=false;
					button_exposure.setText("Mod Exp: Off");
				}else{
					mView.mRenderer.mCameraManager.allowExposureUpdate=true;
					button_exposure.setText("Mod Exp: On");
				}
			}
		});

		seekbar_exposure = (SeekBar) findViewById(R.id.seekbar_exposure);
		seekbar_exposure.setOnSeekBarChangeListener(new SeekBar.OnSeekBarChangeListener() {
			@Override
			public void onStopTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
			}

			@Override
			public void onStartTrackingTouch(SeekBar seekBar) {
				// TODO Auto-generated method stub
			}

			@Override
			public void onProgressChanged(SeekBar seekBar, int progress,boolean fromUser) {
				Log.e("cam","progress changed to "+progress);
				// TODO Auto-generated method stub
				if(mView.mRenderer.mCameraManager.allowExposureUpdate){
					mView.mRenderer.mCameraManager.mExposureSeekbarProgress = progress;
				}else{
					if(seekbar_exposure.getProgress() != mView.mRenderer.mCameraManager.mExposureSeekbarProgress ){
						seekbar_exposure.setProgress(mView.mRenderer.mCameraManager.mExposureSeekbarProgress );
					}
				}
			}
		});
	}

	private void setModifyFocusButton(){
		button_focus = (Button) findViewById(R.id.button_focus);
		button_focus.setOnClickListener(new View.OnClickListener() {
			public void onClick(View v) {
				if(mView.mRenderer.mCameraManager.allowFocusUpdate){
					mView.mRenderer.mCameraManager.allowFocusUpdate=false;

					button_focus.setText("Mod Focus: Off");
				}else{
					mView.mRenderer.mCameraManager.allowFocusUpdate=true;
					button_focus.setText("Mod Focus: On");
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