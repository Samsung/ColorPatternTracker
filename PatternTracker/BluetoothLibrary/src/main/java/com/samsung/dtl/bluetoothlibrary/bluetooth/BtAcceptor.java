package com.samsung.dtl.bluetoothlibrary.bluetooth;

import java.io.InputStream;
import java.io.OutputStream;
import java.util.UUID;

import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;
import android.os.Handler;
import android.os.Looper;
import android.util.Log;

public class BtAcceptor {

    private final BtService mService;

    BtAcceptor(BtService service) {
        mService = service;
        mMainHandler = new Handler(Looper.getMainLooper());
    }

    private static int sThreadId = 0;
    private static final String LOG_TAG = BtUtil.getLogTag(BtAcceptor.class);

    private final Handler mMainHandler;

    private class IncomingConnectionNotifier implements Runnable {

        private final BluetoothSocket mClientSock;
        private final InputStream mInputStream;
        private final OutputStream mOutputStream;

        private IncomingConnectionNotifier(BluetoothSocket clientSock, InputStream is, OutputStream os) {
            mClientSock = clientSock;
            mInputStream = is;
            mOutputStream = os;
        }

        @Override
        public void run() {
            onIncomingConnection(mClientSock, mInputStream, mOutputStream);
        }
    };

    private class AcceptorThread extends Thread {

        private final String LOG_TAG = BtUtil.getLogTag(AcceptorThread.class);
        private final BluetoothServerSocket mThServerSock;

        public AcceptorThread(BluetoothServerSocket serverSock) {
            super(BtUtil.getThreadName(BtAcceptor.this, ++sThreadId));
            mThServerSock = serverSock;
        }

        public void run() {
            BluetoothSocket clientSock;
            do {
                clientSock = acceptInternal(mThServerSock);
                if (null != clientSock) {
                    InputStream is = BtUtil.inputStream(clientSock, this);
                    OutputStream os = BtUtil.outputStream(clientSock, this);
                    if (null != is && null != os) {
                        mMainHandler.post(new IncomingConnectionNotifier(clientSock, is, os));
                    } else {
                        BtUtil.close(clientSock, this);
                    }
                }
            } while (null != clientSock);
            if (BtUtil.sDebug) {
                Log.d(LOG_TAG, "Thread " + getName() + " terminated server sock " + mThServerSock);
            }
        }
    }

    boolean stop() {
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "Trying to stop acceptor this " + BtUtil.getHashCode(this) + " server sock " + mServerSock);
        }
        if (null == mServerSock) {
            return false;
        }
        BtUtil.close(mServerSock, this);
        mServerSock = null;
        mAcceptorThread = null;
        return true;
    }

    private Thread mAcceptorThread;
    private BluetoothServerSocket mServerSock;

    boolean start() {
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "Trying to start acceptor this " + BtUtil.getHashCode(this) + " server sock " + mServerSock);
        }
        if (null != mServerSock) {
            return false;
        }
        String name = mService.getName();
        UUID uuid = mService.getUUID();
        mServerSock = BtUtil.listen(name, uuid, this);
        if (null == mServerSock) {
            return false;
        }
        mAcceptorThread = new AcceptorThread(mServerSock);
        mAcceptorThread.start();
        return true;
    }

    boolean restart() {
        stop();
        return start();
    }

    private BluetoothSocket acceptInternal(BluetoothServerSocket serverSock) {
        return BtUtil.accept(serverSock, BtUtil.ACCEPT_WAIT_INDEFINITE, this);
    }

    private void onIncomingConnection(BluetoothSocket clientSock, InputStream is, OutputStream os) {

        if (BtUtil.sDebug) {
            BluetoothDevice remote = clientSock.getRemoteDevice();
            Log.d(LOG_TAG, "Accepted connection " + clientSock + " from " + remote + " name " + remote.getName());
        }
        BtConnectedSock connectedSock = new BtConnectedSock(clientSock, is, os);
        mService.dispatchConnected(connectedSock);
    }
}
