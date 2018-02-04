package com.samsung.dtl.bluetoothlibrary.profile;

import android.app.Activity;


import com.samsung.dtl.bluetoothlibrary.bluetooth.BtConnectedSock;
import com.samsung.dtl.bluetoothlibrary.bluetooth.BtService;
import com.samsung.dtl.bluetoothlibrary.profile.BtPosition;

public class BTReceiverClass6 implements BtPosition6f.Listener {

    private BtPosition6f mPosition; // receiver
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
    public boolean data_dirty;
    public BTReceiverClass6(Activity activity) {
        super();
        initBT(activity);
        data_dirty = false;
        mx=0;
        my=0;
        mz=0;
    }
    
    public static BTReceiverClass6 newInstance(Activity activity) {
    	return new BTReceiverClass6(activity); 
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
        data_dirty = true;
		return false;
	}
}
