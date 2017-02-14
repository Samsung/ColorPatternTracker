package com.samsung.dtl.bluetoothlibrary.bluetooth;

import java.io.InputStream;
import java.io.OutputStream;
import java.util.UUID;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothSocket;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

public class BtConnector {

    private static int sThreadId = 0;
    private static final String LOG_TAG = BtUtil.getLogTag(BtConnector.class);
    protected static final int ACCEPT_WAIT_INDEFINITE = -1;

    private final BtBondedDevice mDevice;
    private final BtService mService;
    private final Handler mMainHandler;

    protected BluetoothSocket mSock;
    private Thread mConnectorThread;

    BtConnector(BtBondedDevice device, BtService service) {
        mDevice = device;
        mService = service;
        mMainHandler = new Handler(Looper.getMainLooper());
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "New connector for device " + device + " service " + service);
        }
    }

    private class ConnectedNotifier implements Runnable {

        private final BluetoothSocket mSock;
        private final InputStream mInputStream;
        private final OutputStream mOutputStream;

        private ConnectedNotifier(BluetoothSocket sock, InputStream is, OutputStream os) {
            mSock = sock;
            mInputStream = is;
            mOutputStream = os;
        }

        @Override
        public void run() {
            onConnected(mSock, mInputStream, mOutputStream);
        }
    };

    private class ConnectorThread extends Thread {

        private final String LOG_TAG = BtUtil.getLogTag(ConnectorThread.class);

        private final BluetoothSocket mThSock;

        public ConnectorThread(BluetoothSocket sock) {
            super(BtUtil.getThreadName(BtConnector.this, ++sThreadId));
            mThSock = sock;
        }

        public void run() {
            boolean success = false;
            if (BtUtil.connect(mThSock, this)) {
                InputStream is = BtUtil.inputStream(mThSock, this);
                OutputStream os = BtUtil.outputStream(mThSock, this);
                if (null != is && null != os) {
                    mMainHandler.post(new ConnectedNotifier(mThSock, is, os));
                    success = true;
                } else {
                    mMainHandler.post(new Runnable() {
                        @Override
                        public void run() {
                            BtConnector.this.stop();
                        }
                    });
                }
            }
            if (BtUtil.sDebug) {
                Log.d(LOG_TAG, "Thread " + getName() + " terminated sock " + mThSock + " success " + success);
            }
        }
    }

    boolean start() {
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "Trying to start connector this " + BtUtil.getHashCode(this) + " sock " + mSock);
        }

        if (null != mSock) {
            return false;
        }
        BluetoothDevice device = mDevice.getDevice();
        UUID uuid = mService.getUUID();
        BtManager.cancelDiscovery();
        mSock = BtUtil.create(device, uuid, this);
        if (null == mSock) {
            return false;
        }
        mConnectorThread = new ConnectorThread(mSock);
        mConnectorThread.start();
        return true;
    }

    boolean stop() {
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "Trying to stop connector this " + BtUtil.getHashCode(this) + " sock " + mSock);
        }
        if (null == mSock) {
            return false;
        }
        BtUtil.close(mSock, this);
        mSock = null;
        mConnectorThread = null;
        return true;
    }

    boolean restart() {
        stop();
        return start();
    }

    boolean matches(BtService service) {
        return mService.matches(service);
    }

    boolean matches(BtBondedDevice device) {
        return mDevice.matches(device);
    }

    private void onConnected(BluetoothSocket sock, InputStream is, OutputStream os) {
        if (sock != mSock) {
            BtUtil.close(sock, this);
            return;
        }
        BtConnectedSock connectedSock = new BtConnectedSock(mSock, is, os);
        mService.dispatchConnected(connectedSock);
    }

}
