package com.samsung.dtl.wearableremote.profile;

import android.app.Activity;


import com.samsung.dtl.wearableremote.bluetooth.BtConnectedSock;
import com.samsung.dtl.wearableremote.bluetooth.BtService;
import com.samsung.dtl.wearableremote.profile.BtPosition;

public class BTReceiverClass6 implements BtPosition.Listener {

    private BtPosition mPosition; // receiver
    public float mx,my,mz;
    
    public float getX(){
    	return mx;
    }
    public float getY(){
    	return my;
    }
    public float getZ(){
    	return mz;
    }
    public BTReceiverClass6(Activity activity) {
        super();
        initBT(activity);
        mx=0;
        my=0;
        mz=0;
    }
    
    public static BTReceiverClass6 newInstance(Activity activity) {
    	return new BTReceiverClass6(activity); 
    }
        
    public void initBT(Activity activity){
    	mPosition = BtPosition.getInstance(activity);
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
	public boolean onPositionReceived(float x, float y, float z) {
		// TODO Auto-generated method stub
		mx=x;
		my=y;
		mz=z;
		return false;
	}
}
