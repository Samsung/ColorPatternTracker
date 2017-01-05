package com.samsung.dtl.wearableremote.bluetooth;

import java.io.InputStream;
import java.io.OutputStream;
import java.lang.ref.WeakReference;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;

public class BtConnectedSock {

    //private static final String LOG_TAG = BtUtil.getLogTag(BtConnectedSock.class);
    private BluetoothSocket mSock;
    private InputStream mInputStream;
    private OutputStream mOutputStream;

    static interface DisconnectObserver {
        public void onDisconnected(BtConnectedSock sock);
    }

    private WeakReference<DisconnectObserver> mDisconnectObserverRef;

    void setDisconnectObserver(DisconnectObserver observer) {
        mDisconnectObserverRef = new WeakReference<DisconnectObserver>(observer);
        notifyDisconnectIfNeeded();
    }

    BtConnectedSock(BluetoothSocket sock, InputStream is, OutputStream os) {
        mSock = sock;
        if (null == is || null == os) {
            throw new RuntimeException("Streams should not be null input " + is +
                    " output " + os + " this " + BtUtil.getHashCode(this));
        }
        mInputStream = is;
        mOutputStream = os;
    }

    public boolean close() {
        if (null == mSock) {
            return false;
        }
        BtUtil.close(mSock, this);
        mSock = null;
        mInputStream = null;
        mOutputStream = null;
        notifyDisconnectIfNeeded();
        return true;
    }

    public BluetoothSocket getSock() {
        return mSock;
    }

    void notifyDisconnectIfNeeded() {
        DisconnectObserver observer = null;
        if (null != mDisconnectObserverRef) {
            observer = mDisconnectObserverRef.get();
            if (null == observer) {
                mDisconnectObserverRef = null;
                return;
            }
        }
        if (null != observer && null == mSock) {
            observer.onDisconnected(this);
        }
    }

    public InputStream getInputStream() {
        return mInputStream;
    }

    public OutputStream getOutputStream() {
        return mOutputStream;
    }

    boolean sameRemoteDevice(BtConnectedSock sock) {
        if (null == mSock || null == sock.mSock) {
            return false;
        }
        BluetoothDevice mine = mSock.getRemoteDevice();
        BluetoothDevice other = sock.mSock.getRemoteDevice();
        return mine.equals(other);
    }

}
