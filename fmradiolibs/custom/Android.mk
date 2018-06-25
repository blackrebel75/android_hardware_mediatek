
LOCAL_PATH := $(call my-dir)
###############################################################################
# Define MTK FM Radio Chip solution
###############################################################################

include $(CLEAR_VARS)

LOCAL_SRC_FILES := \
	custom.cpp 
	
LOCAL_C_INCLUDES := $(JNI_H_INCLUDE)
	
LOCAL_CFLAGS+= \
    -DMT6620_FM
LOCAL_CFLAGS+= \
    -DMT6628_FM
LOCAL_CFLAGS+= \
    -DMT6627_FM
ifeq ($(findstring MT6625_FM,$(MTK_FM_CHIP)),MT6625_FM)
LOCAL_CFLAGS+= \
    -DMT6627_FM
endif
ifeq ($(findstring MT6630_FM,$(MTK_FM_CHIP)),MT6630_FM)
LOCAL_CFLAGS+= \
    -DMT6630_FM
endif
LOCAL_SHARED_LIBRARIES := libcutils
LOCAL_PRELINK_MODULE := false

LOCAL_MODULE := libfmcust
include $(BUILD_SHARED_LIBRARY)


