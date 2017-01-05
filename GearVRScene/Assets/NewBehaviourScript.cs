using UnityEngine;
using System;
using System.Collections;
using System.Runtime.InteropServices;

public class NewBehaviourScript : MonoBehaviour {
    private static AndroidJavaObject mAndroidHeadPlugin = null;

    private Quaternion mSyncOrientationTracker;
    private Vector3 mSyncTranslationTracker;
    private Quaternion mSyncOrientationSensor;
    private Vector3 mSyncTranslationSensor;
    private TextMesh mDebugText;
    
    [DllImport("OculusPlugin")]
    private static extern bool OVR_GetSensorState(bool monoscopic,
                                                  ref float w,
                                                  ref float x,
                                                  ref float y,
                                                  ref float z,
                                                  ref float fov,
                                                  ref int viewNumber);
	// Use this for initialization
	void Start () {
        Debug.LogError("Starting Tracking 1");
        mDebugText = GameObject.Find("DebugText").GetComponent<TextMesh>();

        Debug.LogError("Starting Tracking 2");
        // Initialize an instance of the head plugin with an activity context from the current Unity Player Activity.
        if (RuntimePlatform.Android == Application.platform && null == mAndroidHeadPlugin)
        {
            using (var activityClass = new AndroidJavaClass("com.unity3d.player.UnityPlayer"))
            {
                AndroidJavaObject activityContext = activityClass.GetStatic<AndroidJavaObject>("currentActivity");
                if (null != activityContext)
                {
                    using (var headPluginClass = new AndroidJavaClass("com.samsung.dtl.wearableremote.profile.BTReceiverClass"))
                    {
                        if (null != headPluginClass)
                        {
                            mAndroidHeadPlugin = headPluginClass.CallStatic<AndroidJavaObject>("newInstance", activityContext);
                        }
                    }
                }
            }
        }
	}
	
	// Update is called once per frame
	void Update () {
        if (RuntimePlatform.Android == Application.platform && null != mAndroidHeadPlugin)
        {
            Vector3 TrackerPos;
            TrackerPos.x = -mAndroidHeadPlugin.Call<float>("getY")/100;
            TrackerPos.y = mAndroidHeadPlugin.Call<float>("getX")/100+1.97f;
            TrackerPos.z = -mAndroidHeadPlugin.Call<float>("getZ")/100;
            Vector3 TrackerOrt;
            TrackerOrt.x = mAndroidHeadPlugin.Call<float>("getYaw");
            TrackerOrt.y = mAndroidHeadPlugin.Call<float>("getPitch");
            TrackerOrt.z = mAndroidHeadPlugin.Call<float>("getRoll");
                        
            Transform root = transform.Find("TrackingSpace");
            Debug.LogError("TrackerPos:[" + TrackerPos.x + "," + TrackerPos.y + "," + TrackerPos.z + "] Root:["+root.position.x+","+root.position.y+","+root.position.z+"]");
            mDebugText.text = "[" + TrackerPos.x + "," + TrackerPos.y + "," + TrackerPos.z + "]";
            root.position = TrackerPos;
        }
	}
}
