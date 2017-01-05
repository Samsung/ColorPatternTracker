package com.samsung.dtl.wearableremote.bluetooth;

import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;
import java.util.UUID;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.bluetooth.BluetoothServerSocket;
import android.bluetooth.BluetoothSocket;
import android.os.Handler;
import android.os.Message;
import android.util.Log;

public class BtUtil {

    private static final String LOG_TAG = BtUtil.getLogTag(BtUtil.class);
    public static final boolean sDebug = true;

    static boolean close(BluetoothSocket sock, Object caller) {
        boolean result = false;
        try {
            sock.close();
            result = true;
        } catch (IOException ex) {
            logEx(LOG_TAG, "close client sock", caller, ex);
        }
        return result;
    }

    static boolean close(BluetoothServerSocket sock, Object caller) {
        boolean result = false;
        try {
            sock.close();
            result = true;
        } catch (IOException ex) {
            logEx(LOG_TAG, "close server sock", caller, ex);
        }
        return result;
    }

    static boolean connect(BluetoothSocket sock, Object caller) {
        boolean result = false;
        try {
            sock.connect();
            if (BtUtil.sDebug) {
                BluetoothDevice remote = sock.getRemoteDevice();
                Log.d(LOG_TAG, "Connected this " + getHashCode(caller) +
                        " with " + remote + " name " + remote.getName());
            }
            result = true;
        } catch (IOException ex) {
            logEx(LOG_TAG, "connect this " + getHashCode(caller) + " sock " + sock, caller, ex);
        }
        return result;
    }

    static BluetoothSocket create(BluetoothDevice device, UUID uuid, Object caller) {
        BluetoothSocket sock = null;
        try {
            sock = device.createInsecureRfcommSocketToServiceRecord(uuid);
            if (BtUtil.sDebug) {
                Log.d(LOG_TAG, "createInternal this " + getHashCode(caller) + " sock " + sock);
            }
        } catch (IOException ex) {
            sock = null;
            logEx(LOG_TAG, "create", caller, ex);
        }
        return sock;
    }

    protected static final int ACCEPT_WAIT_INDEFINITE = Integer.MIN_VALUE;

    static BluetoothSocket accept(BluetoothServerSocket serverSock, int timeout, Object caller) {
        BluetoothSocket result = null;
        try {
            if (ACCEPT_WAIT_INDEFINITE == timeout) {
                result = serverSock.accept();
            } else {
                result = serverSock.accept(timeout);
            }
            if (BtUtil.sDebug) {
                BluetoothDevice remote = result.getRemoteDevice();
                Log.d(LOG_TAG, "acceptInternal server sock " + serverSock + " result " + result +
                        " from " + remote + " name " + remote.getName());
            }
        } catch (IOException ex) {
            logEx(LOG_TAG, "acceptInternal failed for server sock " + serverSock, caller, ex);
        }
        return result;
    }

    static BluetoothServerSocket listen(String name, UUID uuid, Object caller) {
        BluetoothServerSocket result = null;
        BluetoothAdapter adapter = BtManager.getAdapter();
        if (null != adapter) {
            try {
                result = adapter.listenUsingInsecureRfcommWithServiceRecord(name, uuid);
                if (BtUtil.sDebug) {
                    Log.d(LOG_TAG, "listen name " + name + " uuid " + uuid + " server sock " + result);
                }
            } catch (Exception ex) {
                logEx(LOG_TAG, "listen name " + name + " uuid " + uuid, caller, ex);
                result = null;
            }
        }
        return result;
    }

    static InputStream inputStream(BluetoothSocket sock, Object caller) {
        InputStream result = null;
        try {
            result = sock.getInputStream();
        } catch (IOException ex) {
            logEx(LOG_TAG, "inputStream name ", caller, ex);
            result = null;
        }
        return result;
    }

    static OutputStream outputStream(BluetoothSocket sock, Object caller) {
        OutputStream result = null;
        try {
            result = sock.getOutputStream();
        } catch (IOException ex) {
            logEx(LOG_TAG, "outputStream name ", caller, ex);
            result = null;
        }
        return result;
    }

    static Message obtain(Handler handler, Runnable runnable, Object obj, int what) {
        Message msg = Message.obtain(handler, runnable);
        msg.obj = obj;
        msg.arg1 = -1;
        msg.arg2 = -1;
        msg.replyTo = null;
        msg.setData(null);
        msg.what = what;
        return msg;
    }

    public static String randomUUID() {
        return UUID.randomUUID().toString();
    }

    public static String getLogTag(Class<?> objClass) {
        return "log." + objClass.getName();
    }

    public static String getHashCode(Object obj) {
        return "0x" + Integer.toHexString(System.identityHashCode(obj));
    }

    public static void logEx(String tag, String msg, Object who, Exception ex) {
        Log.e(tag, msg + " who " + who + " ptr " + getHashCode(who) + " ex " + ex);
    }

    public static String getThreadName(Object who, int instanceCount) {
        Class<?> objClass = who.getClass();
        return objClass.getSimpleName() + "_" + getHashCode(who) + "_" + instanceCount;
    }

}
