using UnityEngine;
using System.Runtime.InteropServices;

public class ViewpointController : MonoBehaviour
{
    private static AndroidJavaObject mAndroidHeadPlugin = null;

    private Quaternion mSyncOrientationTracker;
    private Vector3 mSyncTranslationTracker;
    private Quaternion mSyncOrientationSensor;
    private Vector3 mSyncTranslationSensor;
    private TextMesh mDebugText;

    Vector3 mTrackerPos;
    Vector3 mTrackerOrt;

    [DllImport("OculusPlugin")]
    private static extern bool OVR_GetSensorState(bool monoscopic,
                                                  ref float w,
                                                  ref float x,
                                                  ref float y,
                                                  ref float z,
                                                  ref float fov,
                                                  ref int viewNumber);

	void Start()
    {
        mDebugText = GameObject.Find("DebugText").GetComponent<TextMesh>();

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

	void Update()
    {
        if (RuntimePlatform.Android == Application.platform && null != mAndroidHeadPlugin)
        {
            mTrackerPos.x = -mAndroidHeadPlugin.Call<float>("getY") / 10.0f;
            mTrackerPos.y = mAndroidHeadPlugin.Call<float>("getX") / 10.0f;
            mTrackerPos.z = -mAndroidHeadPlugin.Call<float>("getZ") / 10.0f;

            mTrackerOrt.x = mAndroidHeadPlugin.Call<float>("getYaw");
            mTrackerOrt.y = mAndroidHeadPlugin.Call<float>("getPitch");
            mTrackerOrt.z = mAndroidHeadPlugin.Call<float>("getRoll");
                        
            Transform root = transform.Find("TrackingSpace");
            Debug.LogError("TrackerPos:[" + mTrackerPos.x + "," + mTrackerPos.y + "," + mTrackerPos.z +
                "] Root:[" + root.position.x + "," + root.position.y + "," + root.position.z + "]");
            mDebugText.text = "[" + mTrackerPos.x + "," + mTrackerPos.y + "," + mTrackerPos.z + "]";
            root.position = mTrackerPos;
        }
	}
}
