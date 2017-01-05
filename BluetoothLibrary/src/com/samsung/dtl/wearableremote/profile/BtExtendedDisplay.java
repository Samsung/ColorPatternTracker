package com.samsung.dtl.wearableremote.profile;

import java.io.ByteArrayOutputStream;
import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.UUID;

import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.graphics.Bitmap;
import android.graphics.BitmapFactory;
import android.graphics.Point;
import android.util.Log;

import com.samsung.dtl.wearableremote.bluetooth.BtService;
import com.samsung.dtl.wearableremote.bluetooth.BtUtil;

public class BtExtendedDisplay extends BtProfileHelper implements BtService.Observer {
    private static final String LOG_TAG = BtUtil.getLogTag(BtExtendedDisplay.class) + "CRR";
    private static final boolean sDebug = false;
    
    private static BtExtendedDisplay sInstance = null;
    
    public static BtExtendedDisplay getInstance(Context context) {
        if (null == sInstance) {
            Context appContext = context.getApplicationContext();
            sInstance = new BtExtendedDisplay(appContext);
        }
        return sInstance;
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
    
    private BtExtendedDisplay(Context context) {
        super(context, "Extended_Display_" + BtUtil.getHashCode(BtExtendedDisplay.class), 
                UUID.fromString("44c76ed0-4b53-4a1b-bf55-882b5bcff362"), true);
        
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

    private class PageInfoParcel extends TransportParcel<PageDisplayInfo> {

        protected PageInfoParcel() {
            super(PAGE_INFO);
        }

        private PageInfoParcel set(int index, Bitmap image) {
            PageDisplayInfo item = new PageDisplayInfo();
            item.mBitmap = image;
            item.mIndex = index;
            setData(item);
            return this;
        }
        
        @Override
        protected void read(DataInputStream is) throws IOException {
            final int index = is.readInt();
            final int bufferLength = is.readInt();
            byte[] bytes = new byte[bufferLength];
            int count = 0;
            while (count < bufferLength) {
                int read = is.read(bytes, count, bufferLength - count);
                count += read;
            }

            Bitmap b = BitmapFactory.decodeByteArray(bytes, 0, bytes.length, null);
            set(index, b);
        }

        @Override
        protected void write(DataOutputStream os) throws IOException {
            ByteArrayOutputStream stream = new ByteArrayOutputStream();
            mData.mBitmap.compress(Bitmap.CompressFormat.PNG, 100, stream);
            byte[] bytes = stream.toByteArray();
            os.writeInt(mData.mIndex);
            os.writeInt(bytes.length);
            os.write(bytes);
            os.flush();
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
    
    private class ScrollLocationParcel extends TransportParcel<Point> {

        public ScrollLocationParcel() {
            super(SCROLL_LOCATION);
        }
        
        private ScrollLocationParcel set(int x, int y) {
            setData(new Point(x, y));
            return this;
        }
        
        @Override
        protected void read(DataInputStream is) throws IOException {
            int x = is.readInt();
            int y = is.readInt();
            set(x, y);
        }
        
        @Override
        protected void write(DataOutputStream os) throws IOException {
            os.writeInt(mData.x);
            os.writeInt(mData.y);
            if (sDebug) {
                Log.d(LOG_TAG, "Writing " + mData);
            }
        }
        
    }   
    
    private static final int PAGE_INFO = 0;
    private static final int SCREEN_STATUS = 1;
    private static final int SCROLL_LOCATION = 2;
    
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
     
     private void onRecvScrollChange(TransportParcel<?> parcel) {
         if (null == mListener) return;
         ScrollLocationParcel scrollParcel = (ScrollLocationParcel) parcel;
         mListener.onScrollChange(scrollParcel.mData.x, scrollParcel.mData.y);
     }
     
     private void onRecvScreenStatusChange(TransportParcel<?> parcel) {
         if (null == mListener) return;
         ScreenStatusParcel statusParcel = (ScreenStatusParcel)parcel;
         mListener.onScreenStatusChanged(statusParcel.getData().booleanValue());
     }
     
     private void onRecvExtendedDisplayInfo(TransportParcel<?> parcel) {
         if (null == mListener) return;
         PageInfoParcel pageInfoParcel = (PageInfoParcel) parcel;
         mListener.onPageUpdate(pageInfoParcel.mData.mIndex, pageInfoParcel.mData.mBitmap);
     }
     
     public interface ExtendedDisplayListener {
         public void onScreenStatusChanged(boolean on);
         public void onPageUpdate(int index, Bitmap page);
         public void onScrollChange(int x, int y);
     }
     
     private ExtendedDisplayListener mListener;
     
     public void setListener(ExtendedDisplayListener listener) {
         mListener = listener;
     }
     
     public static class PageDisplayInfo {
         public Bitmap mBitmap;
         public int mIndex;
     }
     
     public void sendExtendedDisplayInfo(Bitmap[] pages) {
         for (int i = 0; i < pages.length; i++) {
             TransportParcel<?> parcel = null;
             parcel = PageInfoParcel.class.cast(newParcel(PAGE_INFO)).set(i, pages[i]);
             dispatchParcel(parcel);
         }
     }
     
     public void sendScrollLocation(int x, int y) {
         TransportParcel<?> parcel = null;
         
         parcel = ScrollLocationParcel.class.cast(newParcel(SCROLL_LOCATION)).set(x, y);
         dispatchParcel(parcel);
     }

    @Override
    protected TransportParcel<?> newParcel(int type) {
        switch (type) {
        case SCROLL_LOCATION:
            return new ScrollLocationParcel();
        case SCREEN_STATUS:
            return new ScreenStatusParcel();
        case PAGE_INFO:
            return new PageInfoParcel();
        }
        return null;
    }

    @Override
    protected void onIncoming(ConnectionData connData, TransportParcel<?> parcel) {
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "Incoming parcel " + BtUtil.getHashCode(parcel) 
                    + " data " + parcel.mData + " type " + parcel.mType);
        }
        int type = parcel.getType();
        switch (type) {
        case SCROLL_LOCATION:
            onRecvScrollChange(parcel);
            break;
        case SCREEN_STATUS:
            onRecvScreenStatusChange(parcel);
            break;
           case PAGE_INFO:
               onRecvExtendedDisplayInfo(parcel);
               break;
           default:
               break;
        }        
    }

}
