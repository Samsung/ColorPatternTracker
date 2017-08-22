LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
OPENCV_CAMERA_MODULES:=on
OPENCV_INSTALL_MODULES:=on
include $(LOCAL_PATH)/../../../../../Config/ConfigOpencv.mk

SRC_PATH := src

LOCAL_CFLAGS    += -I$(SRC_PATH) -g -Wno-deprecated-declarations
LOCAL_CFLAGS    += -DSHOW_REFERENCE_PROGRESS=1
LOCAL_MODULE    := cgt
LOCAL_SRC_FILES := cgt.cpp \
	$(SRC_PATH)/util.cpp \
	$(SRC_PATH)/CLManager.cpp \
	$(SRC_PATH)/CLTracker.cpp

LOCAL_LDLIBS := -landroid -llog -ljnigraphics $(LOCAL_PATH)/libs/gs7/libOpenCL.so $(LOCAL_PATH)/libs/gs7/libEGL.so
LOCAL_STATIC_LIBRARIES := android_native_app_glue

include $(BUILD_SHARED_LIBRARY)
$(call import-module,android/native_app_glue)
