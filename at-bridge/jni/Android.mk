# Android.mk - GsmTools AT Bridge
# ndk-build NDK_PROJECT_PATH=.. APP_BUILD_SCRIPT=jni/Android.mk

LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)

LOCAL_MODULE     := AT
LOCAL_SRC_FILES  := at_bridge.c

# Static linking - لا يعتمد على أي مكتبة على الجهاز
LOCAL_LDFLAGS    := -static

# تحسينات الحجم والأداء
LOCAL_CFLAGS     := -O2 -Wall -Wextra -DNDEBUG

# لا نحتاج STL
LOCAL_STL        := none

include $(BUILD_EXECUTABLE)
