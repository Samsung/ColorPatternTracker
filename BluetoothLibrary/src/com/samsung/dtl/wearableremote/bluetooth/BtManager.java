package com.samsung.dtl.wearableremote.bluetooth;

import java.util.ArrayList;
import java.util.List;
import java.util.Set;
import java.util.UUID;

import android.bluetooth.BluetoothAdapter;
import android.bluetooth.BluetoothDevice;
import android.content.BroadcastReceiver;
import android.content.Context;
import android.content.Intent;
import android.content.IntentFilter;
import android.os.ParcelUuid;
import android.util.Log;

public class BtManager {

    private static final String LOG_TAG = BtUtil.getLogTag(BtManager.class);
    
    private static final BluetoothAdapter sAdapter = BluetoothAdapter.getDefaultAdapter();
    
    protected static BluetoothAdapter getAdapter() {
        return sAdapter;
    }
    
    public static boolean hasAdapter() {
        return null != sAdapter;
    }
    
    public static boolean isAdapterEnabled() {
        return (null != sAdapter) && sAdapter.isEnabled();
    }
    
    public static boolean cancelDiscovery() {
        if (isAdapterEnabled()) {
            return sAdapter.cancelDiscovery();
        }
        return false;
    }
    
    private final Context mContext;

    public static BtManager sInstance = null;
    
    public static BtManager getInstance(Context context) {
        if (null == sInstance) {
            Context appContext = context.getApplicationContext();
            sInstance = new BtManager(appContext); 
        }
        return sInstance;
    }
    
    private BtManager(Context context) {
        mContext = context;
        
        mContext.registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context arg0, Intent arg1) {
                onStateChanged(arg1);
            }
        }, new IntentFilter(BluetoothAdapter.ACTION_STATE_CHANGED));

        mContext.registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context arg0, Intent arg1) {
                onDiscoveryComplete();
            }
        }, new IntentFilter(BluetoothAdapter.ACTION_DISCOVERY_FINISHED));

        mContext.registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context arg0, Intent arg1) {
                onDeviceFound(arg1);
            }
        }, new IntentFilter(BluetoothDevice.ACTION_FOUND));


        mContext.registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context arg0, Intent arg1) {
                onDeviceUUIDFetched(arg1);
            }
        }, new IntentFilter(BluetoothDevice.ACTION_UUID));

        mContext.registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context arg0, Intent arg1) {
                onScanModeChanged(arg1);
            }
        }, new IntentFilter(BluetoothAdapter.ACTION_SCAN_MODE_CHANGED));

        mContext.registerReceiver(new BroadcastReceiver() {
            @Override
            public void onReceive(Context arg0, Intent arg1) {
                onBondStateChanged(arg1);
            }
        }, new IntentFilter(BluetoothDevice.ACTION_BOND_STATE_CHANGED));
        
        if (hasAdapter()) {
            Set<BluetoothDevice> allBonded = sAdapter.getBondedDevices();
            if (null != allBonded) {
                for (BluetoothDevice bonded : allBonded) {
                    onDeviceBonded(bonded);
                }
            }
        }
    }
    

    /*
     * Services
     */
    
    private List<BtService> mServices = new ArrayList<BtService>();
    
    public BtService registerService(String name, UUID uuid, int maxConnPerDevice) {
        BtService found = findService(uuid);
        if (null == found) {
            found = new BtService(name, uuid, maxConnPerDevice);
            mServices.add(found);
            for (BtBondedDevice bonded : mBondedDevices) {
                found.onDeviceBonded(bonded);
            }
        }
        return found;
    }
    
    
    public BtService findService(UUID uuid) {
        for (BtService service : mServices) {
            if (service.matches(uuid)) {
                return service;
            }
        }
        return null;
    }

    /*
     * Bonding handlers
     */
    
    private List<BtBondedDevice> mBondedDevices = new ArrayList<BtBondedDevice>();
    
    private boolean onDeviceBonded(BluetoothDevice device) {
        BtBondedDevice found = findBondedDevice(device);
        if (null == found) {
            found = new BtBondedDevice(device);
            for (BtService service : mServices) {
                service.onDeviceBonded(found);
            }
            mBondedDevices.add(found);
        }
        return true;
    }
    
    private boolean onDeviceUnbonded(BluetoothDevice device) {
        BtBondedDevice found = findBondedDevice(device);
        if (null == found) {
            return false;
        }
        for (BtService service : mServices) {
            service.onDeviceUnbonded(found);
        }
        mBondedDevices.remove(found);
        return true;
    }
    
    public BtBondedDevice findBondedDevice(BluetoothDevice device) {
        for (BtBondedDevice bonded : mBondedDevices) {
            if (bonded.matches(device)) {
                return bonded;
            }
        }
        return null;
    }
    
    /*
     * Utils
     */
    
    private String scanModeToStr(int mode) {
        switch (mode) {
        case BluetoothAdapter.SCAN_MODE_CONNECTABLE:
            return "Connectable";
        case BluetoothAdapter.SCAN_MODE_CONNECTABLE_DISCOVERABLE:
            return "Connectable and Discoverable";
        case BluetoothAdapter.SCAN_MODE_NONE:
            return "None";
        }
        return "Unknown";
    }

    private String bondToStr(int bond) {
        switch (bond) {
        case BluetoothDevice.BOND_BONDED:
            return "Bonded";
        case BluetoothDevice.BOND_BONDING:
            return "Bonding";
        case BluetoothDevice.BOND_NONE:
            return "None";
        }
        return "Unknown";
    }
    
    private String stateToStr(int state) {
        switch (state) {
        case BluetoothAdapter.STATE_CONNECTED:
            return "Connected";
        case BluetoothAdapter.STATE_CONNECTING:
            return "Connecting";
        case BluetoothAdapter.STATE_DISCONNECTED:
            return "Disconnected";
        case BluetoothAdapter.STATE_DISCONNECTING:
            return "Disconnecting";
        case BluetoothAdapter.STATE_OFF:
            return "Off";
        case BluetoothAdapter.STATE_ON:
            return "On";
        case BluetoothAdapter.STATE_TURNING_OFF:
            return "Turning off";
        case BluetoothAdapter.STATE_TURNING_ON:
            return "Turning on";
        }
        return "Unknown";
    }
    
    
    /*
     * Broadcast handlers
     */
    
    private void onScanModeChanged(Intent intent) {
        int currScanMode = intent.getIntExtra(BluetoothAdapter.EXTRA_SCAN_MODE, -1);
        int prevScanMode = intent.getIntExtra(BluetoothAdapter.EXTRA_PREVIOUS_SCAN_MODE, -1);
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "onScanModeChanged curr: " + 
                    scanModeToStr(currScanMode) + " prev: " + scanModeToStr(prevScanMode));
        }
        for (BtService service : mServices) {
            service.onScanModeChanged(prevScanMode, currScanMode);
        }
    }
    
    private void onStateChanged(Intent intent) {
        int currState = intent.getIntExtra(BluetoothAdapter.EXTRA_STATE, -1);
        int prevState = intent.getIntExtra(BluetoothAdapter.EXTRA_PREVIOUS_STATE, -1);
        
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "onStateChanged curr " + stateToStr(currState) + " prev " + stateToStr(prevState));
        }
        for (BtService service : mServices) {
            service.onStateChanged(prevState, currState);
        }
    }
    
    private void onDeviceFound(Intent intent) {
        BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "onDeviceFound device " + device);
        }
    }

    private void onDiscoveryComplete() {
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "onDiscoveryComplete");
        }
    }
    
    private void onBondStateChanged(Intent intent) {
        BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
        int currBondState = intent.getIntExtra(BluetoothDevice.EXTRA_BOND_STATE, -1);
        int prevBondState = intent.getIntExtra(BluetoothDevice.EXTRA_PREVIOUS_BOND_STATE, -1);
        
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "onBondStateChanged device " + device + " curr " + 
                    bondToStr(currBondState) + " prev " + bondToStr(prevBondState));
        }
        if (null != device) {
            switch (device.getBondState()) {
            case BluetoothDevice.BOND_BONDED:
                onDeviceBonded(device);
                break;
            case BluetoothDevice.BOND_NONE:
                onDeviceUnbonded(device);
                break;
            }
        }
    }

    private void onDeviceUUIDFetched(Intent intent) {
        BluetoothDevice device = intent.getParcelableExtra(BluetoothDevice.EXTRA_DEVICE);
        ParcelUuid uuid = intent.getParcelableExtra(BluetoothDevice.EXTRA_UUID);
        if (BtUtil.sDebug) {
            Log.d(LOG_TAG, "Found uuid on scan device " + device + " uuid " + uuid + 
                    " intent " + intent + " extras " + intent.getExtras());
        }
    }
    
    /*
     * methods
     */
    
}

