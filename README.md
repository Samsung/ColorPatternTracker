Color Pattern Tracker: User Guide
==========

Virtual reality, or VR, is the newest and potentially the most efficient
dimension of media. One of the important components of creating a
virtual experience is simulating the vision. And for simulating vision,
one should know the user’s field of view at any given time. The field of
view is in turn determined by the position and orientation of user’s
head. Only a few companies have provided some solutions to the head tracking
problem.

System overview
===============

Here, we describe a cost efficient solution to head tracking. It only
requires a color-printed sheet of paper and a commodity smartphone. In
the proposed system, a printed pattern is tracked using the smartphone
camera to estimate its position and orientation. The estimated position
and orientation are then sent back to the headset via bluetooth. The
headset in turn renders a virtual scene from a desired viewpoint
relative to the user’s head. High efficiency and accuracy are two of the
main characteristics of our system that makes it viable.

The main components of the proposed systems are

1.  Pattern design

2.  Detector/tracker design

3.  Multiple pattern registration


Pattern design
--------------

The six patterns used in our system are shown in the figure
above. Note that the only difference between the different
patterns is on their border. Different patterns have different grid
blocks with black outer border and this difference is what is used to
identify different patterns. These borders are specifically placed with
good error tolerance using error correcting codes.

The color grid pattern itself is designed by exhaustively searching for
all possible combinations of red, green and blue colors on a grid of
size 4 x 4 for quick and accurate detection.

| <img src="https://raw.githubusercontent.com/Samsung/ColorPatternTracker/master/Documents/images/pattern_idNewSel5.bmp" alt="Pattern 2" width="140px" height="100px">  | <img src="https://raw.githubusercontent.com/Samsung/ColorPatternTracker/master/Documents/images/pattern_idNewSel6.bmp" alt="Pattern 1" width="140px" height="100px"> | <img src="https://raw.githubusercontent.com/Samsung/ColorPatternTracker/master/Documents/images/pattern_idNewSel9.bmp" alt="Pattern 0" width="140px" height="100px">|
| ------------- | ------------- | -------|
| Pattern 0  | Pattern 1  | Pattern 2|
| <img src="https://raw.githubusercontent.com/Samsung/ColorPatternTracker/master/Documents/images/pattern_idNewSel10.bmp" alt="Pattern 2" width="140px" height="100px">  | <img src="https://raw.githubusercontent.com/Samsung/ColorPatternTracker/master/Documents/images/pattern_idNewSel13.bmp" alt="Pattern 1" width="140px" height="100px"> | <img src="https://raw.githubusercontent.com/Samsung/ColorPatternTracker/master/Documents/images/pattern_idNewSel14.bmp" alt="Pattern 0" width="140px" height="100px">|
| Pattern 3  | Pattern 4  | Pattern 5|

Detector/Tracker design
-----------------------

The pattern is detected by fast detection of corners in the image which
contain all the three colors used e.g. red, green and blue. It uses a
highly optimized coarse to fine algorithm implementation for precise
detection of corners. Note that all the 9 interior corners of the grid
pattern have all the three colors surrounding them.

We avoid running (relatively) expensive pattern detection algorithm in
each frame of video captured by the camera by predicting the position of
the corners in the next frame and then confining the search for corners
to a small region. The well known Kalman filter is also used to smoothen
the pattern trajectory.

Multiple pattern registration
-----------------------------

In order to be able to track the user’s head with a single camera,
multiple patterns are required to be placed on the headset in case one
of the patterns is highly oblique to the camera. To achieve this,
relative position and orientation of different patterns is estimated.

Detailed description
====================

A paper providing the algorithm description and theoretical underpinnings is available [here](https://drive.google.com/file/d/0B9ib55eGTbQCRmptcXNuQW1oSkk/view?usp=sharing). The doxygen documentation generated from this project is available [here](https://rawgit.com/Samsung/ColorPatternTracker/master/PatternTracker/doxygen/html/index.html).

Operation
=========

In this Section we shall describe the procedure to operate the system
with varying degree of control on the system. First we shall describe
the simplest way to run the system using the provided .apk files.

Following are the minimum set of items required to operate the proposed
system:

1.  Tracker phone: Galaxy S6

2.  VR headset: GearVR with a smartphone (VR phone)

3.  Printed patterns: Cardboard/foamboard cutouts with patterns pasted

As more control is required on the system, additional software will be
required.

<img src="https://raw.githubusercontent.com/Samsung/ColorPatternTracker/master/Documents/images/sideVRH.jpg" alt="VR Headset" width="800px" height="440px">   
VR Headset

<img src="https://raw.githubusercontent.com/Samsung/ColorPatternTracker/master/Documents/images/trackerPhone.jpg" alt="Tracker phone" width="800px" height="440px">   
Tracker phone

Direct installation
-------------------
Following are the instructions to directly operate the system without
installation:

1.  Install the Tracker apk \[PatternTracker.apk\] available in the "Executables" folder  on the Tracker phone.

2.  Install VR apk \[TrackerGearVR.apk\] available in the "Executables" folder on a the VR phone.

3.  Pair Tracker phone and VR phone via bluetooth.

4.  Print the six patterns shown in the figure above as described
    in “Pattern printing instructions" below

5.  Attach the Pattern 0 on the front side of VR headset and any number
    of the remaining patterns on other parts of the VR headset. See
    figure above.

6.  Start the PatternTracker.apk on Tracker phone. Place this phone
    horizontally so that the “Debug:on" button is on top left corner and
    the back-facing camera facing towards the user. See Figure
    above. Place it appropriately so that the
    GearVR is comfortably in the field of view.

7.  Start the app on the VR phone and put it in GearVR.

8.  Optionally, register the multiple patterns. See details below.

9.  Optionally, click the “Debug:on" button to turn it to “Debug:off"
    which will enhance the performance of the system but will stop
    showing the corner indicator.

10. Wear GearVR and try to move back and forth, and left and right to
    notice that your view in VR is adjusting according to your motion.


<img src="https://raw.githubusercontent.com/Samsung/ColorPatternTracker/master/Documents/images/updateGeomOn.jpg" alt="update geometry on" width="800px" height="440px">   
Tracker phone with “update geometry" feature on.

<img src="https://raw.githubusercontent.com/Samsung/ColorPatternTracker/master/Documents/images/updateGeomOff.jpg" alt="update geometry off" width="800px" height="440px">     
Tracker phone with “update geometry" feature off.

Following are the instructions for registering multiple patterns:

1.  Click the “Update Geom: off" button so that it show “Update Geom:
    on"

2.  Place the camera of the Tracker phone so that the Pattern 0 and one
    of the other patterns are in view as seen on its screen and capture
    good portion -more than half- of the screen. Make sure the lighting
    is good.

3.  See the corner indicators appear on the screen.

4.  Once the corner indicators are stable i.e. not coming on and going
    off frequently, press the “Update Geom: on” button again to turn it
    into “Update Geom: off". Notice that below the buttons, each new
    detected pattern will be listed. This shows that the new pattern has
    been registered with the assumption that Pattern 0 was
    already registered. See figure above where
    Pattern 3 is registered with Pattern 0.

5.  Follow steps 1 through 4 for registering each new pattern given the
    already registered patterns. E.g. if in the last step Pattern 1 was
    registered, in order to register Pattern 2 place the Tracker phone so
    that either Pattern 0 and Pattern 2 are in view or Pattern 1 and
    Pattern 2 are in view and then follow steps 2 through 3.

Pattern printing instructions on windows machine:

1.  Open a pattern image among those available at PatternDesign\patterns in windows photo viewer;

2.  Print image in letter paper, 3.5x5in, four images in a page,
    un-check “fit to frame”. Size of printed pattern should be 10.6cm
    wide and 7.1cm high. See figure above.

<img src="https://raw.githubusercontent.com/Samsung/ColorPatternTracker/master/Documents/images/patternSize.jpg" alt="update geometry off" width="800px" height="440px">   
Printed pattern with a measurement scale.

In case the system crashes, one reason is that the opengl libraries on
phone are incompatible with the ones provided with the system. To
resolve this issue, the Tracker phone apk will need to be compiled again
as described in Section below.

Project compilation
-------------------

First, we shall describe the compilation of Tracker apk. The compilation
requires latest android studio with latest Android SDK and NDK, available at https://developer.android.com/studio/index.html, and OpenCV 2.4.9 for Android, available at http://opencv.org/downloads.html.

To compile the project, 

1. Open PatternTracker folder in Android Studio using File>Open and then pressing Run> Run 'app'.

2. Edit PatternTracker\app\src\main\cpp\Android.mk file and modify the line “include C:/.../OpenCV.mk" to provide the directory where Android OpenCV is installed.

3. Press Build>Rebuild project.

4. Connect your mobile phone and press Run> Run 'app'


If it does not run, libEGL.so and libOpenCL.so will need to be extracted
from the phone and placed at appropriate location. The libEGL.so file
typically resides in the /system/lib/ directory of the smartphone and
libOpenCL.so is typically found in /system/vendor/lib/ directory. The
following commands on windows command prompt can be used to achieve this
task:   

>\>adb shell  
>\>cd storage/sdcard0  
>\>cp /system/lib/libEGL.so .  
>\>cp /system/vendor/lib/libOpenCL.so .  

Then you can copy the two library files from the windows explorer to “PatternTracker\app\src\main\cpp\libs\gs6". It should now be ready to run.

In order to compile the VR apk, perform the following steps:

1.  Install unity 5.3.4f1 available at [https://unity3d.com/get-unity/download](https://unity3d.com/get-unity/download).

2.  Open the project and go to File>Build Settings>Player
    Settings>Publishing settings

3.  Sign the application with an Oculus Signature File that is specific to your device.  [ See this page for more details on the Oculus Signature File: https://developer3.oculus.com/documentation/mobilesdk/latest/concepts/mobile-submission-sig-file/ ]

	3.1 Create an Oculus Signature file following the instructions on this webpage: https://dashboard.oculus.com/tools/osig-generator/

	3.2. Place the Oculus Signature file generated in step 1 in the Unity project in the  \<UnityProject\>/Assets/Plugins/Android/assets/ directory.
4. Build the Unity project for the Android platform.

5.  Click build to create the apk.

Once both the VR apk and Tracker apk are built, follow the steps
described in Direct Installation section to run the system.

By: Abhishek Nagar   
	Samsung Electronics America, Dallas   
	[a.nagar@samsung.com](mailto:a.nagar@samsung.com)   
	[https://sites.google.com/site/nagarabh/](https://sites.google.com/site/nagarabh/)
