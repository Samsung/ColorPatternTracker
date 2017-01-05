using UnityEngine;
using System;
using System.Collections;
using System.Runtime.InteropServices;

public class HeadController : MonoBehaviour {
    private static AndroidJavaObject mAndroidHeadPlugin = null;

	private Quaternion mSyncOrientationTracker;
	private Vector3 mSyncTranslationTracker;
	private Quaternion mSyncOrientationSensor;
	private Vector3 mSyncTranslationSensor;

	private int TimeWarpViewNumber = 0;

	private int flag_setOrientation=0;

	[DllImport("OculusPlugin")]
	private static extern bool OVR_GetSensorState( bool monoscopic,
	                                              ref float w,
	                                              ref float x,
	                                              ref float y,
	                                              ref float z,
	                                              ref float fov,
	                                              ref int viewNumber  );

    void Start() {
		// Initialize an instance of the head plugin with an activity context from the current Unity Player Activity.
        if (RuntimePlatform.Android == Application.platform && null == mAndroidHeadPlugin) {
            using (var activityClass = new AndroidJavaClass("com.unity3d.player.UnityPlayer")) {
                AndroidJavaObject activityContext = activityClass.GetStatic<AndroidJavaObject>("currentActivity");
                if (null != activityContext) {
					using (var headPluginClass = new AndroidJavaClass("com.samsung.dtl.wearableremote.profile.BTReceiverClass")) {
                        if (null != headPluginClass) {
							mAndroidHeadPlugin = headPluginClass.CallStatic<AndroidJavaObject>("newInstance", activityContext);
                        }
                    }
                }
            }
        }
    }

    void Update() {
		if (RuntimePlatform.Android == Application.platform && null != mAndroidHeadPlugin) {
			Vector3 TrackerPos;
			TrackerPos.x = -mAndroidHeadPlugin.Call<float>("getY")/1000.0f;
			TrackerPos.y = mAndroidHeadPlugin.Call<float>("getX")/1000.0f;
			TrackerPos.z = -mAndroidHeadPlugin.Call<float>("getZ")/1000.0f;
			Vector3 TrackerOrt;
			TrackerOrt.x = mAndroidHeadPlugin.Call<float>("getYaw");
			TrackerOrt.y = mAndroidHeadPlugin.Call<float>("getPitch");
			TrackerOrt.z = mAndroidHeadPlugin.Call<float>("getRoll");

			float w = 0, x = 0, y = 0, z = 0;
			float fov = 90.0f;
			//OVRCamera[] ovrcameras = gameObject.GetComponentsInChildren<OVRCamera>( true );

			if( (TrackerOrt.x !=0 || TrackerOrt.y!=0 || TrackerOrt.z!=0) && flag_setOrientation==0){
				// Fetch the latest head orientation for this frame
				OVR_GetSensorState( false, ref w, ref x, ref y, ref z, ref fov, ref TimeWarpViewNumber);
				mSyncOrientationSensor.x = x;
				mSyncOrientationSensor.y = y;
				mSyncOrientationSensor.z = z;
				mSyncOrientationSensor.w = w;

				//mSyncOrientationSensor =  ovrcameras[0].camera.transform.rotation;
				mSyncTranslationSensor = transform.position;

				mSyncOrientationTracker.eulerAngles = TrackerOrt;
				mSyncTranslationTracker = TrackerPos;
				flag_setOrientation=1;
				Debug.Log ("AN: Initial Tracker Ort = " + TrackerOrt.ToString() + " , Pos = " + TrackerPos.ToString());
				Debug.Log ("AN: Initial Sensor  Ort = " + mSyncOrientationSensor.eulerAngles.ToString() + " , Pos = " + mSyncTranslationSensor.ToString());
			}
			if(TrackerOrt.x==0 && TrackerOrt.y==0 && TrackerOrt.z==0)flag_setOrientation=0;

			Quaternion ortSensor = Quaternion.identity;
			w = 0;
			x = 0;
			y = 0;
			z = 0;
			fov = 90.0f;
			OVR_GetSensorState( false, ref w, ref x, ref y, ref z, ref fov, ref TimeWarpViewNumber);
			ortSensor.x = x;
			ortSensor.y = y;
			ortSensor.z = z;
			ortSensor.w = w;

			//ortSensor = ovrcameras[0].camera.transform.rotation;

			Quaternion orientationTransfer = Quaternion.FromToRotation(mSyncOrientationSensor.eulerAngles,mSyncOrientationTracker.eulerAngles);

			Quaternion tempQ = Quaternion.identity;
			tempQ.eulerAngles = new Vector3(0,0,0);
			//transform.localRotation = new Quaternion(tempQ.x,tempQ.y,tempQ.z,tempQ.w);

			Vector3 Translation = (TrackerPos-mSyncTranslationTracker);
			//Translation.y += 1.67f;
			if(!name.Contains("Root")){
				Translation.y += 1.67f;
				transform.localRotation = ortSensor;
				transform.rotation = ortSensor;
			}else{
				Translation.y += 1.12f;
			}

			transform.position = Translation;


			//Debug.Log ("AN: Translation = "+Translation.ToString()+" Ort transfer = "+orientationTransfer.eulerAngles.ToString());
			//Debug.Log ("AN: coordinates Pos = "+transform.position.ToString()+" Ort = "+transform.localRotation.eulerAngles.ToString());
			//Debug.Log ("AN: Senosr = "+ ortSensor.eulerAngles.ToString());
			//Debug.Log ("AN: transform R= "+transform.rotation.eulerAngles.ToString()+" lR="+transform.localRotation.eulerAngles.ToString()+" T="+transform.position.ToString() + " lT="+transform.localPosition.ToString());

        }
    }

    void OnDestroy() {
		// Destruct the head plugin.
		if (RuntimePlatform.Android == Application.platform && null != mAndroidHeadPlugin) {
			mAndroidHeadPlugin.Call("onDestroy");
			mAndroidHeadPlugin = null;
        }
    }
}