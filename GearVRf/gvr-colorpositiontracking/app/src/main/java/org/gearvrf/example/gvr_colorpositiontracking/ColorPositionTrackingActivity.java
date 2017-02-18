package org.gearvrf.example.gvr_colorpositiontracking;

import android.app.Activity;
import android.os.Bundle;

import org.gearvrf.GVRActivity;
import org.gearvrf.GVRCameraRig;
import org.gearvrf.GVRContext;
import org.gearvrf.GVRMain;
import org.gearvrf.GVRScene;
import org.gearvrf.scene_objects.GVRTextViewSceneObject;

import com.samsung.dtl.wearableremote.profile.BTReceiverClass;

public class ColorPositionTrackingActivity extends GVRActivity {

    private GVRCameraRig cameraRig;
    private BTReceiverClass btreceiver;

    private static final String TAG = "ColorPositionTrackingActivity";

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        super.onCreate(savedInstanceState);

        setMain(new ColorPositionTrackingMain(), "gvr.xml");

        btreceiver = BTReceiverClass.newInstance(this);
    }

    private class ColorPositionTrackingMain extends GVRMain {
        public void onInit(GVRContext gvrContext) {
            GVRScene scene = gvrContext.getNextMainScene();
            cameraRig = scene.getMainCameraRig();
            GVRTextViewSceneObject textview = new GVRTextViewSceneObject(gvrContext, "position tracking!");
            textview.getTransform().setPosition(0.0f, 0.0f, -4.0f);
            scene.addSceneObject(textview);
        }

        public void onStep() {
            float x = btreceiver.getX();
            float y = btreceiver.getY();
            float z = btreceiver.getZ();

            // convert from cm to m
            x /= 10.0f;
            y /= 10.0f;
            z /= 10.0f;

            cameraRig.getHeadTransform().setPosition(y, x, z);
            android.util.Log.d(TAG, "head x, y, z: " + y + ", " + x + ", " + z);

        }

    }
}
