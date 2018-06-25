LOCAL_PATH:= $(call my-dir)

#
# libmtkplayer
#

include $(CLEAR_VARS)

LOCAL_SRC_FILES:=               \
    FMAudioPlayer.cpp

#This is for customization
LOCAL_SHARED_LIBRARIES :=     \
	libcutils             \
	libutils              \
	libbinder             \
	libmedia              



LOCAL_C_INCLUDES :=  \
  $(JNI_H_INCLUDE)                                                \
  $(TOP)/frameworks/av/include                                  \
  $(TOP)/frameworks/av/include/media                              \
  $(TOP)/external                                                 \
  $(TOP)/frameworks/av/services/audioflinger                                
	
LOCAL_C_INCLUDES += \
  $(call include-path-for, audio-utils) \
  $(call include-path-for, audio-effects)
  
LOCAL_MODULE:= libmtkplayer

LOCAL_PRELINK_MODULE := no

include $(BUILD_SHARED_LIBRARY)

