package com.samsung.dtl.wearableremote.profile;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.UUID;

import android.content.Context;
import android.util.Log;

import com.samsung.dtl.wearableremote.bluetooth.BtService;
import com.samsung.dtl.wearableremote.bluetooth.BtUtil;
import com.samsung.dtl.wearableremote.bluetooth.Point6F;


public class BtPosition6f extends BtProfileHelper implements BtService.Observer {
    
    private static final String LOG_TAG = BtUtil.getLogTag(BtPosition6f.class);

    public static BtPosition6f sInstance = null;
    
    public static BtPosition6f getInstance(Context context) {
        if (null == sInstance) {
            Context appContext = context.getApplicationContext();
            sInstance = new BtPosition6f(appContext); 
        }
        return sInstance;
    }
    
    public interface Listener {
        public boolean onPositionReceived(float x, float y, float z, float yaw, float pitch, float roll);
    }
       
    private BtPosition6f(Context context) {
        super(context, "Remote_Position_" + BtUtil.getHashCode(BtPosition6f.class), 
                UUID.fromString("21864823-7766-4993-8881-180bc37d0f81"), true);
     }

    private Listener mListener;
    
    public void setListener(Listener listener) {
        mListener = listener;
    }

    private class PositionParcel extends TransportParcel<Point6F> {

        public PositionParcel(int msgType) {
            super(msgType);
        }
        
        private PositionParcel set(float x, float y, float z, float yaw, float pitch, float roll) {
            setData(new Point6F(x, y, z, yaw, pitch, roll));
            return this;
        }
        
        @Override
        protected void read(DataInputStream is) throws IOException {
            float x = is.readFloat();
            float y = is.readFloat();
            float z = is.readFloat();
            float yaw = is.readFloat();
            float pitch = is.readFloat();
            float roll = is.readFloat();
            
            set(x, y, z, yaw, pitch, roll);
        }
        
        @Override
        protected void write(DataOutputStream os) throws IOException {
            os.writeFloat(mData.x);
            os.writeFloat(mData.y);
            os.writeFloat(mData.z);
            os.writeFloat(mData.yaw);
            os.writeFloat(mData.pitch);
            os.writeFloat(mData.roll);
            os.flush();

            if (sDebug) {
                //Log.d(LOG_TAG, "Writing " + mData);
            }
        }
        
    }
    
    public boolean sendData(float x, float y, float z, float yaw, float pitch, float roll) {
        PositionParcel parcel = PositionParcel.class.cast(newParcel(0));
        parcel.set(x, y, z, yaw, pitch, roll);
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
        Point6F data = positionParcel.getData();
        mListener.onPositionReceived(data.x, data.y, data.z, data.yaw, data.pitch, data.roll);
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
