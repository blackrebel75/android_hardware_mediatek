#ifndef ANDROID_AUDIO_MTK_HARDWARE_H
#define ANDROID_AUDIO_MTK_HARDWARE_H

#include <stdint.h>
#include <sys/types.h>
#include <utils/threads.h>
#include <utils/String8.h>

#include <hardware_legacy/AudioHardwareBase.h>
#include <hardware_legacy/AudioSystemLegacy.h>
#include <media/AudioSystem.h>
#include <utils/threads.h>
#include <utils/KeyedVector.h>
#include <utils/Vector.h>

#include "AudioType.h"
#include "AudioMTKVolumeInterface.h"
#include "AudioDigitalControlInterface.h"
#include "AudioAnalogControlInterface.h"
#include "AudioDigitalType.h"

#include "AudioMTKStreamInManager.h"
#include "AudioMTKStreamManager.h"
#include "AudioMTKStreamManagerInterface.h"

#include "AudioResourceManager.h"
#include "AudioResourceFactory.h"
#include "AudioFtm.h"

#include "SpeechDriverFactory.h"
#include "AudioSpeechEnhanceInfo.h"

#include "AudioParamTuning.h"

#include "AudioFMController.h"
#include "AudioMTKHardwareCommand.h"
namespace android
{

enum AudioCommand
{
    AUDIOCOMMAND_NONE = 0,
    SETOUTPUTFIRINDEX = 1,
    GETOUTPUTFIRINDEX = 2,
    SETMEDDATA = 3,
    GETMEDDATA = 4,
    GETAUDIOCUSTOMDATASIZE = 5,
    SETAUDIOCUSTOMDATA = 6,
    GETAUDIOCUSTOMDATA = 7,
    START_FMTX_SINEWAVE = 8,
    STOP_FMTX_SINEWAVE  = 9,
    // use for MED output FIR
    GETNORMALOUTPUTFIRINDEX = 0x10,
    GETHEADSETOUTPUTFIRINDEX = 0x11,
    GETSPEAKEROUTPUTFIRINDEX = 0x12,
    SETNORMALOUTPUTFIRINDEX = 0x20,
    SETHEADSETOUTPUTFIRINDEX = 0x21,
    SETSPEAKEROUTPUTFIRINDEX = 0x22,
    GET_DUAL_MIC_PARAMETER   = 0x30,
    SET_DUAL_MIC_PARAMETER   = 0x31,
    GET_WB_SPEECH_PARAMETER   = 0x40,
    SET_WB_SPEECH_PARAMETER   = 0x41,
    SET_LOAD_VOLUME_SETTING    = 0x50,
    SET_DUMP_AUDIO_AEE_CHECK    = 0x5E,
    GET_DUMP_AUDIO_AEE_CHECK    = 0x5F,
    // used for Speech Logger
    SET_SPEECH_VM_ENABLE       = 0x60,
    SET_DUMP_SPEECH_DEBUG_INFO = 0x61,
    // used for Audio Logger
    SET_DUMP_AUDIO_DEBUG_INFO  = 0x62,
    SET_DUMP_AUDIO_STREAM_OUT  = 0x63,
    GET_DUMP_AUDIO_STREAM_OUT  = 0x64,
    SET_DUMP_AUDIO_MIXER_BUF   = 0x65,
    GET_DUMP_AUDIO_MIXER_BUF   = 0x66,
    SET_DUMP_AUDIO_TRACK_BUF   = 0x67,
    GET_DUMP_AUDIO_TRACK_BUF   = 0x68,
    SET_DUMP_A2DP_STREAM_OUT   = 0x69,
    GET_DUMP_A2DP_STREAM_OUT   = 0x6A,
    SET_DUMP_AUDIO_STREAM_IN   = 0x6B,
    GET_DUMP_AUDIO_STREAM_IN   = 0x6C,
    SET_DUMP_IDLE_VM_RECORD    = 0x6D,
    GET_DUMP_IDLE_VM_RECORD    = 0x6E,
    // for audio taste tool
    AUD_TASTE_TUNING = 0x70,
    HOOK_FM_DEVICE_CALLBACK    = 0x71,
    UNHOOK_FM_DEVICE_CALLBACK  = 0x72,
    //used for Gain Table in advanced meta
    GET_GAIN_TABLE_CTRPOINT_NUM     = 0x80,
    GET_GAIN_TABLE_CTRPOINT_BITS    = 0x81,
    GET_GAIN_TABLE_CTRPOINT_TABLE   = 0x82,
    // used for test case
    AUDIO_USER_TEST = 0x90,
    AUDIO_SET_PMIC_REG = 0x91,
    // audio data
    GET_AUDIO_VER1_DATA = 0x100,
    SET_AUDIO_VER1_DATA = 0x101,
    // used for voice ui and unlock
    GET_VOICE_CUST_PARAM = 0x200,
    GET_VOICE_FIR_COEF = 0x201,
    GET_VOICE_GAIN = 0x202
};


class AudioMTKHardware : public android_audio_legacy::AudioHardwareBase
{
    public:
        AudioMTKHardware();
        AudioMTKHardware(bool SpeechControlEnable);
        virtual             ~AudioMTKHardware();

        /**
         * check to see if the audio hardware interface has been initialized.
         * return status based on values defined in include/utils/Errors.h
         */
        virtual status_t    initCheck();

        /** set the audio volume of a voice call. Range is between 0.0 and 1.0 */
        virtual status_t    setVoiceVolume(float volume);

        /**
         * set the audio volume for all audio activities other than voice call.
         * Range between 0.0 and 1.0. If any value other than NO_ERROR is returned,
         * the software mixer will emulate this capability.
         */
        virtual status_t    setMasterVolume(float volume);

        /**
         * setMode is called when the audio mode changes. NORMAL mode is for
         * standard audio playback, RINGTONE when a ringtone is playing, and IN_CALL
         * when a call is in progress.
         */
        virtual status_t    setMode(int mode);

        // mic mute
        virtual status_t    setMicMute(bool state);
        virtual status_t    getMicMute(bool *state);

        // set/get global audio parameters
        virtual status_t    setParameters(const String8 &keyValuePairs);
        virtual String8     getParameters(const String8 &keys);

        // Returns audio input buffer size according to parameters passed or 0 if one of the
        // parameters is not supported
        virtual size_t    getInputBufferSize(uint32_t sampleRate, int format, int channelCount);
        virtual status_t    getMasterVolume(float *volume);
        /** This method creates and opens the audio hardware output stream */
        virtual android_audio_legacy::AudioStreamOut *openOutputStream(
            uint32_t devices,
            int *format = 0,
            uint32_t *channels = 0,
            uint32_t *sampleRate = 0,
            status_t *status = 0);

        virtual android_audio_legacy::AudioStreamOut *openOutputStreamWithFlags(uint32_t devices,
                                                  audio_output_flags_t flags=(audio_output_flags_t)0,
                                                  int *format=0,
                                                  uint32_t *channels=0,
                                                  uint32_t *sampleRate=0,
                                                  status_t *status=0);

        virtual    void        closeOutputStream(android_audio_legacy::AudioStreamOut *out);
        /** This method creates and opens the audio hardware input stream */
        virtual android_audio_legacy::AudioStreamIn *openInputStream(
            uint32_t devices,
            int *format,
            uint32_t *channels,
            uint32_t *sampleRate,
            status_t *status,
            android_audio_legacy::AudioSystem::audio_in_acoustics acoustics);
        virtual    void        closeInputStream(android_audio_legacy::AudioStreamIn *in);

        /**This method dumps the state of the audio hardware */
        virtual status_t dumpState(int fd, const Vector<String16> &args);

        // Interface of AduioMTKHardware.h
        // add EM parameter or general purpose commands
        virtual status_t SetEMParameter(void *ptr, int len) ;
        virtual status_t GetEMParameter(void *ptr, int len) ;
        virtual status_t SetAudioCommand(int par1, int par2);
        virtual status_t GetAudioCommand(int par1);
        virtual status_t SetAudioData(int par1, size_t len, void *ptr);
        virtual status_t GetAudioData(int par1, size_t len, void *ptr);

        // ACF Preview parameter
        virtual status_t SetACFPreviewParameter(void *ptr , int len);
        virtual status_t SetHCFPreviewParameter(void *ptr , int len);

        //for PCMxWay Interface API
        virtual int xWayPlay_Start(int sample_rate);
        virtual int xWayPlay_Stop(void);
        virtual int xWayPlay_Write(void *buffer, int size_bytes);
        virtual int xWayPlay_GetFreeBufferCount(void);
        virtual int xWayRec_Start(int sample_rate);
        virtual int xWayRec_Stop(void);
        virtual int xWayRec_Read(void *buffer, int size_bytes);
        //added by wendy
        virtual int ReadRefFromRing(void *buf, uint32_t datasz, void *DLtime);
        virtual int GetVoiceUnlockULTime(void *DLtime);
        virtual int SetVoiceUnlockSRC(uint outSR, uint outChannel);
        virtual bool startVoiceUnlockDL();
        virtual bool stopVoiceUnlockDL();
        virtual void freeVoiceUnlockDLInstance();
        virtual int GetVoiceUnlockDLLatency();
        virtual bool getVoiceUnlockDLInstance();

        virtual status_t setMasterMute(bool muted);
        virtual int createAudioPatch(unsigned int num_sources,
                       const struct audio_port_config *sources,
                       unsigned int num_sinks,
                       const struct audio_port_config *sinks,
                       audio_patch_handle_t *handle);

        virtual int releaseAudioPatch(audio_patch_handle_t handle);

        virtual int getAudioPort(struct audio_port *port);

        virtual int setAudioPortConfig(const struct audio_port_config *config);
        class AudioHalPatch {
        public:
            AudioHalPatch(audio_patch_handle_t newHalHandle) :
                mHalHandle(newHalHandle), num_sources(0), num_sinks(0) {memset((void*)sources,0x00,sizeof(struct audio_port_config)*AUDIO_PATCH_PORTS_MAX);memset((void*)sinks,0x00,sizeof(struct audio_port_config)*AUDIO_PATCH_PORTS_MAX);}

            unsigned int      num_sources;      /* number of sources in following array */
            struct audio_port_config sources[AUDIO_PATCH_PORTS_MAX];
            unsigned int      num_sinks;        /* number of sinks in following array */
            struct audio_port_config sinks[AUDIO_PATCH_PORTS_MAX];
            audio_patch_handle_t mHalHandle;
        };
    protected:
        /** returns true if the given mode maps to a telephony or VoIP call is in progress */
        virtual bool     isModeInCall(int mode)
        {
            return ((mode == AUDIO_MODE_IN_CALL)
//                    || (mode == AUDIO_MODE_IN_CALL_2)
                    || (mode == AUDIO_MODE_IN_COMMUNICATION));
        }
        /** returns true if a telephony or VoIP call is in progress */
        virtual bool     isInCall() { return isModeInCall(mMode); }

	    float MappingFMVolofOutputDev(int Gain, audio_devices_t eOutput);
	private:
        status_t ForceAllStandby(void);
        status_t SetOutputSuspend(bool bEnable);
        status_t SetInputSuspend(bool bEnable);
        bool ModeInCall(audio_mode_t mode);

        // this will base on Mode and Audiohardware's mMode to check
        bool ModeEnterCall(audio_mode_t Mode);
        bool ModeLeaveCall(audio_mode_t Mode);
        bool ModeEnterSipCall(audio_mode_t Mode);
        bool ModeLeaveSipCall(audio_mode_t Mode);
        bool ModeCallSwitch(audio_mode_t Mode);

        bool InputStreamExist();
        bool OutputStreamExist();
        bool StreamExist();

        bool IsOutPutStreamActive();
        bool IsInPutStreamActive();

        status_t HardwareInit(bool BenableSpeech);

        bool UpdateOutputFIR(int mode , int index);

        bool ReadAuxadcData(int channel, int *value);
 

        virtual status_t dump(int fd, const Vector<String16> &args);

        void             UpdateKernelState();

        audio_mode_t     mMode;

        int              mFd;
        bool             mHardwareInit;

        bool             mMicMute;

        AudioMTKVolumeInterface *mAudioVolumeInstance;
        AudioAnalogControlInterface *mAudioAnalogInstance;
        AudioDigitalControlInterface *mAudioDigitalInstance;

        SpeechDriverFactory *mSpeechDriverFactory;

        AudioMTKStreamInManager *mStreamInManager;
        AudioResourceManagerInterface *mAudioResourceManager;
        AudioMTKStreamManagerInterface *mAudioMTKStreamManager;
        AudioFtm *mAudioFtmInstance;
        AudioAnalogReg *mAudioAnaRegInstance;;
        AudioSpeechEnhanceInfo *mAudioSpeechEnhanceInfoInstance;

        SPH_Control     mAudio_Control_State;

        AudioParamTuning *mAudioTuningInstance;

        pthread_mutex_t setParametersMutex;  // use for setParameters

        status_t SetAudioCommonCommand(int par1, int par2);
        status_t GetAudioCommonCommand(int parameters1);
        status_t SetAudioCommonData(int par1, size_t len, void *ptr);
        status_t GetAudioCommonData(int par1, size_t len, void *ptr);
};

}

#endif
