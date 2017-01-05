package com.samsung.dtl.wearableremote.profile;

import android.app.Activity;


import com.samsung.dtl.wearableremote.bluetooth.BtConnectedSock;
import com.samsung.dtl.wearableremote.bluetooth.BtService;
import com.samsung.dtl.wearableremote.profile.BtPosition6f;

public class BTReceiverClass implements BtPosition6f.Listener {

    private BtPosition6f mPosition; // receiver
    public float mx,my,mz,mYaw,mPitch,mRoll;
    
    public float getX(){
    	return mx;
    }
    public float getY(){
    	return my;
    }
    public float getZ(){
    	return mz;
    }
    public float getYaw(){
    	return mYaw;
    }
    public float getPitch(){
    	return mPitch;
    }
    public float getRoll(){
    	return mRoll;
    }
    public BTReceiverClass(Activity activity) {
        super();
        initBT(activity);
        mx=0;
        my=0;
        mz=0;
        mYaw=0;
        mPitch=0;
        mRoll=0;
    }
    
    public static BTReceiverClass newInstance(Activity activity) {
    	return new BTReceiverClass(activity); 
    }
        
    public void initBT(Activity activity){
    	mPosition = BtPosition6f.getInstance(activity);
    	mPosition.getService().registerObserver(mConnectionCountUpdater);
    	mPosition.start();
    	mPosition.setListener(this);
    }
    
    public void onDestroy() {
    	mPosition.getService().unregisterObserver(mConnectionCountUpdater);
    	mPosition.setListener(null);
    	mPosition.stop();   
    }
    
    private BtService.Observer mConnectionCountUpdater = new BtService.Observer() {
        
        @Override
        public void onDisconnected(BtConnectedSock sock) {
        }
        
        @Override
        public void onConnected(BtConnectedSock sock) {
        }
    };

	@Override
	public boolean onPositionReceived(float x, float y, float z, float yaw, float pitch, float roll) {
		// TODO Auto-generated method stub
		mx=x;
		my=y;
		mz=z;
		mYaw=yaw;
		mPitch=pitch;
		mRoll=roll;
		return false;
	}
}
