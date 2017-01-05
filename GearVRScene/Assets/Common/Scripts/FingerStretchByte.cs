using UnityEngine;
using System.Collections;

public class FingerStretchByte : MonoBehaviour {
    private static AndroidJavaObject mAndroidGloveIfPlugin = null;
    public int jointIndex = 0;
    public int handIndex = 0;
    private int jI = 0;
    private float lastNormalizedJointValue = 0f;

    static private int MAX_JOINTS_PER_HAND = 7;
    static private int JOINT_COUNT = 2 * MAX_JOINTS_PER_HAND;
    // The HISTORY_COUNT is the number of samples to average.  The minimum value of 1
    // means only one value will be used to update the on-screen response - this case
    // would have the most responsive, but also likely most jerky motion.  The more
    // values that are averaged, the smoother the motion becomes, but this will also
    // introduce more lag between input and observed change of position.
    static private int HISTORY_COUNT = 9;
    static float[] openAngles = new float[JOINT_COUNT];
    static float[] closedAngles = new float[JOINT_COUNT];

    static private float MIN_VALID_SIGNAL_THRESHOLD = 0.01f;
    static float[] recentStretchAverage = new float[JOINT_COUNT];
    static float[] recentAverageLo = new float[JOINT_COUNT];
    static float[] recentAverageHi = new float[JOINT_COUNT];

    static float[] baseJointAngle = new float[JOINT_COUNT];
    private float[] jointValueHistory = new float[HISTORY_COUNT];
    private int historyIndex = 0;

    static private int HAND_LEFT = 0;
    static private int HAND_RIGHT = 1;

    private static int LEFT_THUMB_CHANNEL = 6;
    private static int LEFT_FIRST_CHANNEL = 4;
    private static int LEFT_MIDDLE_CHANNEL = 5;
    private static int LEFT_RING_CHANNEL = 0;
    private static int LEFT_PINKY_CHANNEL = 1;

    private static int RIGHT_THUMB_CHANNEL = MAX_JOINTS_PER_HAND + 1;
    private static int RIGHT_FIRST_CHANNEL = MAX_JOINTS_PER_HAND + 0;
    private static int RIGHT_MIDDLE_CHANNEL = MAX_JOINTS_PER_HAND + 5;
    private static int RIGHT_RING_CHANNEL = MAX_JOINTS_PER_HAND + 4;
    private static int RIGHT_PINKY_CHANNEL = MAX_JOINTS_PER_HAND + 6;

    static private float GUI_TEXT_Y_MIN = 0.1f;
    static private float GUI_TEXT_Y_RANGE = 0.8f;

    private GUIText rawHexGuiText;
    private GUIText leftThumbGuiText;
    private GUIText leftFirstGuiText;
    private GUIText leftMiddleGuiText;
    private GUIText leftRingGuiText;
    private GUIText leftPinkyGuiText;
    private GUIText leftThumbGuiTextLo;
    private GUIText leftFirstGuiTextLo;
    private GUIText leftMiddleGuiTextLo;
    private GUIText leftRingGuiTextLo;
    private GUIText leftPinkyGuiTextLo;
    private GUIText leftThumbGuiTextHi;
    private GUIText leftFirstGuiTextHi;
    private GUIText leftMiddleGuiTextHi;
    private GUIText leftRingGuiTextHi;
    private GUIText leftPinkyGuiTextHi;
    private GUIText rightThumbGuiText;
    private GUIText rightFirstGuiText;
    private GUIText rightMiddleGuiText;
    private GUIText rightRingGuiText;
    private GUIText rightPinkyGuiText;
    private GUIText rightThumbGuiTextLo;
    private GUIText rightFirstGuiTextLo;
    private GUIText rightMiddleGuiTextLo;
    private GUIText rightRingGuiTextLo;
    private GUIText rightPinkyGuiTextLo;
    private GUIText rightThumbGuiTextHi;
    private GUIText rightFirstGuiTextHi;
    private GUIText rightMiddleGuiTextHi;
    private GUIText rightRingGuiTextHi;
    private GUIText rightPinkyGuiTextHi;

    void Start() {
        // NOTE: The setting of jI maps joint indicies 1 (thumb), 3 (index), and 5 (middle)
        // - which are used to get stretch values from the current revision of glove hardware -
        // to the indices of the local arrays: 0, 1, and 2.
        //jI = (jointIndex - 1) / 2;
        jI = jointIndex + MAX_JOINTS_PER_HAND * handIndex;

        rawHexGuiText = GameObject.Find("RawHexGuiText").GetComponent<GUIText>();
        leftThumbGuiText = GameObject.Find("LeftThumbGuiText").GetComponent<GUIText>();
        leftFirstGuiText = GameObject.Find("LeftFirstGuiText").GetComponent<GUIText>();
        leftMiddleGuiText = GameObject.Find("LeftMiddleGuiText").GetComponent<GUIText>();
        leftRingGuiText = GameObject.Find("LeftRingGuiText").GetComponent<GUIText>();
        leftPinkyGuiText = GameObject.Find("LeftPinkyGuiText").GetComponent<GUIText>();
        leftThumbGuiTextLo = GameObject.Find("LeftThumbGuiTextLow").GetComponent<GUIText>();
        leftFirstGuiTextLo = GameObject.Find("LeftFirstGuiTextLow").GetComponent<GUIText>();
        leftMiddleGuiTextLo = GameObject.Find("LeftMiddleGuiTextLow").GetComponent<GUIText>();
        leftRingGuiTextLo = GameObject.Find("LeftRingGuiTextLow").GetComponent<GUIText>();
        leftPinkyGuiTextLo = GameObject.Find("LeftPinkyGuiTextLow").GetComponent<GUIText>();
        leftThumbGuiTextHi = GameObject.Find("LeftThumbGuiTextHigh").GetComponent<GUIText>();
        leftFirstGuiTextHi = GameObject.Find("LeftFirstGuiTextHigh").GetComponent<GUIText>();
        leftMiddleGuiTextHi = GameObject.Find("LeftMiddleGuiTextHigh").GetComponent<GUIText>();
        leftRingGuiTextHi = GameObject.Find("LeftRingGuiTextHigh").GetComponent<GUIText>();
        leftPinkyGuiTextHi = GameObject.Find("LeftPinkyGuiTextHigh").GetComponent<GUIText>();

        rightThumbGuiText = GameObject.Find("RightThumbGuiText").GetComponent<GUIText>();
        rightFirstGuiText = GameObject.Find("RightFirstGuiText").GetComponent<GUIText>();
        rightMiddleGuiText = GameObject.Find("RightMiddleGuiText").GetComponent<GUIText>();
        rightRingGuiText = GameObject.Find("RightRingGuiText").GetComponent<GUIText>();
        rightPinkyGuiText = GameObject.Find("RightPinkyGuiText").GetComponent<GUIText>();
        rightThumbGuiTextLo = GameObject.Find("RightThumbGuiTextLow").GetComponent<GUIText>();
        rightFirstGuiTextLo = GameObject.Find("RightFirstGuiTextLow").GetComponent<GUIText>();
        rightMiddleGuiTextLo = GameObject.Find("RightMiddleGuiTextLow").GetComponent<GUIText>();
        rightRingGuiTextLo = GameObject.Find("RightRingGuiTextLow").GetComponent<GUIText>();
        rightPinkyGuiTextLo = GameObject.Find("RightPinkyGuiTextLow").GetComponent<GUIText>();
        rightThumbGuiTextHi = GameObject.Find("RightThumbGuiTextHigh").GetComponent<GUIText>();
        rightFirstGuiTextHi = GameObject.Find("RightFirstGuiTextHigh").GetComponent<GUIText>();
        rightMiddleGuiTextHi = GameObject.Find("RightMiddleGuiTextHigh").GetComponent<GUIText>();
        rightRingGuiTextHi = GameObject.Find("RightRingGuiTextHigh").GetComponent<GUIText>();
        rightPinkyGuiTextHi = GameObject.Find("RightPinkyGuiTextHigh").GetComponent<GUIText>();

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
            float stretch = 0;
            if (HAND_LEFT == handIndex) {
                stretch = mAndroidGloveIfPlugin.Call<float>("getLeftStretch", jointIndex);
            } else if (HAND_RIGHT == handIndex) {
                stretch = mAndroidGloveIfPlugin.Call<float>("getRightStretch", jointIndex);
            }
            Debug.Log("JRM: hand = " + handIndex + " stretch = " + stretch);
            if (0f == stretch) {
                return;
            }
            // Keep the last few recent joint values
            jointValueHistory[historyIndex] = stretch;
            historyIndex++;
            historyIndex %= HISTORY_COUNT;
            recentStretchAverage[jI] = getAverageJointValue();
            // Track least and greatest average value per joint since the last "Back" button calibration
            if (recentStretchAverage[jI] < recentAverageLo[jI] && recentStretchAverage[jI] > MIN_VALID_SIGNAL_THRESHOLD) {
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
            baseJointAngle[jI] = (normalizedJointValue - lastNormalizedJointValue) * 66;
            // Only display once per frame (when traversing a specific node on robot model).
            if (name.Contains("Right_Thumb_Joint_01b")) {
                string allHex = mAndroidGloveIfPlugin.Call<string>("showRawHexData");
                //for (int i = 0; i < JOINT_COUNT; i++) {
                    /*
                    float percent = getNormalizedJointValueForStats(recentStretchAverage[i], i) * 0.9f;

                    // Note: If the console text output is preferred, change the 0.9 value back to 100 to scale to percentages,
                    // and uncomment out this commented block of code that includes printing out "ByteData".
                    string graph = "";
                    for (int k = 0; k < 100; k++) {
                        if (k == (int)percent) {
                            graph += i;
                        } else {
                            if (0 == k % 10) {
                                graph += ".";
                            } else {
                                graph += " ";
                            }
                        }
                    }

                    Debug.Log("ByteData" + i + "[" + graph + "]" +
                              recentStretchAverage[i].ToString(".0000") + "=" +
                              percent.ToString("00") + "% on " + recentAverageLo[i].ToString(".0000") +
                              "-" + recentAverageHi[i].ToString(".0000") + " " + allHex);
                    */

                    // The second parameter (34) accounts for only showing the first 7 channels of data * 4 Hex characters each + separating ':' between each channel.
                    rawHexGuiText.text = allHex.Substring(0, 34);

                    // Update On-screen GUI ranges for each finger
                    leftThumbGuiTextLo.text = recentAverageLo[LEFT_THUMB_CHANNEL].ToString();
                    leftThumbGuiTextHi.text = recentAverageHi[LEFT_THUMB_CHANNEL].ToString();
                    leftFirstGuiTextLo.text = recentAverageLo[LEFT_FIRST_CHANNEL].ToString();
                    leftFirstGuiTextHi.text = recentAverageHi[LEFT_FIRST_CHANNEL].ToString();
                    leftMiddleGuiTextLo.text = recentAverageLo[LEFT_MIDDLE_CHANNEL].ToString();
                    leftMiddleGuiTextHi.text = recentAverageHi[LEFT_MIDDLE_CHANNEL].ToString();
                    leftRingGuiTextLo.text = recentAverageLo[LEFT_RING_CHANNEL].ToString();
                    leftRingGuiTextHi.text = recentAverageHi[LEFT_RING_CHANNEL].ToString();
                    leftPinkyGuiTextLo.text = recentAverageLo[LEFT_PINKY_CHANNEL].ToString();
                    leftPinkyGuiTextHi.text = recentAverageHi[LEFT_PINKY_CHANNEL].ToString();

                    rightThumbGuiTextLo.text = recentAverageLo[RIGHT_THUMB_CHANNEL].ToString();
                    rightThumbGuiTextHi.text = recentAverageHi[RIGHT_THUMB_CHANNEL].ToString();
                    rightFirstGuiTextLo.text = recentAverageLo[RIGHT_FIRST_CHANNEL].ToString();
                    rightFirstGuiTextHi.text = recentAverageHi[RIGHT_FIRST_CHANNEL].ToString();
                    rightMiddleGuiTextLo.text = recentAverageLo[RIGHT_MIDDLE_CHANNEL].ToString();
                    rightMiddleGuiTextHi.text = recentAverageHi[RIGHT_MIDDLE_CHANNEL].ToString();
                    rightRingGuiTextLo.text = recentAverageLo[RIGHT_RING_CHANNEL].ToString();
                    rightRingGuiTextHi.text = recentAverageHi[RIGHT_RING_CHANNEL].ToString();
                    rightPinkyGuiTextLo.text = recentAverageLo[RIGHT_PINKY_CHANNEL].ToString();
                    rightPinkyGuiTextHi.text = recentAverageHi[RIGHT_PINKY_CHANNEL].ToString();

                    leftThumbGuiText.transform.position =
                        new Vector3(0.4f, GUI_TEXT_Y_MIN + GUI_TEXT_Y_RANGE * getNormalizedJointValueForStats(
                            recentStretchAverage[LEFT_THUMB_CHANNEL], LEFT_THUMB_CHANNEL), 0f);
                    leftFirstGuiText.transform.position =
                        new Vector3(0.3f, GUI_TEXT_Y_MIN + GUI_TEXT_Y_RANGE * getNormalizedJointValueForStats(
                            recentStretchAverage[LEFT_FIRST_CHANNEL], LEFT_FIRST_CHANNEL), 0f);
                    leftMiddleGuiText.transform.position =
                        new Vector3(0.2f, GUI_TEXT_Y_MIN + GUI_TEXT_Y_RANGE * getNormalizedJointValueForStats(
                            recentStretchAverage[LEFT_MIDDLE_CHANNEL], LEFT_MIDDLE_CHANNEL), 0f);
                    leftRingGuiText.transform.position =
                        new Vector3(0.1f, GUI_TEXT_Y_MIN + GUI_TEXT_Y_RANGE * getNormalizedJointValueForStats(
                            recentStretchAverage[LEFT_RING_CHANNEL], LEFT_RING_CHANNEL), 0f);
                    leftPinkyGuiText.transform.position =
                        new Vector3(0.0f, GUI_TEXT_Y_MIN + GUI_TEXT_Y_RANGE * getNormalizedJointValueForStats(
                            recentStretchAverage[LEFT_PINKY_CHANNEL], LEFT_PINKY_CHANNEL), 0f);

                    rightThumbGuiText.transform.position =
                        new Vector3(0.5f, GUI_TEXT_Y_MIN + GUI_TEXT_Y_RANGE * getNormalizedJointValueForStats(
                            recentStretchAverage[RIGHT_THUMB_CHANNEL], RIGHT_THUMB_CHANNEL), 0f);
                    rightFirstGuiText.transform.position =
                        new Vector3(0.6f, GUI_TEXT_Y_MIN + GUI_TEXT_Y_RANGE * getNormalizedJointValueForStats(
                            recentStretchAverage[RIGHT_FIRST_CHANNEL], RIGHT_FIRST_CHANNEL), 0f);
                    rightMiddleGuiText.transform.position =
                        new Vector3(0.7f, GUI_TEXT_Y_MIN + GUI_TEXT_Y_RANGE * getNormalizedJointValueForStats(
                            recentStretchAverage[RIGHT_MIDDLE_CHANNEL], RIGHT_MIDDLE_CHANNEL), 0f);
                    rightRingGuiText.transform.position =
                        new Vector3(0.8f, GUI_TEXT_Y_MIN + GUI_TEXT_Y_RANGE * getNormalizedJointValueForStats(
                            recentStretchAverage[RIGHT_RING_CHANNEL], RIGHT_RING_CHANNEL), 0f);
                    rightPinkyGuiText.transform.position =
                        new Vector3(0.9f, GUI_TEXT_Y_MIN + GUI_TEXT_Y_RANGE * getNormalizedJointValueForStats(
                            recentStretchAverage[RIGHT_PINKY_CHANNEL], RIGHT_PINKY_CHANNEL), 0f);
                //}
            }
            // end track and display the results

            transform.Rotate(Vector3.forward, baseJointAngle[jI]);
            lastNormalizedJointValue = normalizedJointValue;
            Debug.Log("FingerStretchByte:value: " + normalizedJointValue + " joint" + jI);
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
