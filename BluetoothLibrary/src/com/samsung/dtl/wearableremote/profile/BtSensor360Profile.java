package com.samsung.dtl.wearableremote.profile;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.util.Log;

import com.samsung.dtl.wearableremote.bluetooth.BtService;
import com.samsung.dtl.wearableremote.bluetooth.BtUtil;

public class BtSensor360Profile extends BtProfileHelper implements BtService.Observer {

    private static final String LOG_TAG = BtUtil.getLogTag(BtSensor360Profile.class);

    public static BtSensor360Profile sInstance = null;

    public static BtSensor360Profile getInstance(Context context) {
        if (null == sInstance) {
            Context appContext = context.getApplicationContext();
            sInstance = new BtSensor360Profile(appContext);
        }
        return sInstance;
    }

    public interface RotationListener {
        public void onRotationChanged(long time, float azimuth, float pitch, float roll,
                final float x, final float y, final float z, final float w);
    }

    public interface CompassListener {
        public void onCompassChangedListener(long time, float x, float y, float z);
    }

    private final BroadcastReceiver mScreenOnReceiver = new BroadcastReceiver() {
        @Override
        public void onReceive(Context context, Intent intent) {
            screenOn();
        }
    };

    private final BroadcastReceiver mScreenOffReceiver = new BroadcastReceiver() {
       @Override
       public void onReceive(Context context, Intent intent) {
           screenOff();
       }
    };

    private BtSensor360Profile(Context context) {
        super(context, "Remote_Sensor_" + BtUtil.getHashCode(BtSensor360Profile.class),
                UUID.fromString("7ee44e72-f155-471b-94d6-181479e99862"), true, false);
        Context appContext = context.getApplicationContext();

        IntentFilter filter = null;

        filter = new IntentFilter();
        filter.addAction(Intent.ACTION_SCREEN_ON);
        filter.addAction(Intent.ACTION_USER_PRESENT);
        appContext.registerReceiver(mScreenOnReceiver, filter);

        filter = new IntentFilter();
        filter.addAction(Intent.ACTION_SCREEN_OFF);
        appContext.registerReceiver(mScreenOffReceiver, filter);
    }

    private List<RotationListener> mRotationListeners = new ArrayList<RotationListener>();
    private List<CompassListener> mCompassListeners = new ArrayList<CompassListener>();

    public void registerRotationListener(RotationListener aListener) {
        if (!mRotationListeners.contains(aListener)) {
            mRotationListeners.add(aListener);
        }
    }

    public void unregisterRotationListener(RotationListener aListener) {
        mRotationListeners.remove(aListener);
    }

    public void registerCompassListener(CompassListener aListener) {
        if (!mCompassListeners.contains(aListener)) {
            mCompassListeners.add(aListener);
        }
    }

    public void unregisterCompassListener(CompassListener aListener) {
        mCompassListeners.remove(aListener);
    }

    private class RotationData {
        public long mTime;
        public float mAzimuth, mPitch, mRoll, mX, mY, mZ, mW;

        public RotationData(long time, float azimuth, float pitch, float roll,
                float x, float y, float z, float w) {
            mX = x;
            mY = y;
            mZ = z;
            mW = w;
            mTime = time;
            mAzimuth = azimuth;
            mPitch = pitch;
            mRoll = roll;
        }
    }

    private class RotationParcel extends TransportParcel<RotationData> {

        public RotationParcel() {
            super(ROTATION_CHANGE);
        }

        private RotationParcel set(long time, float azimuth, float pitch, float roll,
                float x, float y, float z, float w) {
            setData(new RotationData(time, azimuth, pitch, roll, x, y, z, w));
            return this;
        }

        @Override
        protected void read(DataInputStream is) throws IOException {
            long time = is.readLong();
            float azimuth = is.readFloat();
            float pitch = is.readFloat();
            float roll = is.readFloat();
            float x = is.readFloat();
            float y = is.readFloat();
            float z = is.readFloat();
            float w = is.readFloat();
            set(time, azimuth, pitch, roll, x, y, z, w);
        }

        @Override
        protected void write(DataOutputStream os) throws IOException {
            os.writeLong(mData.mTime);
            os.writeFloat(mData.mAzimuth);
            os.writeFloat(mData.mPitch);
            os.writeFloat(mData.mRoll);
            os.writeFloat(mData.mX);
            os.writeFloat(mData.mY);
            os.writeFloat(mData.mZ);
            os.writeFloat(mData.mW);
            os.flush();
            if (sDebug) {
                Log.d(LOG_TAG, "Writing " + mData);
            }
        }

    }

    private class ScreenStatusParcel extends TransportParcel<Boolean> {

        public ScreenStatusParcel() {
            super(SCREEN_STATUS);
        }

        private ScreenStatusParcel set(boolean on) {
            setData(Boolean.valueOf(on));
            return this;
        }

        @Override
        protected void read(DataInputStream is) throws IOException {
            set(is.readBoolean());
        }

        @Override
        protected void write(DataOutputStream os) throws IOException {
            os.writeBoolean(mData.booleanValue());
            if (sDebug) {
                Log.d(LOG_TAG, "Writing " + mData);
            }
        }
    }

    //private static final int COMPASS_CHANGE = 0;
    private static final int ROTATION_CHANGE = 1;
    private static final int SCREEN_STATUS = 3;

    public boolean sendRotationEvent(long time, float azimuth, float pitch, float roll,
            final float x, final float y, final float z, final float w) {
        TransportParcel<?> parcel = RotationParcel.class.cast(newParcel(ROTATION_CHANGE))
                .set(time, azimuth, pitch, roll, x, y, z, w);
        if (null == parcel) {
            return false;
        }
        dispatchParcel(parcel);
        return true;
    }


    @Override
    protected void onIncoming(ConnectionData connData, TransportParcel<?> parcel) {
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "Incoming parcel " + BtUtil.getHashCode(parcel)
                    + " data " + parcel.getData() + " type " + parcel.getType());
        }
        int type = parcel.getType();
        switch (type) {
        case ROTATION_CHANGE:
            onRecvRotationChange(parcel);
            break;
        case SCREEN_STATUS:
            onRecvScreenStatusChange(parcel);
            break;
        }
    }

    private void onRecvRotationChange(TransportParcel<?>  parcel) {
        RotationParcel rotationParcel = (RotationParcel)parcel;
        RotationData data = rotationParcel.getData();

        for (int i = mRotationListeners.size() - 1; i >= 0; i -= 1) {
            mRotationListeners.get(i).onRotationChanged(data.mTime,
                    data.mAzimuth, data.mPitch, data.mRoll, data.mX, data.mY, data.mZ, data.mW);
        }
    }

    private void onRecvScreenStatusChange(TransportParcel<?> parcel) {
    }

    private void screenOn() {
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "Screen on");
        }
        sendScreenStatusChange(true);
     }

     private void screenOff() {
         if (BtUtil.sDebug) {
             Log.d(LOG_TAG, "Screen off");
         }
         sendScreenStatusChange(false);
     }

     private void sendScreenStatusChange(boolean status) {
         ScreenStatusParcel parcel = ScreenStatusParcel.class.cast(newParcel(SCREEN_STATUS)).set(status);
         dispatchParcel(parcel);
     }

    @Override
    protected TransportParcel<?> newParcel(int type) {
        switch (type) {
        case ROTATION_CHANGE:
            return new RotationParcel();
        case SCREEN_STATUS:
            return new ScreenStatusParcel();
        }
        return null;
    }
}
