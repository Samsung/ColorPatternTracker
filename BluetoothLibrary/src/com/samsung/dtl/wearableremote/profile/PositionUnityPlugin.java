package com.samsung.dtl.wearableremote.profile;

import android.app.Activity;

public class PositionUnityPlugin implements BtPosition.Listener {

    private final Activity mActivity;
    private BtPosition mPosition;
    
    private PositionUnityPlugin(Activity activity) {
        mActivity = activity;
        mActivity.runOnUiThread(new Runnable() {
    		@Override
    		public void run() {
    			mPosition = BtPosition.getInstance(mActivity);
    			mPosition.setListener(PositionUnityPlugin.this);
    			mPosition.start();
    		}
        });
    }
    
    public void destroy() {
        mActivity.runOnUiThread(new Runnable() {
            @Override
            public void run() {
                if (null != mPosition) {
            		mPosition.setListener(null);
            		mPosition.stop();
            		mPosition = null;
                }
            }
        });
    }    
    
    public static PositionUnityPlugin newInstance(Activity activity) {
        return new PositionUnityPlugin(activity);
    }

    private float mX, mY, mZ;
    
	@Override
	public boolean onPositionReceived(float x, float y, float z) {
		mX = x;
		mY = y;
		mZ = z;
		return true;
	}
	
	public float getX() {
		return mX;
	}

	public float getY() {
		return mY;
	}
	
	public float getZ() {
		return mZ;
	}
	
}
