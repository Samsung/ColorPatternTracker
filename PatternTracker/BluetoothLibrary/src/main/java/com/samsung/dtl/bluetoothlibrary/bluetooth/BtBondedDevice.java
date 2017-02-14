package com.samsung.dtl.bluetoothlibrary.bluetooth;

import android.bluetooth.BluetoothDevice;
import android.util.Log;

public class BtBondedDevice {

    private final BluetoothDevice mDevice;

    private static final String LOG_TAG = BtUtil.getLogTag(BtBondedDevice.class);

    BtBondedDevice(BluetoothDevice device) {
        mDevice = device;
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "New bonded device this " + BtUtil.getHashCode(this) + " device " + mDevice +
                    " name " + mDevice.getName());
        }
    }

    boolean matches(BluetoothDevice device) {
        if (null == mDevice || null == device) {
            return false;
        }
        return mDevice.equals(device);
    }

    BluetoothDevice getDevice() {
        return mDevice;
    }


    boolean matches(BtBondedDevice device) {
        if (null == mDevice || null == device.mDevice) {
            return false;
        }
        return mDevice.equals(device.mDevice);
    }
}
