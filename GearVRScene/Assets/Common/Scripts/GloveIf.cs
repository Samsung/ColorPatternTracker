using UnityEngine;
using System.Collections;

public class GloveIf : MonoBehaviour {
	private static AndroidJavaObject mAndroidGloveIfPlugin = null;
	public int jointIndex = 0;

	private int historyIndex = 0;
	private static int HISTORY_COUNT = 5;
	private static int AXIS_COUNT = 3;
	private static float[] recentAccelerometerAxisAverage = new float[AXIS_COUNT];
	private float[] jointValueHistory = new float[HISTORY_COUNT * AXIS_COUNT];

	void Start () {
		if (RuntimePlatform.Android == Application.platform && null == mAndroidGloveIfPlugin) {
			using (var activityClass = new AndroidJavaClass("com.unity3d.player.UnityPlayer")) {
				AndroidJavaObject activityContext = activityClass.GetStatic<AndroidJavaObject>("currentActivity");
				if (null != activityContext) {
					using (var glovePluginClass = new AndroidJavaClass("com.samsung.wearable.gloveif.GloveIfUnityPlugin")) {
						if (null != glovePluginClass) {
							mAndroidGloveIfPlugin = glovePluginClass.CallStatic<AndroidJavaObject>("newInstance", activityContext);
						}
					}
				}
			}
		}
	}

	void OnDestroy() {
		if (RuntimePlatform.Android == Application.platform && null != mAndroidGloveIfPlugin) {
			mAndroidGloveIfPlugin.Call("destroy");
			mAndroidGloveIfPlugin = null;
		}
	}

	void Update () {
		if (RuntimePlatform.Android == Application.platform && null != mAndroidGloveIfPlugin) {
			Vector3[] rotationAxis = new Vector3[AXIS_COUNT];
			rotationAxis[0] = Vector3.right;
			rotationAxis[1] = Vector3.up;
			rotationAxis[2] = Vector3.forward;
			for (int i = 0; i < AXIS_COUNT; i++) {
				float value = mAndroidGloveIfPlugin.Call<float>("getAccelerometer", i);
				// Keep the last few recent joint values
				jointValueHistory[AXIS_COUNT * historyIndex + i] = value;

				Debug.Log("accelerometer value from joint " + i + " changed to " + getAverageJointValue(i));
				transform.Rotate(rotationAxis[i], 100 * (getAverageJointValue(i) - recentAccelerometerAxisAverage[i]));
				recentAccelerometerAxisAverage[i] = getAverageJointValue(i);
			}
			historyIndex++;
			historyIndex %= HISTORY_COUNT;
		}
	}

	float getAverageJointValue(int jIndex) {
		float average = 0;
		for (int historyIndex = 0; historyIndex < HISTORY_COUNT; historyIndex++) {
			average += jointValueHistory[AXIS_COUNT * historyIndex + jIndex];
		}
		average /= (float)HISTORY_COUNT;
		return average;
	}
}
