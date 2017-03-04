package org.gearvrf.example.gvr_colorpositiontracking;

import android.app.Activity;
import android.os.Bundle;

import java.io.IOException;

import org.gearvrf.debug.DebugServer;
import org.gearvrf.GVRActivity;
import org.gearvrf.GVRCameraRig;
import org.gearvrf.GVRContext;
import org.gearvrf.GVRMain;
import org.gearvrf.GVRScene;
import org.gearvrf.GVRSceneObject;
import org.gearvrf.scene_objects.GVRTextViewSceneObject;
import org.gearvrf.scene_objects.GVRModelSceneObject;
import org.gearvrf.IErrorEvents;
import org.gearvrf.animation.GVRAnimationEngine;
import org.gearvrf.animation.GVRAnimation;
import org.gearvrf.animation.GVRRepeatMode;

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
        final DebugServer debug = gvrContext.startDebugServer();
            GVRScene scene = gvrContext.getNextMainScene();
            cameraRig = scene.getMainCameraRig();

            /*
            GVRTextViewSceneObject textview = new GVRTextViewSceneObject(gvrContext, "position tracking!");
            textview.getTransform().setPosition(0.0f, 0.0f, -4.0f);
            textview.setName("textview");
            scene.addSceneObject(textview);
            */

            String telnetString = "telnet " + "localhost" + " " + DebugServer.DEFAULT_DEBUG_PORT;

            android.util.Log.d(TAG, telnetString);

            IErrorEvents errorHandler = new IErrorEvents()
            {
                public void onError(String message, Object source)
                {
                    debug.logError(message);
                }
            };
            gvrContext.getEventReceiver().addListener(errorHandler);

            try {
                // load space platform
                GVRModelSceneObject platform = gvrContext.loadModel("https://github.com/gearvrf/GearVRf-Demos/raw/master/gvr-remote-scripting/app/src/main/assets/platform.fbx");
                platform.getTransform().setPosition(0, -2, -10);
                platform.setName("platform");
                scene.addSceneObject(platform);
            } catch(IOException e) {
                e.printStackTrace();
            } 

            try {
                // load space trex
                GVRModelSceneObject trex = gvrContext.loadModel("https://github.com/gearvrf/GearVRf-Demos/raw/master/gvr-meshanimation/app/src/main/assets/TRex_NoGround.fbx");
                trex.setName("trex");

                // place trex
                trex.getTransform().setPosition(0.0f, -2.0f, -10.0f);
                trex.getTransform().setRotationByAxis(90.0f, 1.0f, 0.0f, 0.0f);
                trex.getTransform().setRotationByAxis(0.0f, 0.0f, 1.0f, 0.0f);
                scene.addSceneObject(trex);

                GVRAnimationEngine engine = gvrContext.getAnimationEngine();
                GVRAnimation animation = trex.getAnimations().get(0);
                animation.setRepeatMode(GVRRepeatMode.REPEATED).setRepeatCount(-1);
                animation.start(engine);

            } catch(IOException e) {
                e.printStackTrace();
            } 
            /*
            try {
                GVRSceneObject test = gvrContext.loadModel("testscene.fbx");
                test.getTransform().setPosition(0.0f, 0.0f, -50.0f);
                test.setName("test");
                scene.addSceneObject(test);
            } catch(IOException e) {
                e.printStackTrace();
            } 
            */

            // done adding objects, bind shaders
            scene.bindShaders();

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
