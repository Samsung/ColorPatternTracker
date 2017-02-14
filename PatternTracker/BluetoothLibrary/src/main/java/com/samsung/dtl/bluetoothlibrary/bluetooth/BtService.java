package com.samsung.dtl.bluetoothlibrary.bluetooth;

import java.lang.ref.WeakReference;
import java.util.ArrayList;
import java.util.List;
import java.util.UUID;

import android.os.Handler;
import android.os.Looper;
import android.os.Message;
import android.util.Log;

public class BtService {

    private static final String LOG_TAG = BtUtil.getLogTag(BtService.class);

    private final String mName;
    private final UUID mUUID;
    private final Handler mMainHandler;
    private final int mMaxConnPerDevice;
    private BtAcceptor mAcceptor;

    public static final int INFINITE_CONNECTIONS_PER_DEVICE = Integer.MIN_VALUE;

    BtService(String name, UUID uuid, int maxConnPerDevice) {
        mName = name;
        mUUID = uuid;
        mMaxConnPerDevice = maxConnPerDevice;
        mMainHandler = new Handler(Looper.getMainLooper());

        mAcceptor = new BtAcceptor(this);
    }

    public static interface Observer {
        public void onConnected(BtConnectedSock sock);
        public void onDisconnected(BtConnectedSock sock);
    }

    private List<WeakReference<Observer>> mObserverRefs = new ArrayList<WeakReference<Observer>>();

    public boolean registerObserver(Observer observer) {
        WeakReference<Observer> ref = findObserverRef(observer);
        if (null != ref) {
            return false;
        }
        ref = new WeakReference<Observer>(observer);
        if (mObserverRefs.add(ref)) {
            Message msg = BtUtil.obtain(mMainHandler, new AllAvailableConnectionsProvider(observer),
                    observer, 0);
            mMainHandler.sendMessage(msg);
            return true;
        }
        return false;
    }

    public boolean unregisterObserver(Observer observer) {
        WeakReference<Observer> ref = findObserverRef(observer);
        if (null == ref) {
            return false;
        }
        mMainHandler.removeMessages(0, observer);
        mObserverRefs.remove(ref);
        return true;
    }

    private WeakReference<Observer> findObserverRef(Observer observer) {
        for (int i = mObserverRefs.size() - 1; i >= 0; i -= 1) {
            WeakReference<Observer> ref = mObserverRefs.get(i);
            Observer temp = ref.get();
            if (null == temp) {
                mObserverRefs.remove(i);
                continue;
            }
            if (temp == observer) {
                return ref;
            }
        }
        return null;
    }

    public int getConnectedCount() {
        return mConnectedSocks.size();
    }

    private abstract class ConnectionNotifier implements Runnable {
        protected final BtConnectedSock mSock;

        private ConnectionNotifier(BtConnectedSock sock) {
            mSock = sock;
        }

        protected abstract boolean dispatch(Observer observer);
        protected abstract boolean onRun();

        @Override
        public void run() {
            if (onRun()) {
                for (int i = mObserverRefs.size() - 1; i >= 0; i -= 1) {
                    WeakReference<Observer> ref = mObserverRefs.get(i);
                    Observer temp = ref.get();
                    if (null == temp) {
                        mObserverRefs.remove(i);
                        continue;
                    }
                    if (!dispatch(temp)) {
                        break;
                    }
                }
            }
        }
    }

    private BtConnectedSock.DisconnectObserver mDisconnectObserver = new BtConnectedSock.DisconnectObserver() {
        @Override
        public void onDisconnected(BtConnectedSock sock) {
            dispatchDisconnected(sock);
        }
    };

    private List<BtConnectedSock> mConnectedSocks = new ArrayList<BtConnectedSock>();

    private class ConnectedNotifier extends ConnectionNotifier {

        private ConnectedNotifier(BtConnectedSock sock) {
            super(sock);
        }

        @Override
        protected boolean onRun() {
            if (INFINITE_CONNECTIONS_PER_DEVICE != mMaxConnPerDevice) {

                int connCount = 0;
                for (int i = mConnectedSocks.size() - 1; i >= 0; i -= 1) {
                    if (mSock.sameRemoteDevice(mConnectedSocks.get(i))) {
                        connCount += 1;
                        if (connCount >= mMaxConnPerDevice) {
                            if (BtUtil.sDebug) {
                                Log.d(LOG_TAG, "Connect count exceeded close sock " + mSock + " count " +
                                            connCount + " max " + mMaxConnPerDevice);
                            }
                            mSock.close();
                            return false;
                        }
                    }
                }
            }
            mSock.setDisconnectObserver(mDisconnectObserver);
            mConnectedSocks.add(mSock);
            return true;
        }

        @Override
        protected boolean dispatch(Observer observer) {
            observer.onConnected(mSock);
            return true;
        }
    }

    private class DisconnectedNotifier extends ConnectionNotifier {

        private DisconnectedNotifier(BtConnectedSock sock) {
            super(sock);
        }

        @Override
        protected boolean onRun() {
            mSock.setDisconnectObserver(null);
            return mConnectedSocks.remove(mSock);
        }

        @Override
        protected boolean dispatch(Observer observer) {
            observer.onDisconnected(mSock);
            return true;
        }
    }

    private class AllAvailableConnectionsProvider implements Runnable {

        private final Observer mObserver;

        private AllAvailableConnectionsProvider(Observer observer) {
            mObserver = observer;
        }

        @Override
        public void run() {
            for (BtConnectedSock sock : mConnectedSocks) {
                mObserver.onConnected(sock);
            }
        }

    }

    boolean matches(UUID uuid) {
        if (null == mUUID || null == uuid) {
            return false;
        }
        return mUUID.equals(uuid);
    }

    boolean matches(BtService service) {
        UUID other = service.getUUID();
        if (null == mUUID || null == other) {
            return false;
        }
        return mUUID.equals(other);
    }

    public UUID getUUID() {
        return mUUID;
    }

    public String getName() {
        return mName;
    }

    void onScanModeChanged(int prevScanMode, int currScanMode) {
        if (enabled()) {
            mAcceptor.restart();
            for (BtConnector connector : mConnectors) {
                connector.restart();
            }
        } else {
            mAcceptor.stop();
            for (BtConnector connector : mConnectors) {
                connector.stop();
            }

        }
    }

    void onStateChanged(int prevState, int currState) {
        if (enabled()) {
            mAcceptor.restart();
            for (BtConnector connector : mConnectors) {
                connector.restart();
            }
        } else {
            mAcceptor.stop();
            for (BtConnector connector : mConnectors) {
                connector.stop();
            }
        }
    }

    boolean dispatchConnected(BtConnectedSock sock) {
        return mMainHandler.post(new ConnectedNotifier(sock));
    }

    boolean dispatchDisconnected(BtConnectedSock sock) {
        return mMainHandler.post(new DisconnectedNotifier(sock));
    }

    private boolean mStarted = false;

    public boolean start() {
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "starting service this " + BtUtil.getHashCode(this) + " uuid " + mUUID);
        }
        mStarted = true;
        if (!BtManager.isAdapterEnabled()) {
            return false;
        }
        mAcceptor.start();
        for (BtConnector connector : mConnectors) {
            connector.start();
        }
        return true;
    }

    public boolean stop() {
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "stopping service this " + BtUtil.getHashCode(this) + " uuid " + mUUID);
        }
        mStarted = false;
        mAcceptor.stop();
        for (BtConnector connector : mConnectors) {
            connector.stop();
        }
        return true;
    }

    private boolean enabled() {
        return mStarted && BtManager.isAdapterEnabled();
    }

    private List<BtConnector> mConnectors = new ArrayList<BtConnector>();

    public BtConnector findConnector(BtBondedDevice device) {
        for (BtConnector connector : mConnectors) {
            if (connector.matches(device)) {
                return connector;
            }
        }
        return null;
    }

    void onDeviceBonded(BtBondedDevice device) {
        BtConnector found = findConnector(device);
        if (null == found) {
            found = new BtConnector(device, this);
            mConnectors.add(found);
        }
        if (enabled()) {
            found.start();
        }
    }

    void onDeviceUnbonded(BtBondedDevice device) {
        BtConnector found = findConnector(device);
        if (null != found) {
            found.stop();
            mConnectors.remove(found);
        }

    }

}
