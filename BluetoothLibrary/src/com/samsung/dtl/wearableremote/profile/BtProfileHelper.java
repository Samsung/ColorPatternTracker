package com.samsung.dtl.wearableremote.profile;

import java.io.DataInputStream;
import java.io.DataOutputStream;
import java.io.IOException;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

import android.content.Context;
import android.os.Handler;
import android.os.HandlerThread;
import android.os.Looper;
import android.util.Log;

import com.samsung.dtl.wearableremote.bluetooth.BtConnectedSock;
import com.samsung.dtl.wearableremote.bluetooth.BtManager;
import com.samsung.dtl.wearableremote.bluetooth.BtService;
import com.samsung.dtl.wearableremote.bluetooth.BtUtil;

public abstract class BtProfileHelper implements BtService.Observer {

    private static final String LOG_TAG = BtUtil.getLogTag(BtProfileHelper.class);
    protected static final boolean sDebug = BtUtil.sDebug;

    protected final Handler mHandler;
    private final BtService mBtService;
    protected final UUID mUUID;
    protected final boolean mDispatchIncomingOnUIThread, mCheckForOutgoingDuplicates;

    protected BtProfileHelper(Context context, String name, UUID uuid,
            boolean dispatchIncomingOnUIThread, boolean checkForOutgoingDuplicates) {
        mHandler = new Handler(Looper.getMainLooper());
        mUUID = uuid;
        mDispatchIncomingOnUIThread = dispatchIncomingOnUIThread;
        mCheckForOutgoingDuplicates = checkForOutgoingDuplicates;
        Context appContext = context.getApplicationContext();
        BtManager manager = BtManager.getInstance(appContext);
        mBtService = manager.registerService(name, mUUID, 1);
    }

    protected BtProfileHelper(Context context, String name, UUID uuid, boolean dispatchIncomingOnUIThread) {
        this(context, name, uuid, dispatchIncomingOnUIThread, true);
    }

    public BtService getService() {
        return mBtService;
    }

    public boolean start() {
        mBtService.registerObserver(this);
        return mBtService.start();
    }

    public boolean stop() {
        mBtService.unregisterObserver(this);
        return mBtService.stop();
    }

    private class ConnectionDataCloser implements Runnable {

        private final ConnectionData mConnectionData;

        private ConnectionDataCloser(ConnectionData connData) {
            mConnectionData = connData;
        }

        @Override
        public void run() {
            mConnectionData.close();
        }
    }

    private enum TransportDirection {
        OUTGOING,
        INCOMING
    }

    public abstract class TransportParcel<T> {

        protected T mData;
        protected final int mType;

        protected TransportParcel(int type) {
            mType = type;
        }

        protected void setData(T data) {
            mData = data;
        }

        public int getType() {
            return mType;
        }

        public T getData() {
            return mData;
        }

        protected abstract void read(DataInputStream is) throws IOException;
        protected abstract void write(DataOutputStream os) throws IOException;

        protected boolean matches(TransportParcel<?> other) {
            if (null == other) {
                return false;
            }
            if (!this.getClass().equals(other.getClass())) {
                return false;
            }
            if (!(mType == other.mType) || null == mData || null == other.mData) {
                return false;
            }
            return mData.equals(other.mData);
        }
    }

    private List<Transporter> mRecycledTransporters = new ArrayList<Transporter>();
    private Object mLock = new Object();

    private void recycleTransporter(Transporter transporter) {
        synchronized (mLock) {
            mRecycledTransporters.add(transporter);
        }
    }

    protected Transporter getTransporter(ConnectionData connData) {
        Transporter result = null;
        synchronized (mLock) {
            if (mRecycledTransporters.size() > 0) {
                result = mRecycledTransporters.remove(0);
            }
        }
        if (null == result) {
            result = new Transporter();
        }
        return result.set(connData);
    }

    protected class Transporter implements Runnable {

        private TransportParcel<?> mParcel;
        private TransportDirection mDir;
        private ConnectionData mConnectionData;

        private Transporter set(ConnectionData connData) {
            if (null == connData) {
                throw new RuntimeException("Connection data cannot be null this " + BtUtil.getHashCode(this));
            }
            mConnectionData = connData;
            return this;
        }

        private Transporter set(TransportParcel<?> parcel) {
            if (null == parcel) {
                throw new RuntimeException("Parcel cannot be NULL this " + BtUtil.getHashCode(this));
            }
            mParcel = parcel;
            return this;
        }

        private void outgoing() {
            if (null == mParcel) {
                throw new RuntimeException("Trying to dispath an empty parcel this " + BtUtil.getHashCode(this));
            }
            mDir = TransportDirection.OUTGOING;
            mConnectionData.mWriterThreadHandler.post(this);
        }

        private void incomingOnUIThread() {
            mDir = TransportDirection.INCOMING;
            mHandler.post(this);
        }

        private void incomingOnThisThread() {
            mDir = TransportDirection.INCOMING;
            run();
        }

        @Override
        public void run() {
            switch (mDir) {
            case INCOMING:
                onIncoming(mConnectionData, mParcel);
                break;
            case OUTGOING:
                try {
                    mConnectionData.mOutputStream.writeInt(mParcel.mType);
                    mParcel.write(mConnectionData.mOutputStream);
                } catch (IOException ex) {
                    mHandler.post(new ConnectionDataCloser(mConnectionData));
                }
                break;
            }
            recycleTransporter(this);
        }
    }

    protected abstract TransportParcel<?> newParcel(int type);
    protected abstract void onIncoming(ConnectionData connData, TransportParcel<?> parcel);

    private class ReaderThread extends Thread {

        private final String LOG_TAG = BtUtil.getLogTag(ReaderThread.class);

        private final ConnectionData mConnectionData;

        public ReaderThread(ConnectionData data) {
            super("ReaderThread_" + BtUtil.getHashCode(data));
            mConnectionData = data;
        }

        public void run() {
            while (true) {
                try {
                    int type = mConnectionData.mInputStream.readInt();
                    if (sDebug) {
                        Log.d(LOG_TAG, "Received type " + type);
                    }
                    TransportParcel<?> parcel = newParcel(type);
                    if (null != parcel) {
                        parcel.read(mConnectionData.mInputStream);
                        Transporter transporter = getTransporter(mConnectionData).set(parcel);
                        if (mDispatchIncomingOnUIThread) {
                            transporter.incomingOnUIThread();
                        } else {
                            transporter.incomingOnThisThread();
                        }
                        yield();
                    } else {
                        break;
                    }
                } catch (IOException ex) {
                    BtUtil.logEx(LOG_TAG, "ReadThread::run", this, ex);
                    break;
                }
            }
            if (sDebug) {
                Log.d(LOG_TAG, "Terminating " + this + " " + BtUtil.getHashCode(this));
            }
            mHandler.post(new ConnectionDataCloser(mConnectionData));
        }
    }


    protected class ConnectionData {

        protected final Thread mReaderThread;
        protected final HandlerThread mWriterThread;
        protected final Handler mWriterThreadHandler;
        protected final BtConnectedSock mSock;
        protected final DataInputStream mInputStream;
        protected final DataOutputStream mOutputStream;

        private TransportParcel<?> mLastDispatched;


        private ConnectionData(BtConnectedSock sock) {
            mSock = sock;
            mInputStream = new DataInputStream(mSock.getInputStream());
            mOutputStream = new DataOutputStream(mSock.getOutputStream());
            mReaderThread = new ReaderThread(this);
            mReaderThread.start();
            mWriterThread = new HandlerThread("WriterThread_" + BtUtil.getHashCode(mSock));
            mWriterThread.start();
            mWriterThreadHandler = new Handler(mWriterThread.getLooper());
        }

        private void close() {
            mSock.close();
            mWriterThread.quit();
        }

        private boolean matches(BtConnectedSock sock) {
            return sock == mSock;
        }

        protected boolean send(TransportParcel<?> parcel) {
            if (mCheckForOutgoingDuplicates && parcel.matches(mLastDispatched)) {
                if (BtUtil.sDebug) {
                    Log.d(LOG_TAG, "Skipping send on parcel " + parcel + ", duplicate of the previously sent data "
                            + parcel.getData());
                }
                return false;
            }
            getTransporter(this).set(parcel).outgoing();
            mLastDispatched = parcel;
            return true;
        }
    }

    protected List<ConnectionData> mConnectionData = new ArrayList<ConnectionData>();

    @Override
    public void onConnected(BtConnectedSock sock) {
        if (sDebug) {
            Log.d(LOG_TAG, "Connected to " + sock);
        }
        ConnectionData data = new ConnectionData(sock);
        if (sDebug) {
            Log.d(LOG_TAG, "Got io streams " + sock);
        }
        mConnectionData.add(data);
    }


    @Override
    public void onDisconnected(BtConnectedSock sock) {
        if (sDebug) {
            Log.d(LOG_TAG, "Disconnected from " + sock);
        }
        for (int i = mConnectionData.size() - 1; i >= 0; i -= 1) {
            if (mConnectionData.get(i).matches(sock)) {
                mConnectionData.remove(i);
                break;
            }
        }
    }

    protected void dispatchParcel(TransportParcel<?> parcel) {
        if (null == parcel) {
            return;
        }
        for (int i = mConnectionData.size() - 1; i >= 0; i -= 1) {
            ConnectionData data = mConnectionData.get(i);
            data.send(parcel);
        }
    }
}
