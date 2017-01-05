using UnityEngine;
using System.Collections;

public class FingerStretchAccelerometer : MonoBehaviour {
    private static AndroidJavaObject mAndroidGloveIfPlugin = null;
    public int jointIndex = 0;
	private float lastNormalizedJointValue = 0f;
	static private int JOINT_COUNT = 5;
	static private int FINGER_COUNT = 5;
	static private int HISTORY_COUNT = 5;
	static float[] openAngles = new float[JOINT_COUNT];
	static float[] closedAngles = new float[JOINT_COUNT];
	static float[] baseJointAngle = new float[FINGER_COUNT];
	private float[] jointValueHistory = new float[HISTORY_COUNT];
	private int historyIndex = 0;
	private int carpalLevel = 0;

	void Start () {
		if (RuntimePlatform.Android == Application.platform && null == mAndroidGloveIfPlugin) {
			using (var activityClass = new AndroidJavaClass("com.unity3d.player.UnityPlayer")) {
				AndroidJavaObject activityContext = activityClass.GetStatic<AndroidJavaObject>("currentActivity");
				if (null != activityContext) {
					using (var glovePluginClass = new AndroidJavaClass("com.samsung.wearable.gloveif.GloveIfUnityPlugin")) {
						if (null != glovePluginClass) {
						    mAndroidGloveIfPlugin = glovePluginClass.CallStatic<AndroidJavaObject>("newInstance", activityContext);
 							for (int i = 0; i < JOINT_COUNT; i++) {
								openAngles[i] = 0.04f;
								closedAngles[i] = 0.08f;
								jointValueHistory[i] = 0.1f;
							}
						}
					}
				}
			}
		}

		// Calculate carpalLevel for more extreme metacarpal joints.
		if (name.Contains("01b")) {
			carpalLevel = 1;
		} else if (name.Contains("01c")) {
			carpalLevel = 2;
		}
	}

	void Update () {
		if (RuntimePlatform.Android == Application.platform && null != mAndroidGloveIfPlugin) {
			if (0 == carpalLevel) {
				float acceleration = mAndroidGloveIfPlugin.Call<float>("getAccelerometer", jointIndex);
				// Keep the last few recent joint values
				jointValueHistory[historyIndex] = acceleration;
				historyIndex++;
				historyIndex %= HISTORY_COUNT;

				// Save off joint angles for open and closed hand positions when corresponding "back" key pressed down and let go.
				if (Input.GetKeyDown(KeyCode.Escape)) {
					openAngles[jointIndex] = getAverageJointValue();
					Debug.Log ("FingerStretchAccelerometer: Open angles triggered. openAnglesJoint[" + jointIndex + "] = " + openAngles[jointIndex]);
				}
				if (Input.GetKeyUp(KeyCode.Escape)) {
					// TODO: remove the "+ 0.02f" when an alternate event is identified to trigger input of "closed hand" state.
					closedAngles[jointIndex] = getAverageJointValue() + 0.02f;
					Debug.Log ("FingerStretchAccelerometer: Closed angles triggered. closedAnglesJoint[" + jointIndex + "] = " + closedAngles[jointIndex]);
				}

				baseJointAngle[jointIndex] = (getNormalizedJointValue(acceleration) - lastNormalizedJointValue) * 25;
				transform.Rotate(Vector3.forward, baseJointAngle[jointIndex]);
				lastNormalizedJointValue = getNormalizedJointValue(acceleration);
				Debug.Log("FingerStretchAccelerometer: value: " + getNormalizedJointValue(acceleration) + " from joint " + jointIndex);
			} else {
				Debug.Log("FingerStretchAccelerometer: with carpalLevel value: " + baseJointAngle[jointIndex - 5 * carpalLevel] + " from joint " + jointIndex);
				transform.Rotate(Vector3.forward, baseJointAngle[jointIndex - 5 * carpalLevel]);
			}
		}
	}

	float getAverageJointValue() {
		float average = 0;
		for (int i = 0; i < HISTORY_COUNT; i++) {
			average += jointValueHistory[i];
		}
		average /= (float)HISTORY_COUNT;
		return average;
	}

	float getNormalizedJointValue(float acceleration) {
		return (acceleration - openAngles[jointIndex]) / (closedAngles[jointIndex] - openAngles[jointIndex]);
	}

	void OnDestroy() {
		if (RuntimePlatform.Android == Application.platform && null != mAndroidGloveIfPlugin) {
			mAndroidGloveIfPlugin.Call("destroy");
			mAndroidGloveIfPlugin = null;
		}
	}
}
