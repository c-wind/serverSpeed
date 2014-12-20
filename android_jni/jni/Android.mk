LOCAL_PATH := $(call my-dir)
APP_ABI := armeabi
APP_PLATFORM := android-9
APP_OPTIM := release
LOCAL_ALLOW_UNDEFINED_SYMBOLS := false
LCAL_PRELINK_MODULE := false

include $(CLEAR_VARS)

LOCAL_CFLAGS := -Wall -DBUILD_JNI -I./
LOCAL_SRC_FILES := main.c tcp_conn.c tcp_echo.c udp_echo.c message_list.c

LOCAL_MODULE := speed
LOCAL_LDLIBS := -llog 

include $(BUILD_SHARED_LIBRARY)
