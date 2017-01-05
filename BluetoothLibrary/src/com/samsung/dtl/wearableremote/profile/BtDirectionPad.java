package com.samsung.dtl.wearableremote.profile;


import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.UUID;

import android.content.Context;
import android.util.Log;

import com.samsung.dtl.wearableremote.bluetooth.BtService;
import com.samsung.dtl.wearableremote.bluetooth.BtUtil;

public class BtDirectionPad extends BtProfileHelper implements BtService.Observer {

    private static final String LOG_TAG = BtUtil.getLogTag(BtTouchPad.class);

    public static BtDirectionPad sInstance = null;

    public static BtDirectionPad getInstance(Context context) {
        if (null == sInstance) {
            Context appContext = context.getApplicationContext();
            sInstance = new BtDirectionPad(appContext);
        }
        return sInstance;
    }

    public interface Listener {
        public boolean onBtActionMove(int action);
        public boolean onBtActionTap();
    }

    private BtDirectionPad(Context context) {
        super(context, "Remote_Directionpad_" + BtUtil.getHashCode(BtTouchPad.class),
                UUID.fromString("0200adb3-3e4b-44cf-8544-05c3b2cf99a6"), true, false);
    }


    private Listener mListener;

    public void setListener(Listener listener) {
        mListener = listener;
    }

    private class ActionMoveParcel extends TransportParcel<Integer> {

        public ActionMoveParcel() {
            super(ACTION_MOVE);
        }

        private ActionMoveParcel set(int action) {
            setData(Integer.valueOf(action));
            return this;
        }

        @Override
        protected void read(DataInputStream is) throws IOException {
            set(is.readInt());
        }

        @Override
        protected void write(DataOutputStream os) throws IOException {
            os.writeInt(mData.intValue());
            os.flush();
            if (sDebug) {
                Log.d(LOG_TAG, "Writing action " + mData);
            }
        }

    }

    private class ActionTapParcel extends TransportParcel<Object> {

        public ActionTapParcel() {
            super(ACTION_TAP);
        }

        @Override
        protected void read(DataInputStream is) throws IOException {
        }

        @Override
        protected void write(DataOutputStream os) throws IOException {
        }

    }

    private static final int ACTION_MOVE = 0;
    private static final int ACTION_TAP = 1;

    public static final int ACTION_MOVE_NORTH = 1;
    public static final int ACTION_MOVE_SOUTH = 2;
    public static final int ACTION_MOVE_EAST = 4;
    public static final int ACTION_MOVE_WEST = 8;

    public boolean sendActionMove(int action) {
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "Sending move action " + action);
        }
        TransportParcel<?> parcel = ActionMoveParcel.class.cast(newParcel(ACTION_MOVE)).set(action);
        dispatchParcel(parcel);
        return true;
    }

    public boolean sendActionTap() {
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "Sending tap action");
        }
        TransportParcel<?> parcel = ActionTapParcel.class.cast(newParcel(ACTION_TAP));
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
        case ACTION_MOVE:
            onRecvActionMove(parcel);
            break;
        case ACTION_TAP:
            onRecvActionTap(parcel);
            break;
        }
    }

    private void onRecvActionMove(TransportParcel<?>  parcel) {
        if (null == mListener) {
            return;
        }

        ActionMoveParcel actionParcel = ActionMoveParcel.class.cast(parcel);
        int action = actionParcel.getData();
        mListener.onBtActionMove(action);
    }

    private void onRecvActionTap(TransportParcel<?>  parcel) {
        if (null == mListener) {
            return;
        }
        mListener.onBtActionTap();
    }

    @Override
    protected TransportParcel<?> newParcel(int type) {
        switch (type) {
        case ACTION_MOVE:
            return new ActionMoveParcel();
        case ACTION_TAP:
            return new ActionTapParcel();
        }
        return null;
    }

    public static boolean hasAction(int action, int check) {
        return check == (action & check);
    }
}
