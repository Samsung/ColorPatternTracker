package com.samsung.dtl.bluetoothlibrary.profile;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.UUID;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Point;
import android.graphics.PointF;
import android.os.SystemClock;
import android.util.Log;
import android.view.MotionEvent;

import com.samsung.dtl.bluetoothlibrary.bluetooth.BtService;
import com.samsung.dtl.bluetoothlibrary.bluetooth.BtUtil;

public class BtTouchPad extends BtProfileHelper implements BtService.Observer {
    
    private static final String LOG_TAG = BtUtil.getLogTag(BtTouchPad.class);

    public static BtTouchPad sInstance = null;
    
    public static BtTouchPad getInstance(Context context) {
        if (null == sInstance) {
            Context appContext = context.getApplicationContext();
            sInstance = new BtTouchPad(appContext); 
        }
        return sInstance;
    }
    
    public interface Listener {
        public boolean onBtTouchEvent(MotionEvent event);
        public boolean onScreenStatusChanged(boolean on);
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
    
    private BtTouchPad(Context context) {
        super(context, "Remote_Touchpad_" + BtUtil.getHashCode(BtTouchPad.class), 
                UUID.fromString("218bf423-7766-4993-8881-180bc37d0f81"), true);
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
    
    private Point mSize = new Point();
    
    public void setSize(Point size) {
        mSize.set(size.x, size.y);
    }

    private Listener mListener;
    
    public void setListener(Listener listener) {
        mListener = listener;
    }

    private class LocationParcel extends TransportParcel<PointF> {

        public LocationParcel(int msgType) {
            super(msgType);
        }
        
        private LocationParcel set(float x, float y) {
            setData(new PointF(x, y));
            return this;
        }
        
        @Override
        protected void read(DataInputStream is) throws IOException {
            float x = is.readFloat();
            float y = is.readFloat();
            set(x, y);
        }
        
        @Override
        protected void write(DataOutputStream os) throws IOException {
            os.writeFloat(mData.x);
            os.writeFloat(mData.y);
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
    
    private static final int TOUCH_DOWN = 0;
    private static final int TOUCH_MOVE = 1;
    private static final int TOUCH_UP = 2;
    private static final int SCREEN_STATUS = 3;
    
    private float roundTouchLocation(float location) {
        return (float)(Math.round(location * 100.0) / 100.0);
    }
    
    public boolean sendTouchEvent(MotionEvent event) {
        if (mSize.x == 0 || mSize.y == 0) {
            return false;
        }
        int action = event.getAction();
        TransportParcel<?> parcel = null;
        float x = roundTouchLocation(event.getX() / mSize.x);
        float y = roundTouchLocation(event.getY() / mSize.y);
        switch (action) {
        case MotionEvent.ACTION_DOWN:
            parcel = LocationParcel.class.cast(newParcel(TOUCH_DOWN)).set(x, y);
            break;
        case MotionEvent.ACTION_MOVE:
            parcel = LocationParcel.class.cast(newParcel(TOUCH_MOVE)).set(x, y);
            break;
        case MotionEvent.ACTION_UP:
            parcel = LocationParcel.class.cast(newParcel(TOUCH_UP)).set(x, y);
            break;
        }
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
                    + " data " + parcel.mData + " type " + parcel.mType);
        }
        int type = parcel.getType();
        switch (type) {
        case TOUCH_DOWN:
            onRecvTouchEvent(MotionEvent.ACTION_DOWN, parcel);
            break;
        case TOUCH_UP:
            onRecvTouchEvent(MotionEvent.ACTION_UP, parcel);
            break;
        case TOUCH_MOVE:
            onRecvTouchEvent(MotionEvent.ACTION_MOVE, parcel);
            break;
        case SCREEN_STATUS:
            onRecvScreenStatusChange(parcel);
            break;
        }
    }
    
    private void onRecvTouchEvent(int action, TransportParcel<?>  parcel) {
        if (null == mListener || 0 == mSize.x || 0 == mSize.y) {
            return;
        }
        
        LocationParcel locationParcel = (LocationParcel)parcel;
        PointF data = locationParcel.getData();
        long time = SystemClock.uptimeMillis();
        int x = (int)(data.x * (float)mSize.x);
        int y = (int)(data.y * (float)mSize.y);
        MotionEvent event = MotionEvent.obtain(time, time, action, x, y, 0);
        mListener.onBtTouchEvent(event);
        event.recycle();
    }
    
    private void onRecvScreenStatusChange(TransportParcel<?> parcel) {
        if (null == mListener) {
            return;
        }
        ScreenStatusParcel statusParcel = (ScreenStatusParcel)parcel;
        mListener.onScreenStatusChanged(statusParcel.getData().booleanValue());
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
        case TOUCH_DOWN:
        case TOUCH_UP:
        case TOUCH_MOVE:
            return new LocationParcel(type);
        case SCREEN_STATUS:
            return new ScreenStatusParcel();
        }
        return null;
    }
}
