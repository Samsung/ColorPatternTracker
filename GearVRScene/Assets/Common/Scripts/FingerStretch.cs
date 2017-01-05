using UnityEngine;
using System.Collections;

public class FingerStretch : MonoBehaviour {
	private static AndroidJavaObject mAndroidGloveIfPlugin = null;
	public int jointIndex = 0;
	private int jI = 0;
	private float lastNormalizedJointValue = 0f;

	static private int JOINT_COUNT = 3;
	// The HISTORY_COUNT is the number of samples to average.  The minimum value of 1
	// means only one value will be used to update the on-screen response - this case
	// would have the most responsive, but also likely most jerky motion.  The more
	// values that are averaged, the smoother the motion becomes, but this will also
	// introduce more lag between input and observed change of position.
	static private int HISTORY_COUNT = 5;
	static float[] openAngles = new float[JOINT_COUNT];
	static float[] closedAngles = new float[JOINT_COUNT];

	static float[] recentStretchAverage = new float[JOINT_COUNT];
	static float[] recentAverageLo = new float[JOINT_COUNT];
	static float[] recentAverageHi = new float[JOINT_COUNT];
	
	static float[] baseJointAngle = new float[JOINT_COUNT];
	private float[] jointValueHistory = new float[HISTORY_COUNT];
	private int historyIndex = 0;

	void Start() {
		// NOTE: The setting of jI maps joint indicies 1 (thumb), 3 (index), and 5 (middle)
		// - which are used to get stretch values from the current revision of glove hardware -
		// to the indices of the local arrays: 0, 1, and 2.
		jI = (jointIndex - 1) / 2;
		if (RuntimePlatform.Android == Application.platform && null == mAndroidGloveIfPlugin) {
			using (var activityClass = new AndroidJavaClass("com.unity3d.player.UnityPlayer")) {
				AndroidJavaObject activityContext = activityClass.GetStatic<AndroidJavaObject>("currentActivity");
				if (null != activityContext) {
					using (var glovePluginClass = new AndroidJavaClass("com.samsung.wearable.gloveif.GloveIfUnityPlugin")) {
						if (null != glovePluginClass) {
							mAndroidGloveIfPlugin = glovePluginClass.CallStatic<AndroidJavaObject>("newInstance", activityContext);
 							for (int i = 0; i < JOINT_COUNT; i++) {
								openAngles[i] = 0.1f;
								closedAngles[i] = 0.11f;
								jointValueHistory[i] = 0.1f;
								recentAverageLo[i] = 1.0f;
								recentAverageHi[i] = 0.0f;
								recentStretchAverage[i] = 0.5f;
							}
						}
					}
				}
			}
		}
	}

	void Update() {
		if (RuntimePlatform.Android == Application.platform && null != mAndroidGloveIfPlugin) {
			float stretch = mAndroidGloveIfPlugin.Call<float>("getStretch", jointIndex);
			if (0f == stretch) {
				return;
			}
			// Keep the last few recent joint values
			jointValueHistory[historyIndex] = stretch;
			historyIndex++;
			historyIndex %= HISTORY_COUNT;
			recentStretchAverage[jI] = getAverageJointValue();
			// track least and greatest average value per joint since the last "Back" button calibration
			if (recentStretchAverage[jI] < recentAverageLo[jI]) {
				recentAverageLo[jI] = recentStretchAverage[jI];
			}
			if (recentStretchAverage[jI] > recentAverageHi[jI]) {
				recentAverageHi[jI] = recentStretchAverage[jI];
			}

			// Save off joint angles for open and closed hand positions when corresponding "back" key pressed down and let go.
			if (Input.GetKeyDown(KeyCode.Escape)) {
				for (int i = 0; i < JOINT_COUNT; i++) {
					openAngles[i] = recentAverageLo[i];
				}
			}
			if (Input.GetKeyUp(KeyCode.Escape)) {
				for (int i = 0; i < JOINT_COUNT; i++) {
					closedAngles[i] = recentAverageHi[i];
				}

				for (int i = 0; i < JOINT_COUNT; i++) {
					recentAverageLo[i] = 1.0f;
					recentAverageHi[i] = 0.0f;
				}
			}

			// This is where the normalized value can be based on the instantaneous "stretch" value, or, conversely,
			// based on the smoothed "recent average" of stretch values.
			float normalizedJointValue = getNormalizedJointValue(recentStretchAverage[jI]);
			baseJointAngle[jI] = (normalizedJointValue - lastNormalizedJointValue) * 46;

			// Only display once per frame (when traversing a specific node on robot model).
			if (name.Contains("Right_Thumb_Joint_01a")) {
				float thumbNormalized = recentStretchAverage[0];
				float thumbLo = recentAverageLo[0];
				float thumbHi = recentAverageHi[0];
				string thumbHexadecimal = (65536 * thumbNormalized).ToString("X");
				string thumbHexLo = (65536 * thumbLo).ToString("X");
				string thumbHexHi = (65536 * thumbHi).ToString("X");
				float thumbPercent = getNormalizedJointValueForStats(recentStretchAverage[0], 0) * 100;

				float indexNormalized = recentStretchAverage[1];
				float indexLo = recentAverageLo[1];
				float indexHi = recentAverageHi[1];
				string indexHexadecimal = (65536 * indexNormalized).ToString("X");
				string indexHexLo = (65536 * indexLo).ToString("X");
				string indexHexHi = (65536 * indexHi).ToString("X");
				float indexPercent = getNormalizedJointValueForStats(recentStretchAverage[1], 1) * 100;

				float middleNormalized = recentStretchAverage[2];
				float middleLo = recentAverageLo[2];
				float middleHi = recentAverageHi[2];
				string middleHexadecimal = (65536 * middleNormalized).ToString("X");
				string middleHexLo = (65536 * middleLo).ToString("X");
				string middleHexHi = (65536 * middleHi).ToString("X");
				float middlePercent = getNormalizedJointValueForStats(recentStretchAverage[2], 2) * 100;
				/*
				Debug.Log ("AllTime" +
				           "   C0 " + thumbNormalized.ToString(".0000") + "=" + thumbPercent.ToString("00") +
				           "% on " + thumbLo.ToString(".0000") + "-" + thumbHi.ToString(".0000") +
				           " or " + thumbHexadecimal + " on " + thumbHexLo + "-" + thumbHexHi +
				           //" Abs:" + openAngles[0].ToString(".0000") + "-" + closedAngles[0].ToString(".0000") +
				           "   C1 " + indexNormalized.ToString(".0000") + "=" + indexPercent.ToString("00") +
				           "% on " + indexLo.ToString(".0000") + "-" + indexHi.ToString(".0000") +
				           " or " + indexHexadecimal + " on " + indexHexLo + "-" + indexHexHi +
				           //" Abs:" + openAngles[1].ToString(".0000") + "-" + closedAngles[1].ToString(".0000") +
				           "   C2 " + middleNormalized.ToString(".0000") + "=" + middlePercent.ToString("00") +
				           "% on " + middleLo.ToString(".0000") + "-" + middleHi.ToString(".0000") +
				           " or " + middleHexadecimal + " on " + middleHexLo + "-" + middleHexHi); //+
				           //" Abs:" + openAngles[2].ToString(".0000") + "-" + closedAngles[2].ToString(".0000"));
				*/
				string thumbSpaces = "";
				string thumbSpacesEnd = "";
				string indexSpaces = "";
				string indexSpacesEnd = "";
				string middleSpaces = "";
				string middleSpacesEnd = "";
				//string maxSpaces = "         .         .         .         .         .         .         .         .         .         .";
				for (int k = 0; k < 100; k++) {
					if (k < thumbPercent) {
						thumbSpaces += " ";
					} else {
						thumbSpacesEnd += " ";
					}
					if (k < indexPercent) {
						indexSpaces += " ";
					} else {
						indexSpacesEnd += " ";
					}
					if (k < middlePercent) {
						middleSpaces += " ";
					} else {
						middleSpacesEnd += " ";
					}
				}

				Debug.Log ("GraphThumb [" + thumbSpaces + '*' + thumbSpacesEnd + "]" +
				           thumbNormalized.ToString(".0000") + "=" + thumbPercent.ToString("00") +
				           "% on " + thumbLo.ToString(".0000") + "-" + thumbHi.ToString(".0000") +
				           " or " + thumbHexadecimal + " on " + thumbHexLo + "-" + thumbHexHi);
				Debug.Log ("GraphIndex [" + indexSpaces + '|' + indexSpacesEnd + "]" +
				           indexNormalized.ToString(".0000") + "=" + indexPercent.ToString("00") +
				           "% on " + indexLo.ToString(".0000") + "-" + indexHi.ToString(".0000") +
				           " or " + indexHexadecimal + " on " + indexHexLo + "-" + indexHexHi);
				Debug.Log ("GraphMiddle[" + middleSpaces + '0' + middleSpacesEnd + "]" +
				           middleNormalized.ToString(".0000") + "=" + middlePercent.ToString("00") +
				           "% on " + middleLo.ToString(".0000") + "-" + middleHi.ToString(".0000") +
				           " or " + middleHexadecimal + " on " + middleHexLo + "-" + middleHexHi);
			}
			// end track and display the results

			transform.Rotate(Vector3.forward, baseJointAngle[jI]);
			lastNormalizedJointValue = normalizedJointValue;
			Debug.Log("FingerStretch:value: " + normalizedJointValue + " joint" + jI);
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


	float getNormalizedJointValue(float stretch) {
		float denominator = Mathf.Abs(recentAverageHi[jI] - recentAverageLo[jI]);
		if (denominator > 0) {
			return (stretch - recentAverageLo[jI]) / Mathf.Max(denominator);
		} else {
			return 1.0f;
		}
	}
	
	float getNormalizedJointValueForStats(float stretch, int index) {
		float denominator = Mathf.Abs(recentAverageHi[index] - recentAverageLo[index]);
		if (denominator > 0) {
			return (stretch - recentAverageLo[index]) / Mathf.Max(denominator);
		} else {
			return 1.0f;
		}
	}


	// These next two methods are versions of the previous two that use sets of open/closed angles
	// instead of averaged recent low/high values for angles.  Using them in place of the previous
	// methods makes the range of values stick to those seen at the time demarked by the pressing
	// and releasing of the "Back" Android button, as opposed to the range always being adjusted
	// to accomodate new low/high averaged values.
/*
	float getNormalizedJointValue(float stretch) {
		float denominator = Mathf.Abs(closedAngles[jI] - openAngles[jI]);
		if (denominator > 0) {
			return (stretch - openAngles[jI]) / Mathf.Max(denominator);
		} else {
			return 1.0f;
		}
	}

	float getNormalizedJointValueForStats(float stretch, int index) {
		float denominator = Mathf.Abs(closedAngles[index] - openAngles[index]);
		if (denominator > 0) {
			return (stretch - openAngles[index]) / Mathf.Max(denominator);
		} else {
			return 1.0f;
		}
	}
*/

	void OnDestroy() {
		if (RuntimePlatform.Android == Application.platform && null != mAndroidGloveIfPlugin) {
			mAndroidGloveIfPlugin.Call("destroy");
			mAndroidGloveIfPlugin = null;
		}
	}
}
