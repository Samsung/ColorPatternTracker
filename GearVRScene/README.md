This Unity based apk receives the XYZ coordinates from ColorPatternTracker and enables motion tracking in the rendered scene via bluetooth.

In order to compile this VR apk, perform the following steps:

1.  Install unity 5.3.4f1 available at [https://unity3d.com/get-unity/download](https://unity3d.com/get-unity/download).

2.  Open the project and go to File>Build Settings>Player
    Settings>Publishing settings

3.  Sign the application with an Oculus Signature File that is specific to your device.  [ See this page for more details on the Oculus Signature File: https://developer3.oculus.com/documentation/mobilesdk/latest/concepts/mobile-submission-sig-file/ ]

	3.1 Create an Oculus Signature file following the instructions on this webpage: https://dashboard.oculus.com/tools/osig-generator/

	3.2. Place the Oculus Signature file generated in step 1 in the Unity project in the  \<UnityProject\>/Assets/Plugins/Android/assets/ directory.
4. Build the Unity project for the Android platform.

5.  Click build to create the apk.