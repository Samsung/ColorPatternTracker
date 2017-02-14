package com.samsung.dtl.bluetoothlibrary.profile;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.UUID;

import android.content.Context;
import android.util.Log;

import com.samsung.dtl.bluetoothlibrary.bluetooth.BtService;
import com.samsung.dtl.bluetoothlibrary.bluetooth.BtUtil;
import com.samsung.dtl.bluetoothlibrary.bluetooth.Point3F;

public class BtPosition extends BtProfileHelper implements BtService.Observer {
    
    private static final String LOG_TAG = BtUtil.getLogTag(BtPosition.class);

    public static BtPosition sInstance = null;
    
    public static BtPosition getInstance(Context context) {
        if (null == sInstance) {
            Context appContext = context.getApplicationContext();
            sInstance = new BtPosition(appContext); 
        }
        return sInstance;
    }
    
    public interface Listener {
        public boolean onPositionReceived(float x, float y, float z);
    }
       
    private BtPosition(Context context) {
        //super(context, "Remote_Position_" + BtUtil.getHashCode(BtPosition.class), UUID.fromString("21864823-7766-4993-8881-180bc37d0f81"), true);
    	super(context, "Remote_Position_" + BtUtil.getHashCode(BtPosition.class), UUID.fromString("0000110a-0000-1000-8000-00805f9b34fb"), true);
     }

    private Listener mListener;
    
    public void setListener(Listener listener) {
        mListener = listener;
    }

    private class PositionParcel extends TransportParcel<Point3F> {

        public PositionParcel(int msgType) {
            super(msgType);
        }
        
        private PositionParcel set(float x, float y, float z) {
            setData(new Point3F(x, y, z));
            return this;
        }
        
        @Override
        protected void read(DataInputStream is) throws IOException {
            float x = is.readFloat();
            float y = is.readFloat();
            float z = is.readFloat();
            set(x, y, z);
        }
        
        @Override
        protected void write(DataOutputStream os) throws IOException {
            os.writeFloat(mData.x);
            os.writeFloat(mData.y);
            os.writeFloat(mData.z);
            os.flush();

            if (sDebug) {
                //Log.d(LOG_TAG, "Writing " + mData);
            }
        }
        
    }
    
    public boolean sendData(float x, float y, float z) {
        PositionParcel parcel = PositionParcel.class.cast(newParcel(0));
        parcel.set(x, y, z);
        dispatchParcel(parcel);
        return true;
    }
    
    public boolean receiveData() {
        return true;
    }
    
    
    @Override
    protected void onIncoming(ConnectionData connData, TransportParcel<?> parcel) {
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "Incoming parcel " + BtUtil.getHashCode(parcel) 
                    + " data " + parcel.mData + " type " + parcel.mType);
        }
        if (null == mListener) {
        	return;
        }
        int type = parcel.getType();
        if (type != 0) {
        	return;
        }
        PositionParcel positionParcel = (PositionParcel)parcel;
        Point3F data = positionParcel.getData();
        mListener.onPositionReceived(data.x, data.y, data.z);
    }
    
    @Override
    protected TransportParcel<?> newParcel(int type) {
        switch (type) {
        case 0:
            return new PositionParcel(type);
        }
        return null;
    }    


}
