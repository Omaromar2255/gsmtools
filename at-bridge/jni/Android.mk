# Android.mk - GsmTools AT Bridge (v4.0)
# ndk-build usage:
#   ndk-build NDK_PROJECT_PATH=.. APP_BUILD_SCRIPT=jni/Android.mk NDK_APPLICATION_MK=jni/Application.mk

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE     := AT
LOCAL_SRC_FILES  := at_bridge.c

# Fully static - no runtime library dependencies on the device
LOCAL_LDFLAGS    := -static
LOCAL_CFLAGS     := -O2 -Wall -Wextra -DNDEBUG
LOCAL_LDLIBS     := -ldl

# no STL needed
LOCAL_STL        := none

include $(BUILD_EXECUTABLE)
