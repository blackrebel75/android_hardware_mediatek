#include "AudioMTKHardware.h"
#include "AudioVolumeFactory.h"
#include "AudioAnalogControlFactory.h"
#include "AudioDigitalControlFactory.h"

#include "AudioMTKStreamOut.h"
#include "AudioMTKStreamIn.h"
#include "AudioMTKStreamInManager.h"

#include "SpeechDriverInterface.h"
#include "SpeechDriverFactory.h"
#include "SpeechEnhancementController.h"
#include "SpeechPcm2way.h"
#include "SpeechPhoneCallController.h"

#include "LoopbackManager.h"

#include "AudioFMController.h"
#include "AudioMATVController.h"
#include "WCNChipController.h"


#include "AudioCompFltCustParam.h"
#include <media/AudioSystem.h>

#include <binder/IServiceManager.h>
#include <media/IAudioPolicyService.h>
#include <AudioMTKPolicyManager.h>

#ifdef MTK_AUDIO_HD_REC_SUPPORT
#include "AudioCustParam.h"
#endif
#include "AudioVUnlockDL.h"

#define LOG_TAG "AudioMTKHardware"
#ifndef ANDROID_DEFAULT_CODE
#include <cutils/xlog.h>
#ifdef ALOGE
#undef ALOGE
#endif
#ifdef ALOGW
#undef ALOGW
#endif ALOGI
#undef ALOGI
#ifdef ALOGD
#undef ALOGD
#endif
#ifdef ALOGV
#undef ALOGV
#endif
#define ALOGE XLOGE
#define ALOGW XLOGW
#define ALOGI XLOGI
#define ALOGD XLOGD
#define ALOGV XLOGV
#else
#include <utils/Log.h>
#endif


#define DL1_BUFFER_SIZE (0x4000)
#define DL2_BUFFER_SIZE (0x4000)
#define AWB_BUFFER_SIZE (0x4000)
#define VUL_BUFFER_SIZE (0x4000)

// for 16k samplerate  below , 8K buffer should be enough
#define DAI_BUFFER_SIZE (0x800)
#define MOD_DAI_BUFFER_SIZE (0x800)


namespace android
{

/*==============================================================================
 *                     setParameters() keys
 *============================================================================*/
static String8 keySetVTSpeechCall     = String8("SetVTSpeechCall");

// FM Related
static String8 keyAnalogFmEnable      = String8("AudioSetFmEnable");
static String8 keyDigitalFmEnable     = String8("AudioSetFmDigitalEnable");
static String8 keyGetFmEnable         = String8("GetFmEnable");

static String8 keySetFmVolume         = String8("SetFmVolume");

static String8 keySetFmForceToSpk     = String8("AudioSetForceToSpeaker");

static String8 keyGetIsWiredHeadsetOn = String8("AudioFmIsWiredHeadsetOn");

//mATV Related
static String8 keyMatvAnalogEnable    = String8("AtvAudioLineInEnable");;
static String8 keyMatvDigitalEnable   = String8("AudioSetMatvDigitalEnable");
static String8 keySetMatvVolume       = String8("SetMatvVolume");
static String8 keySetMatvMute         = String8("SetMatvMute");

//record left/right channel switch
//only support on dual MIC for switch LR input channel for video record when the device rotate
static String8 keyLR_ChannelSwitch = String8("LRChannelSwitch");
//force use Min MIC or Ref MIC data
//only support on dual MIC for only get main Mic or RefMic data
static String8 keyForceUseSpecificMicData = String8("ForceUseSpecificMic");

//enable using MTK VoIP in normal mode, otherwise will only using in in-communication mode or input source is in-communication
static String8 keyEnableNormalModeVoIP = String8("EnableNormalModeVoIP");

#ifdef MTK_AUDIO_HD_REC_SUPPORT
//static String8 key_HD_REC_MODE = String8("HDRecordMode");
static String8 keyHDREC_SET_VOICE_MODE = String8("HDREC_SET_VOICE_MODE");
static String8 keyHDREC_SET_VIDEO_MODE = String8("HDREC_SET_VIDEO_MODE");
#endif

//HDMI command
static String8 key_GET_HDMI_AUDIO_STATUS = String8("GetHDMIAudioStatus");
static String8 key_SET_HDMI_AUDIO_ENABLE = String8("SetHDMIAudioEnable");


// Audio Tool related
//<---for audio tool(speech/ACF/HCF/DMNR/HD/Audiotaste calibration) and HQA
static String8 keySpeechParams_Update = String8("UpdateSpeechParameter");
static String8 keySpeechVolume_Update = String8("UpdateSphVolumeParameter");
static String8 keyACFHCF_Update = String8("UpdateACFHCFParameters");
#if defined(MTK_DUAL_MIC_SUPPORT) || defined(MTK_AUDIO_HD_REC_SUPPORT)
static String8 keyDualMicParams_Update = String8("UpdateDualMicParameters");
static String8 keyDualMicRecPly = String8("DUAL_MIC_REC_PLAY");
static String8 keyDUALMIC_IN_FILE_NAME = String8("DUAL_MIC_IN_FILE_NAME");
static String8 keyDUALMIC_OUT_FILE_NAME = String8("DUAL_MIC_OUT_FILE_NAME");
static String8 keyDUALMIC_GET_GAIN = String8("DUAL_MIC_GET_GAIN");
static String8 keyDUALMIC_SET_UL_GAIN = String8("DUAL_MIC_SET_UL_GAIN");
static String8 keyDUALMIC_SET_DL_GAIN = String8("DUAL_MIC_SET_DL_GAIN");
static String8 keyDUALMIC_SET_HSDL_GAIN = String8("DUAL_MIC_SET_HSDL_GAIN");
#endif
static String8 keyMusicPlusSet      = String8("SetMusicPlusStatus");
static String8 keyMusicPlusGet      = String8("GetMusicPlusStatus");
static String8 keyHDRecTunningEnable    = String8("HDRecTunningEnable");
static String8 keyHDRecVMFileName   = String8("HDRecVMFileName");

//--->




// Dual Mic Noise Reduction, DMNR for Receiver
static String8 keyEnable_Dual_Mic_Setting = String8("Enable_Dual_Mic_Setting");
static String8 keyGet_Dual_Mic_Setting    = String8("Get_Dual_Mic_Setting");

// Dual Mic Noise Reduction, DMNR for Loud Speaker
static String8 keySET_LSPK_DMNR_ENABLE = String8("SET_LSPK_DMNR_ENABLE");
static String8 keyGET_LSPK_DMNR_ENABLE = String8("GET_LSPK_DMNR_ENABLE");

// Voice Clarity Engine, VCE
static String8 keySET_VCE_ENABLE = String8("SET_VCE_ENABLE");
static String8 keyGET_VCE_ENABLE = String8("GET_VCE_ENABLE");
static String8 keyGET_VCE_STATUS = String8("GET_VCE_STATUS"); // old name, rename to GET_VCE_ENABLE, but still reserve it

// Magic Conference Call
static String8 keySET_MAGIC_CON_CALL_ENABLE = String8("SET_MAGIC_CON_CALL_ENABLE");
static String8 keyGET_MAGIC_CON_CALL_ENABLE = String8("GET_MAGIC_CON_CALL_ENABLE");


static String8 keyGET_AUDIO_VOLUME_VER = String8("GET_AUDIO_VOLUME_VERSION");

// Loopbacks
static String8 keySET_LOOPBACK_USE_LOUD_SPEAKER = String8("SET_LOOPBACK_USE_LOUD_SPEAKER");
static String8 keySET_LOOPBACK_TYPE = String8("SET_LOOPBACK_TYPE");

// TTY
static String8 keySetTtyMode     = String8("tty_mode");

// MIC ANALOG SWITCH
static String8 keySetMicAnaLogSwitch   = String8("MicAnaLogSwitch");

// get FSync flag
static String8 keyGetFSyncFlag = String8("GET_FSYNC_FLAG");

// for stereo output
static String8 keyEnableStereoOutput = String8("EnableStereoOutput");

// BT WB
static String8 keySetBTMode     = String8("SET_DAIBT_MODE");

/*==============================================================================
 *                     Emulator
 *============================================================================*/
enum
{
    Normal_Coef_Index,
    Headset_Coef_Index,
    Handfree_Coef_Index,
    VOIPBT_Coef_Index,
    VOIPNormal_Coef_Index,
    VOIPHandfree_Coef_Index,
    AUX1_Coef_Index,
    AuX2_Coef_Index
};


/*==============================================================================
 *                     Property keys
 *============================================================================*/


/*==============================================================================
 *                     Function Implementation
 *============================================================================*/

bool AudioMTKHardware::IsOutPutStreamActive()
{
    return mAudioMTKStreamManager->IsOutPutStreamActive();
}

bool AudioMTKHardware::IsInPutStreamActive()
{
    return mAudioMTKStreamManager->IsInPutStreamActive();
}

status_t AudioMTKHardware::dumpState(int fd, const Vector<String16> &args)
{
    return NO_ERROR;
}

status_t AudioMTKHardware::dump(int fd, const Vector<String16> &args)
{
    return NO_ERROR;
}

status_t AudioMTKHardware::HardwareInit(bool bEnableSpeech)
{
    mFd = 0;
    mHardwareInit = false;
    mMode = AUDIO_MODE_NORMAL;

    mFd = ::open(kAudioDeviceName, O_RDWR);
    if (mFd == 0)
    {
        ALOGE("AudioMTKHardware contrcutor open mfd fail");
    }
    //if mediaerver died , aud driver should do recoevery.
    ::ioctl(mFd, AUD_RESTART, 0);

    mAudioResourceManager = AudioResourceManager::getInstance();
    mAudioResourceManager->EnableAudioClock(AudioResourceManagerInterface::CLOCK_AUD_AFE, true);
    mAudioResourceManager->EnableAudioClock(AudioResourceManagerInterface::CLOCK_AUD_ANA, true);

    mStreamInManager = AudioMTKStreamInManager::getInstance();
    mAudioFtmInstance = AudioFtm::getInstance();
    mAudioAnaRegInstance = AudioAnalogReg::getInstance();
    mAudioMTKStreamManager = AudioMTKStreamManager::getInstance();

    // create volume instance
    mAudioVolumeInstance = AudioVolumeFactory::CreateAudioVolumeController();
    mAudioVolumeInstance->initCheck();
    // create digital control instnace
    mAudioDigitalInstance  = AudioDigitalControlFactory::CreateAudioDigitalControl();
    mAudioDigitalInstance->InitCheck();
    // create digital control instnace
    mAudioAnalogInstance  = AudioAnalogControlFactory::CreateAudioAnalogControl();
    mAudioAnalogInstance->InitCheck();
    mAudioAnalogInstance->SetFrequency(AudioAnalogType::DEVICE_OUT_DAC, 44100);

    //allocate buffer when system is boot up
    mAudioDigitalInstance->SetMemBufferSize(AudioDigitalType::MEM_DL1, DL1_BUFFER_SIZE);
    mAudioDigitalInstance->SetMemBufferSize(AudioDigitalType::MEM_DL2, DL2_BUFFER_SIZE);
    mAudioDigitalInstance->SetMemBufferSize(AudioDigitalType::MEM_AWB, AWB_BUFFER_SIZE);
    mAudioDigitalInstance->SetMemBufferSize(AudioDigitalType::MEM_VUL, VUL_BUFFER_SIZE);
    mAudioDigitalInstance->SetMemBufferSize(AudioDigitalType::MEM_DAI, DAI_BUFFER_SIZE);
    mAudioDigitalInstance->SetMemBufferSize(AudioDigitalType::MEM_MOD_DAI, MOD_DAI_BUFFER_SIZE);

    //allocate buffer when system is boot up
    mAudioDigitalInstance->AllocateMemBufferSize(AudioDigitalType::MEM_DL1);
    mAudioDigitalInstance->AllocateMemBufferSize(AudioDigitalType::MEM_DL2);
    mAudioDigitalInstance->AllocateMemBufferSize(AudioDigitalType::MEM_AWB);
    mAudioDigitalInstance->AllocateMemBufferSize(AudioDigitalType::MEM_VUL);
    mAudioDigitalInstance->AllocateMemBufferSize(AudioDigitalType::MEM_DAI);
    mAudioDigitalInstance->AllocateMemBufferSize(AudioDigitalType::MEM_MOD_DAI);
    if (bEnableSpeech == false)
    {
        // no need to enable speech drivers
        mSpeechDriverFactory = NULL;
    }
    else
    {
        // first time to get speech driver factory, which will create speech dirvers
        mSpeechDriverFactory = SpeechDriverFactory::GetInstance();
    }

    mAudioSpeechEnhanceInfoInstance = AudioSpeechEnhanceInfo::getInstance();
    memset((void *)&mAudio_Control_State, 0, sizeof(SPH_Control));
    mHardwareInit = true;

    // parameters calibrations instance
    mAudioTuningInstance = AudioParamTuning::getInstance();

    // default not mute mic
    mMicMute = false;

    int ret = 0;
    ret |= pthread_mutex_init(&setParametersMutex, NULL);
    if (ret != 0) { ALOGE("Failed to initialize pthread setParametersMutex"); }

    mAudioResourceManager->EnableAudioClock(AudioResourceManagerInterface::CLOCK_AUD_AFE, false);
    mAudioResourceManager->EnableAudioClock(AudioResourceManagerInterface::CLOCK_AUD_ANA, false);

    return NO_ERROR;
}

AudioMTKHardware::AudioMTKHardware()
{
    HardwareInit(true);
    ALOGD("AudioMTKHardware create \n");
}

AudioMTKHardware::AudioMTKHardware(bool SpeechControlEnable)
{
    HardwareInit(SpeechControlEnable);
}

AudioMTKHardware::~AudioMTKHardware()
{
    if (mSpeechDriverFactory != NULL)
    {
        delete mSpeechDriverFactory;
        mSpeechDriverFactory = NULL;
    }
}

status_t AudioMTKHardware::setParameters(const String8 &keyValuePairs)
{
    status_t status = NO_ERROR;
    int value = 0;
    float value_float = 0.0;

    String8 value_str;
    pthread_mutex_lock(&setParametersMutex);
    AudioParameter param = AudioParameter(keyValuePairs);
    ALOGD("+setParameters(): %s ", keyValuePairs.string());

    // VT call (true) / Voice call (false)
    if (param.getInt(keySetVTSpeechCall, value) == NO_ERROR)
    {
        param.remove(keySetVTSpeechCall);
        SpeechPhoneCallController::GetInstance()->SetVtNeedOn((bool)value);
        goto EXIT_SETPARAMETERS;
    }

    //Analog FM enable
    if (param.getInt(keyAnalogFmEnable, value) == NO_ERROR)
    {
        param.remove(keyAnalogFmEnable);
        WARNING("Not Support FM Analog Line In Path Anymore. Please Use Merge_I2S / 2nd_I2S_In");
        goto EXIT_SETPARAMETERS;
    }
    //Digital FM enable
    if (param.getInt(keyDigitalFmEnable, value) == NO_ERROR)
    {
        param.remove(keyDigitalFmEnable);
        AudioFMController::GetInstance()->SetFmEnable((bool)value);
        goto EXIT_SETPARAMETERS;
    }
    //Set FM volume
    if (param.getFloat(keySetFmVolume, value_float) == NO_ERROR)
    {
        param.remove(keySetFmVolume);
        AudioFMController::GetInstance()->SetFmVolume(value_float);
        goto EXIT_SETPARAMETERS;
    }
    //Force FM to loudspeaker
    if (param.getInt(keySetFmForceToSpk, value) == NO_ERROR)
    {
        param.remove(keySetFmForceToSpk);
        WARNING("Do nothing for this command: AudioSetForceToSpeaker");
        goto EXIT_SETPARAMETERS;
    }

    //Analog mATV Enable
    if (param.getInt(keyMatvAnalogEnable, value) == NO_ERROR)
    {
        ALOGD("SetMatvLineInEnable = %d", value);
#if defined(MATV_AUDIO_SUPPORT)
        AudioMATVController::GetInstance()->SetMatvEnable((bool)value, MATV_ANALOG);
#else
        WARNING("Do nothing for this command: AtvAudioLineInEnable");
#endif
        param.remove(keyMatvAnalogEnable);
        goto EXIT_SETPARAMETERS;
    }
    //Digital mATV Enable
    if (param.getInt(keyMatvDigitalEnable, value) == NO_ERROR)
    {
        ALOGD("keySetMatvDigitalEnable = %d", value);
#if defined(MATV_AUDIO_SUPPORT)
        AudioMATVController::GetInstance()->SetMatvEnable((bool)value, MATV_DIGITAL);
#else
        WARNING("Do nothing for this command: AudioSetMatvDigitalEnable");
#endif
        param.remove(keyMatvDigitalEnable);
        goto EXIT_SETPARAMETERS;
    }
    //Set mATV Volume
    if (param.getInt(keySetMatvVolume, value) == NO_ERROR)
    {
#ifdef MATV_AUDIO_LINEIN_PATH
        ALOGD("SetMatvVolume = %d", value);
        mAudioVolumeInstance->setMatvVolume(value);
#endif
        param.remove(keySetMatvVolume);
        goto EXIT_SETPARAMETERS;
    }
    //mute mATV
    if (param.getInt(keySetMatvMute, value) == NO_ERROR)
    {
#ifdef MATV_AUDIO_LINEIN_PATH
        ALOGD("SetMatvMute=%d", value);
        mAudioVolumeInstance->SetMatvMute((bool)value);
#endif
        param.remove(keySetMatvMute);
        goto EXIT_SETPARAMETERS;
    }

    //MusicPkus enable
    if (param.getInt(keyMusicPlusSet, value) == NO_ERROR)
    {
        mAudioMTKStreamManager->SetMusicPlusStatus(value ? true : false);
        param.remove(keyMusicPlusSet);
        goto EXIT_SETPARAMETERS;
    }

    if (param.getInt(keyLR_ChannelSwitch, value) == NO_ERROR)
    {
#ifdef MTK_DUAL_MIC_SUPPORT
        ALOGD("keyLR_ChannelSwitch=%d", value);
        bool bIsLRSwitch = value;
        mAudioSpeechEnhanceInfoInstance->SetRecordLRChannelSwitch(bIsLRSwitch);
#else
        ALOGD("only support in dual MIC");
#endif
        param.remove(keyLR_ChannelSwitch);
        //goto EXIT_SETPARAMETERS;
        //Because parameters will send two strings, we need to parse another.(HD Record info and Channel Switch info)
    }

    if (param.getInt(keyForceUseSpecificMicData, value) == NO_ERROR)
    {
#ifdef MTK_DUAL_MIC_SUPPORT
        ALOGD("keyForceUseSpecificMicData=%d", value);
        int32 UseSpecificMic = value;
        mAudioSpeechEnhanceInfoInstance->SetUseSpecificMIC(UseSpecificMic);
#else
        ALOGD("only support in dual MIC");
#endif
        param.remove(keyForceUseSpecificMicData);
        goto EXIT_SETPARAMETERS;
    }

#ifdef MTK_AUDIO_HD_REC_SUPPORT
    if (param.getInt(keyHDREC_SET_VOICE_MODE, value) == NO_ERROR)
    {
        ALOGD("HDREC_SET_VOICE_MODE=%d", value); // Normal, Indoor, Outdoor,
        param.remove(keyHDREC_SET_VOICE_MODE);
        //Get and Check Voice/Video Mode Offset
        AUDIO_HD_RECORD_SCENE_TABLE_STRUCT hdRecordSceneTable;
        GetHdRecordSceneTableFromNV(&hdRecordSceneTable);
        if (value < hdRecordSceneTable.num_voice_rec_scenes)
        {
            int32 HDRecScene = value + 1;//1:cts verifier offset
            mAudioSpeechEnhanceInfoInstance->SetHDRecScene(HDRecScene);
        }
        else
        {
            ALOGE("HDREC_SET_VOICE_MODE=%d exceed max value(%d)\n", value, hdRecordSceneTable.num_voice_rec_scenes);
        }
        goto EXIT_SETPARAMETERS;
    }

    if (param.getInt(keyHDREC_SET_VIDEO_MODE, value) == NO_ERROR)
    {
        ALOGD("HDREC_SET_VIDEO_MODE=%d", value); // Normal, Indoor, Outdoor,
        param.remove(keyHDREC_SET_VIDEO_MODE);
        //Get and Check Voice/Video Mode Offset
        AUDIO_HD_RECORD_SCENE_TABLE_STRUCT hdRecordSceneTable;
        GetHdRecordSceneTableFromNV(&hdRecordSceneTable);
        if (value < hdRecordSceneTable.num_video_rec_scenes)
        {
            uint32 offset = hdRecordSceneTable.num_voice_rec_scenes + 1;//1:cts verifier offset
            int32 HDRecScene = value + offset;
            mAudioSpeechEnhanceInfoInstance->SetHDRecScene(HDRecScene);
        }
        else
        {
            ALOGE("HDREC_SET_VIDEO_MODE=%d exceed max value(%d)\n", value, hdRecordSceneTable.num_video_rec_scenes);
        }
        goto EXIT_SETPARAMETERS;
    }

    if (param.getInt(keyEnableNormalModeVoIP, value) == NO_ERROR)
    {
        ALOGD("EnableNormalModeVoIP=%d", value);
        param.remove(keyEnableNormalModeVoIP);
        bool bEnableNormalModeVoIP = value;
        mAudioSpeechEnhanceInfoInstance->SetEnableNormalModeVoIP(bEnableNormalModeVoIP);
        goto EXIT_SETPARAMETERS;
    }
#endif
    //<---for audio tool(speech/ACF/HCF/DMNR/HD/Audiotaste calibration)
    // calibrate speech parameters
    if (param.getInt(keySpeechParams_Update, value) == NO_ERROR)
    {
        ALOGD("setParameters Update Speech Parames");
        if (value == 0)
        {
            AUDIO_CUSTOM_PARAM_STRUCT eSphParamNB;
            GetNBSpeechParamFromNVRam(&eSphParamNB);
            SpeechEnhancementController::GetInstance()->SetNBSpeechParametersToAllModem(&eSphParamNB);
        }
#if defined(MTK_WB_SPEECH_SUPPORT)
        else if (value == 1)
        {
            AUDIO_CUSTOM_WB_PARAM_STRUCT eSphParamWB;
            GetWBSpeechParamFromNVRam(&eSphParamWB);
            SpeechEnhancementController::GetInstance()->SetWBSpeechParametersToAllModem(&eSphParamWB);
        }
#endif

        if (ModeInCall(mMode) == true)   // get output device for in_call, and set speech mode
        {
            const audio_devices_t output_device = (audio_devices_t)mAudioResourceManager->getDlOutputDevice();
            const audio_devices_t input_device  = (audio_devices_t)mAudioResourceManager->getUlInputDevice();
            mSpeechDriverFactory->GetSpeechDriver()->SetSpeechMode(input_device, output_device);
        }
        param.remove(keySpeechParams_Update);
        goto EXIT_SETPARAMETERS;
    }
#if defined(MTK_DUAL_MIC_SUPPORT)
    if (param.getInt(keyDualMicParams_Update, value) == NO_ERROR)
    {
        param.remove(keyDualMicParams_Update);
        AUDIO_CUSTOM_EXTRA_PARAM_STRUCT eSphParamDualMic;
        GetDualMicSpeechParamFromNVRam(&eSphParamDualMic);
        SpeechEnhancementController::GetInstance()->SetDualMicSpeechParametersToAllModem(&eSphParamDualMic);

        if (ModeInCall(mMode) == true)   // get output device for in_call, and set speech mode
        {
            const audio_devices_t output_device = (audio_devices_t)mAudioResourceManager->getDlOutputDevice();
            const audio_devices_t input_device  = (audio_devices_t)mAudioResourceManager->getUlInputDevice();
            mSpeechDriverFactory->GetSpeechDriver()->SetSpeechMode(input_device, output_device);
        }
        goto EXIT_SETPARAMETERS;
    }
#endif
    // calibrate speech volume
    if (param.getInt(keySpeechVolume_Update, value) == NO_ERROR)
    {
        ALOGD("setParameters Update Speech volume");
        mAudioVolumeInstance->initVolumeController();
        if (ModeInCall(mMode) == true)
        {
            int32_t outputDevice = mAudioResourceManager->getDlOutputDevice();
            SpeechPhoneCallController *pSpeechPhoneCallController = SpeechPhoneCallController::GetInstance();
#ifndef MTK_AUDIO_GAIN_TABLE
            mAudioVolumeInstance->setVoiceVolume(mAudioVolumeInstance->getVoiceVolume(), mMode, (uint32)outputDevice);
#endif
            switch (outputDevice)
            {
                case AUDIO_DEVICE_OUT_WIRED_HEADSET :
                {
#ifdef  MTK_TTY_SUPPORT
                    if (pSpeechPhoneCallController->GetTtyCtmMode() == AUD_TTY_VCO)
                    {
                        mAudioVolumeInstance->ApplyMicGain(Normal_Mic, mMode);
                    }
                    else if (pSpeechPhoneCallController->GetTtyCtmMode() == AUD_TTY_HCO || pSpeechPhoneCallController->GetTtyCtmMode() == AUD_TTY_FULL)
                    {
                        mAudioVolumeInstance->ApplyMicGain(TTY_CTM_Mic, mMode);
                    }
                    else
                    {
                        mAudioVolumeInstance->ApplyMicGain(Headset_Mic, mMode);
                    }
#else
                    mAudioVolumeInstance->ApplyMicGain(Headset_Mic, mMode);
#endif
                    break;
                }
                case AUDIO_DEVICE_OUT_WIRED_HEADPHONE :
                {
#ifdef  MTK_TTY_SUPPORT
                    if (pSpeechPhoneCallController->GetTtyCtmMode() == AUD_TTY_VCO)
                    {
                        mAudioVolumeInstance->ApplyMicGain(Normal_Mic, mMode);
                    }
                    else if (pSpeechPhoneCallController->GetTtyCtmMode() == AUD_TTY_HCO || pSpeechPhoneCallController->GetTtyCtmMode() == AUD_TTY_FULL)
                    {
                        mAudioVolumeInstance->ApplyMicGain(TTY_CTM_Mic, mMode);
                    }
                    else
                    {
                        mAudioVolumeInstance->ApplyMicGain(Handfree_Mic, mMode);
                    }
#else
                    mAudioVolumeInstance->ApplyMicGain(Handfree_Mic, mMode);
#endif
                    break;
                }
                case AUDIO_DEVICE_OUT_SPEAKER:
                {
                    mAudioVolumeInstance->ApplyMicGain(Handfree_Mic, mMode);
                    break;
                }
                case AUDIO_DEVICE_OUT_EARPIECE:
                {
                    mAudioVolumeInstance->ApplyMicGain(Normal_Mic, mMode);
                    break;
                }
                default:
                {
                    break;
                }
            }
        }
        else
        {
            setMasterVolume(mAudioVolumeInstance->getMasterVolume());
        }
        param.remove(keySpeechVolume_Update);
        goto EXIT_SETPARAMETERS;
    }
    // ACF/HCF parameters calibration
    if (param.getInt(keyACFHCF_Update, value) == NO_ERROR)
    {
        mAudioMTKStreamManager->UpdateACFHCF(value);
        param.remove(keyACFHCF_Update);
        goto EXIT_SETPARAMETERS;
    }
    // HD recording and DMNR calibration
#if defined(MTK_DUAL_MIC_SUPPORT) || defined(MTK_AUDIO_HD_REC_SUPPORT)
    if (param.getInt(keyDualMicRecPly, value) == NO_ERROR)
    {
        unsigned short cmdType = value & 0x000F;
        bool bWB = (value >> 4) & 0x000F;
        status_t ret = NO_ERROR;
        switch (cmdType)
        {
#ifdef DMNR_TUNNING_AT_MODEMSIDE
            case DUAL_MIC_REC_PLAY_STOP:
                ret = mAudioTuningInstance->enableDMNRModem2Way(false, bWB, P2W_RECEIVER_OUT, P2W_NORMAL);
                break;
            case DUAL_MIC_REC:
                ret = mAudioTuningInstance->enableDMNRModem2Way(true, bWB, P2W_RECEIVER_OUT, P2W_RECONLY);
                break;
            case DUAL_MIC_REC_PLAY:
                ret = mAudioTuningInstance->enableDMNRModem2Way(true, bWB, P2W_RECEIVER_OUT, P2W_NORMAL);
                break;
            case DUAL_MIC_REC_PLAY_HS:
                ret = mAudioTuningInstance->enableDMNRModem2Way(true, bWB, P2W_HEADSET_OUT, P2W_NORMAL);
                break;
#else//dmnr tunning at ap side
            case DUAL_MIC_REC_PLAY_STOP:
                ret = mAudioTuningInstance->enableDMNRAtApSide(false, bWB, OUTPUT_DEVICE_RECEIVER, RECPLAY_MODE);
                break;
            case DUAL_MIC_REC:
                ret = mAudioTuningInstance->enableDMNRAtApSide(true, bWB, OUTPUT_DEVICE_RECEIVER, RECONLY_MODE);
                break;
            case DUAL_MIC_REC_PLAY:
                ret = mAudioTuningInstance->enableDMNRAtApSide(true, bWB, OUTPUT_DEVICE_RECEIVER, RECPLAY_MODE);
                break;
            case DUAL_MIC_REC_PLAY_HS:
                ret = mAudioTuningInstance->enableDMNRAtApSide(true, bWB, OUTPUT_DEVICE_HEADSET, RECPLAY_MODE);
                break;
#endif
            default:
                ret = BAD_VALUE;
                break;
        }
        if (ret == NO_ERROR)
        {
            param.remove(keyDualMicRecPly);
        }
        goto EXIT_SETPARAMETERS;
    }

    if (param.get(keyDUALMIC_IN_FILE_NAME, value_str) == NO_ERROR)
    {
        if (mAudioTuningInstance->setPlaybackFileName(value_str.string()) == NO_ERROR)
        {
            param.remove(keyDUALMIC_IN_FILE_NAME);
        }
        goto EXIT_SETPARAMETERS;
    }

    if (param.get(keyDUALMIC_OUT_FILE_NAME, value_str) == NO_ERROR)
    {
        if (mAudioTuningInstance->setRecordFileName(value_str.string()) == NO_ERROR)
        {
#ifndef DMNR_TUNNING_AT_MODEMSIDE
            if (mAudioSpeechEnhanceInfoInstance->SetHDRecVMFileName(value_str.string()) == NO_ERROR)
#endif
                param.remove(keyDUALMIC_OUT_FILE_NAME);
        }
        goto EXIT_SETPARAMETERS;
    }

    if (param.getInt(keyDUALMIC_SET_UL_GAIN, value) == NO_ERROR)
    {
        if (mAudioTuningInstance->setDMNRGain(AUD_MIC_GAIN, value) == NO_ERROR)
        {
            param.remove(keyDUALMIC_SET_UL_GAIN);
        }
        goto EXIT_SETPARAMETERS;
    }

    if (param.getInt(keyDUALMIC_SET_DL_GAIN, value) == NO_ERROR)
    {
        if (mAudioTuningInstance->setDMNRGain(AUD_RECEIVER_GAIN, value) == NO_ERROR)
        {
            param.remove(keyDUALMIC_SET_DL_GAIN);
        }
        goto EXIT_SETPARAMETERS;
    }

    if (param.getInt(keyDUALMIC_SET_HSDL_GAIN, value) == NO_ERROR)
    {
        if (mAudioTuningInstance->setDMNRGain(AUD_HS_GAIN, value) == NO_ERROR)
        {
            param.remove(keyDUALMIC_SET_HSDL_GAIN);
        }
        goto EXIT_SETPARAMETERS;
    }
#endif

    if (param.getInt(keyHDRecTunningEnable, value) == NO_ERROR)
    {
        ALOGD("keyHDRecTunningEnable=%d", value);
        bool bEnable = value;
        mAudioSpeechEnhanceInfoInstance->SetHDRecTunningEnable(bEnable);
        param.remove(keyHDRecTunningEnable);
        goto EXIT_SETPARAMETERS;
    }

    if (param.get(keyHDRecVMFileName, value_str) == NO_ERROR)
    {
        ALOGD("keyHDRecVMFileName=%s", value_str.string());
        if (mAudioSpeechEnhanceInfoInstance->SetHDRecVMFileName(value_str.string()) == NO_ERROR)
        {
            param.remove(keyHDRecVMFileName);
        }
        goto EXIT_SETPARAMETERS;
    }

    // --->for audio tool(speech/ACF/HCF/DMNR/HD/Audiotaste calibration)

#if defined(MTK_DUAL_MIC_SUPPORT)
    // Dual Mic Noise Reduction, DMNR for Receiver
    if (param.getInt(keyEnable_Dual_Mic_Setting, value) == NO_ERROR)
    {
        param.remove(keyEnable_Dual_Mic_Setting);
        SpeechEnhancementController::GetInstance()->SetDynamicMaskOnToAllModem(SPH_ENH_DYNAMIC_MASK_DMNR, (bool)value);
#if defined(MTK_HANDSFREE_DMNR_SUPPORT)
        mAudioSpeechEnhanceInfoInstance->UpdateDynamicSpeechEnhancementMask();
#endif
        goto EXIT_SETPARAMETERS;
    }

    // Dual Mic Noise Reduction, DMNR for Loud Speaker
    if (param.getInt(keySET_LSPK_DMNR_ENABLE, value) == NO_ERROR)
    {
        param.remove(keySET_LSPK_DMNR_ENABLE);
        SpeechEnhancementController::GetInstance()->SetDynamicMaskOnToAllModem(SPH_ENH_DYNAMIC_MASK_LSPK_DMNR, (bool)value);
#if defined(MTK_HANDSFREE_DMNR_SUPPORT)
        mAudioSpeechEnhanceInfoInstance->UpdateDynamicSpeechEnhancementMask();
#endif
        if (SpeechEnhancementController::GetInstance()->GetMagicConferenceCallOn() == true &&
            SpeechEnhancementController::GetInstance()->GetDynamicMask(SPH_ENH_DYNAMIC_MASK_LSPK_DMNR) == true)
        {
            ALOGE("Cannot open MagicConCall & LoudSpeaker DMNR at the same time!!");
        }

        goto EXIT_SETPARAMETERS;
    }
#endif

    // Voice Clarity Engine, VCE
    if (param.getInt(keySET_VCE_ENABLE, value) == NO_ERROR)
    {
        param.remove(keySET_VCE_ENABLE);
        SpeechEnhancementController::GetInstance()->SetDynamicMaskOnToAllModem(SPH_ENH_DYNAMIC_MASK_VCE, (bool)value);
        goto EXIT_SETPARAMETERS;
    }

    // Magic Conference Call
    if (param.getInt(keySET_MAGIC_CON_CALL_ENABLE, value) == NO_ERROR)
    {
        param.remove(keySET_MAGIC_CON_CALL_ENABLE);

        const bool magic_conference_call_on = (bool)value;

        // enable/disable flag
        SpeechEnhancementController::GetInstance()->SetMagicConferenceCallOn(magic_conference_call_on);

        // apply speech mode
        if (ModeInCall(mMode) == true)
        {
            const audio_devices_t output_device = (audio_devices_t)mAudioResourceManager->getDlOutputDevice();
            const audio_devices_t input_device  = (audio_devices_t)mAudioResourceManager->getUlInputDevice();
            if (output_device == AUDIO_DEVICE_OUT_SPEAKER)
            {
                mSpeechDriverFactory->GetSpeechDriver()->SetSpeechMode(input_device, output_device);
            }
        }

        if (SpeechEnhancementController::GetInstance()->GetMagicConferenceCallOn() == true &&
            SpeechEnhancementController::GetInstance()->GetDynamicMask(SPH_ENH_DYNAMIC_MASK_LSPK_DMNR) == true)
        {
            ALOGE("Cannot open MagicConCall & LoudSpeaker DMNR at the same time!!");
        }

        goto EXIT_SETPARAMETERS;
    }


    // Loopback use speaker or not
    static bool bForceUseLoudSpeakerInsteadOfReceiver = false;
    if (param.getInt(keySET_LOOPBACK_USE_LOUD_SPEAKER, value) == NO_ERROR)
    {
        param.remove(keySET_LOOPBACK_USE_LOUD_SPEAKER);
        bForceUseLoudSpeakerInsteadOfReceiver = value & 0x1;
        goto EXIT_SETPARAMETERS;
    }

    // Loopback
    if (param.get(keySET_LOOPBACK_TYPE, value_str) == NO_ERROR)
    {
        param.remove(keySET_LOOPBACK_TYPE);

        // parse format like "SET_LOOPBACK_TYPE=1" / "SET_LOOPBACK_TYPE=1+0"
        int type_value = NO_LOOPBACK;
        int device_value = -1;
        sscanf(value_str.string(), "%d,%d", &type_value, &device_value);
        ALOGV("type_value = %d, device_value = %d", type_value, device_value);

        const loopback_t loopback_type = (loopback_t)type_value;
        loopback_output_device_t loopback_output_device;

        if (loopback_type == NO_LOOPBACK)   // close loopback
        {
            LoopbackManager::GetInstance()->SetLoopbackOff();
        }
        else   // open loopback
        {
            if (device_value == LOOPBACK_OUTPUT_RECEIVER ||
                device_value == LOOPBACK_OUTPUT_EARPHONE ||
                device_value == LOOPBACK_OUTPUT_SPEAKER)   // assign output device
            {
                loopback_output_device = (loopback_output_device_t)device_value;
            }
            else   // not assign output device
            {
                if (AudioSystem::getDeviceConnectionState(AUDIO_DEVICE_OUT_WIRED_HEADSET,   "") == android_audio_legacy::AudioSystem::DEVICE_STATE_AVAILABLE ||
                    AudioSystem::getDeviceConnectionState(AUDIO_DEVICE_OUT_WIRED_HEADPHONE, "") == android_audio_legacy::AudioSystem::DEVICE_STATE_AVAILABLE)
                {
                    loopback_output_device = LOOPBACK_OUTPUT_EARPHONE;
                }
                else if (bForceUseLoudSpeakerInsteadOfReceiver == true)
                {
                    loopback_output_device = LOOPBACK_OUTPUT_SPEAKER;
                }
                else
                {
                    loopback_output_device = LOOPBACK_OUTPUT_RECEIVER;
                }
            }
            LoopbackManager::GetInstance()->SetLoopbackOn(loopback_type, loopback_output_device);
        }
        goto EXIT_SETPARAMETERS;
    }


#ifdef  MTK_TTY_SUPPORT
    // Set TTY mode
    if (param.get(keySetTtyMode, value_str) == NO_ERROR)
    {
        param.remove(keySetTtyMode);
        tty_mode_t tty_mode;

        if (value_str == "tty_full")
        {
            tty_mode = AUD_TTY_FULL;
        }
        else if (value_str == "tty_vco")
        {
            tty_mode = AUD_TTY_VCO;
        }
        else if (value_str == "tty_hco")
        {
            tty_mode = AUD_TTY_HCO;
        }
        else if (value_str == "tty_off")
        {
            tty_mode = AUD_TTY_OFF;
        }
        else
        {
            ALOGD("setParameters tty_mode error !!");
            tty_mode = AUD_TTY_ERR;
        }

        SpeechPhoneCallController::GetInstance()->SetTtyCtmMode(tty_mode, mMode);

        goto EXIT_SETPARAMETERS;
    }
#endif
#ifdef MTK_3MIC_SUPPORT
    if (param.getInt(keySetMicAnaLogSwitch, value) == NO_ERROR)
    {
        ALOGD("keySetMicAnaLogSwitch value = %d", value);
        param.remove(keySetMicAnaLogSwitch);
        mAudioResourceManager->setParameters(INFO_U2K_MICANA_SWITCH, value , 0);
        goto EXIT_SETPARAMETERS;
    }
#endif
    if (param.getInt(keyEnableStereoOutput, value) == NO_ERROR)
    {
        ALOGD("keyEnableStereoOutput=%d", value);
        mAudioMTKStreamManager->setParameters(keyValuePairs, 0);
        param.remove(keyEnableStereoOutput);
        goto EXIT_SETPARAMETERS;
    }

    if (param.getInt(keySetBTMode, value) == NO_ERROR)
    {
        param.remove(keySetBTMode);

        if ((0 != value) && (1 != value))
        {
            ALOGD("setParameters BTMode error, no support mode %d !!", value);
            value = 0;
        }

        mAudioDigitalInstance->BT_SCO_SetMode(value);
#if 1 //mark temp, should unmark for phone call WB BTSCO
        SpeechPhoneCallController::GetInstance()->SetBTMode(value);
#endif
        goto EXIT_SETPARAMETERS;
    }


EXIT_SETPARAMETERS:
    if (param.size())
    {
        ALOGE("%s() still have param.size() = %d, remain param = \"%s\"", __FUNCTION__, param.size(), param.toString().string());
        status = BAD_VALUE;
    }
    ALOGD("-setParameters(): %s ", keyValuePairs.string());
    pthread_mutex_unlock(&setParametersMutex);
    return status;
}

String8 AudioMTKHardware::getParameters(const String8 &keys)
{
    ALOGD("+%s(), keys = %s", __FUNCTION__, keys.string());

    AudioParameter param = AudioParameter(keys);
    AudioParameter returnParam = AudioParameter();
    String8 value;
    int cmdType = 0;

    if (param.get(keyGetFmEnable, value) == NO_ERROR)
    {
        AudioFMController *pAudioFMController = AudioFMController::GetInstance();
        const bool rx_status       = pAudioFMController->GetFmEnable();
        const bool fm_power_status = pAudioFMController->GetFmChipPowerInfo();
        value = (rx_status && fm_power_status) ? "true" : "false";
        param.remove(keyGetFmEnable);
        returnParam.add(keyGetFmEnable, value);
        goto EXIT_GETPARAMETERS;
    }


    if (param.get(keyGetIsWiredHeadsetOn, value) == NO_ERROR)
    {
        ALOGD("+getParameters keyGetIsWiredHeadsetOn ");

        bool isWiredHeadsetOn = mAudioResourceManager->IsWiredHeadsetOn();
        value = (isWiredHeadsetOn) ? "true" : "false";
        param.remove(keyGetIsWiredHeadsetOn);
        returnParam.add(keyGetIsWiredHeadsetOn, value);
        goto EXIT_GETPARAMETERS;
    }


#if defined(MTK_DUAL_MIC_SUPPORT)
    if (param.getInt(keyDUALMIC_GET_GAIN, cmdType) == NO_ERROR)
    {
        unsigned short gain = 0;
        char buf[32];

        if (mAudioTuningInstance->getDMNRGain((unsigned short)cmdType, &gain) == NO_ERROR)
        {
            sprintf(buf, "%d", gain);
            returnParam.add(keyDUALMIC_GET_GAIN, String8(buf));
            param.remove(keyDUALMIC_GET_GAIN);
        }
        goto EXIT_GETPARAMETERS;
    }
#endif

    if (param.get(keyMusicPlusGet, value) == NO_ERROR)
    {
        bool musicplus_status = mAudioMTKStreamManager->GetMusicPlusStatus();
        value = (musicplus_status) ? "1" : "0";
        param.remove(keyMusicPlusGet);
        returnParam.add(keyMusicPlusGet, value);
        goto EXIT_GETPARAMETERS;
    }

    // Dual Mic Noise Reduction, DMNR for Receiver
    if (param.get(keyGet_Dual_Mic_Setting, value) == NO_ERROR)   // new name
    {
        param.remove(keyGet_Dual_Mic_Setting);
        value = (SpeechEnhancementController::GetInstance()->GetDynamicMask(SPH_ENH_DYNAMIC_MASK_DMNR) > 0) ? "1" : "0";
        returnParam.add(keyGet_Dual_Mic_Setting, value);
        goto EXIT_GETPARAMETERS;
    }

    // Dual Mic Noise Reduction, DMNR for Loud Speaker
    if (param.get(keyGET_LSPK_DMNR_ENABLE, value) == NO_ERROR)   // new name
    {
        param.remove(keyGET_LSPK_DMNR_ENABLE);
        value = (SpeechEnhancementController::GetInstance()->GetDynamicMask(SPH_ENH_DYNAMIC_MASK_LSPK_DMNR) > 0) ? "1" : "0";
        returnParam.add(keyGET_LSPK_DMNR_ENABLE, value);
        goto EXIT_GETPARAMETERS;
    }


    // Voice Clarity Engine, VCE
    if (param.get(keyGET_VCE_ENABLE, value) == NO_ERROR)   // new name
    {
        param.remove(keyGET_VCE_ENABLE);
        value = (SpeechEnhancementController::GetInstance()->GetDynamicMask(SPH_ENH_DYNAMIC_MASK_VCE) > 0) ? "1" : "0";
        returnParam.add(keyGET_VCE_ENABLE, value);
        goto EXIT_GETPARAMETERS;
    }
    if (param.get(keyGET_VCE_STATUS, value) == NO_ERROR)   // old name
    {
        param.remove(keyGET_VCE_STATUS);
        value = (SpeechEnhancementController::GetInstance()->GetDynamicMask(SPH_ENH_DYNAMIC_MASK_VCE) > 0) ? "1" : "0";
        returnParam.add(keyGET_VCE_STATUS, value);
        goto EXIT_GETPARAMETERS;
    }

    // Magic Conference Call
    if (param.get(keyGET_MAGIC_CON_CALL_ENABLE, value) == NO_ERROR)   // new name
    {
        param.remove(keyGET_MAGIC_CON_CALL_ENABLE);
        value = (SpeechEnhancementController::GetInstance()->GetMagicConferenceCallOn() == true) ? "1" : "0";
        returnParam.add(keyGET_MAGIC_CON_CALL_ENABLE, value);
        goto EXIT_GETPARAMETERS;
    }

    // Audio Volume version
        if (param.get(keyGET_AUDIO_VOLUME_VER, value) == NO_ERROR)
        {
            param.remove(keyGET_AUDIO_VOLUME_VER);
            value = "1";
            returnParam.add(keyGET_AUDIO_VOLUME_VER, value);
        goto EXIT_GETPARAMETERS;
    }

    // Get input Fsync flag
    if (param.getInt(keyGetFSyncFlag, cmdType) == NO_ERROR)
    {
        param.remove(keyGetFSyncFlag);
        if (mAudioMTKStreamManager->GetFSyncFlag(cmdType))
        {
            // 0 for input stream; 1 for output stream
            mAudioMTKStreamManager->ClearFSync(cmdType);
            value = "1";
        }
        else
        {
            value = "0";
        }
        returnParam.add(keyGetFSyncFlag, value);
        goto EXIT_GETPARAMETERS;
    }

EXIT_GETPARAMETERS:
    String8 keyValuePairs = returnParam.toString();
    ALOGD("-%s(), keyValuePairs = %s", __FUNCTION__, keyValuePairs.string());
    return keyValuePairs;
}

size_t AudioMTKHardware::getInputBufferSize(uint32_t sampleRate, int format, int channelCount)
{
    return mAudioMTKStreamManager->getInputBufferSize(sampleRate, format, channelCount);
}

status_t AudioMTKHardware::SetEMParameter(void *ptr, int len)
{
    ALOGD("%s() len [%d] sizeof [%d]", __FUNCTION__, len, sizeof(AUDIO_CUSTOM_PARAM_STRUCT));
    if (len == sizeof(AUDIO_CUSTOM_PARAM_STRUCT))
    {
        SetNBSpeechParamToNVRam((AUDIO_CUSTOM_PARAM_STRUCT *)ptr);
        SpeechEnhancementController::GetInstance()->SetNBSpeechParametersToAllModem((AUDIO_CUSTOM_PARAM_STRUCT *)ptr);
        mAudioVolumeInstance->initVolumeController();
        return NO_ERROR;
    }
    else
    {
        ALOGE("len [%d] != Sizeof(AUDIO_CUSTOM_PARAM_STRUCT) [%d] ", len, sizeof(AUDIO_CUSTOM_PARAM_STRUCT));
        return UNKNOWN_ERROR;
    }
}

status_t AudioMTKHardware::GetEMParameter(void *ptr , int len)
{
    ALOGD("%s() len [%d] sizeof [%d]", __FUNCTION__, len, sizeof(AUDIO_CUSTOM_PARAM_STRUCT));

    if (len == sizeof(AUDIO_CUSTOM_PARAM_STRUCT))
    {
        GetNBSpeechParamFromNVRam((AUDIO_CUSTOM_PARAM_STRUCT *)ptr);
        return NO_ERROR;
    }
    else
    {
        ALOGE("len [%d] != Sizeof(AUDIO_CUSTOM_PARAM_STRUCT) [%d] ", len, sizeof(AUDIO_CUSTOM_PARAM_STRUCT));
        return UNKNOWN_ERROR;
    }
}

bool AudioMTKHardware::UpdateOutputFIR(int mode , int index)
{
    ALOGD("%s(),  mode = %d, index = %d", __FUNCTION__, mode, index);

    // save index to MED with different mode.
    AUDIO_PARAM_MED_STRUCT eMedPara;
    GetMedParamFromNV(&eMedPara);
    eMedPara.select_FIR_output_index[mode] = index;

    // copy med data into audio_custom param
    AUDIO_CUSTOM_PARAM_STRUCT eSphParamNB;
    GetNBSpeechParamFromNVRam(&eSphParamNB);

    for (int i = 0; i < NB_FIR_NUM;  i++)
    {
        ALOGD("eSphParamNB.sph_out_fir[%d][%d] = %d <= eMedPara.speech_output_FIR_coeffs[%d][%d][%d] = %d",
              mode, i, eSphParamNB.sph_out_fir[mode][i],
              mode, index, i, eMedPara.speech_output_FIR_coeffs[mode][index][i]);
    }

    memcpy((void *)eSphParamNB.sph_out_fir[mode],
           (void *)eMedPara.speech_output_FIR_coeffs[mode][index],
           sizeof(eSphParamNB.sph_out_fir[mode]));

    // set to nvram
    SetNBSpeechParamToNVRam(&eSphParamNB);
    SetMedParamToNV(&eMedPara);

    // set to modem side
    SpeechEnhancementController::GetInstance()->SetNBSpeechParametersToAllModem(&eSphParamNB);

    return true;
}

status_t AudioMTKHardware::SetAudioCommand(int par1, int par2)
{
    ALOGD("%s(), par1 = 0x%x, par2 = %d", __FUNCTION__, par1, par2);

    char value[PROPERTY_VALUE_MAX];
    switch (par1)
    {
        case SETOUTPUTFIRINDEX:
        {
            UpdateOutputFIR(Normal_Coef_Index, par2);
            break;
        }
        case START_FMTX_SINEWAVE:
        {
            return NO_ERROR;
        }
        case STOP_FMTX_SINEWAVE:
        {
            return NO_ERROR;
        }
        case SETNORMALOUTPUTFIRINDEX:
        {
            UpdateOutputFIR(Normal_Coef_Index, par2);
            break;
        }
        case SETHEADSETOUTPUTFIRINDEX:
        {
            UpdateOutputFIR(Headset_Coef_Index, par2);
            break;
        }
        case SETSPEAKEROUTPUTFIRINDEX:
        {
            UpdateOutputFIR(Handfree_Coef_Index, par2);
            break;
        }
        case SET_LOAD_VOLUME_SETTING:
        {
            mAudioVolumeInstance->initVolumeController();
            setMasterVolume(mAudioVolumeInstance->getMasterVolume());
            const sp<IAudioPolicyService> &aps = AudioSystem::get_audio_policy_service();
            aps->SetPolicyManagerParameters(POLICY_LOAD_VOLUME, 0, 0, 0);
            break;
        }
        case SET_SPEECH_VM_ENABLE:
        {
            ALOGD("+SET_SPEECH_VM_ENABLE(%d)", par2);
            AUDIO_CUSTOM_PARAM_STRUCT eSphParamNB;
            GetNBSpeechParamFromNVRam(&eSphParamNB);
            if (par2 == 0)   // normal VM
            {
                eSphParamNB.debug_info[0] = 0;
            }
            else   // EPL
            {
                eSphParamNB.debug_info[0] = 3;
                if (eSphParamNB.speech_common_para[0] == 0)   // if not assign EPL debug type yet, set a default one
                {
                    eSphParamNB.speech_common_para[0] = 6;
                }
            }
            SetNBSpeechParamToNVRam(&eSphParamNB);
            SpeechEnhancementController::GetInstance()->SetNBSpeechParametersToAllModem(&eSphParamNB);
            ALOGD("-SET_SPEECH_VM_ENABLE(%d)", par2);
            break;
        }
        case SET_DUMP_SPEECH_DEBUG_INFO:
        {
            ALOGD(" SET_DUMP_SPEECH_DEBUG_INFO(%d)", par2);
            mSpeechDriverFactory->GetSpeechDriver()->ModemDumpSpeechParam();
            break;
        }
        case SET_DUMP_AUDIO_DEBUG_INFO:
        {
            ALOGD(" SET_DUMP_AUDIO_DEBUG_INFO(%d)", par2);
            ::ioctl(mFd, AUDDRV_LOG_PRINT, 0);
            mAudioDigitalInstance->EnableSideToneHw(AudioDigitalType::O03 , false, true);
            sleep(3);
            mAudioDigitalInstance->EnableSideToneHw(AudioDigitalType::O03 , false, false);
            break;
        }
        case SET_DUMP_AUDIO_AEE_CHECK:
        {
            ALOGD(" SET_DUMP_AUDIO_AEE_CHECK(%d)", par2);
            if (par2 == 0)
            {
                property_set("streamout.aee.dump", "0");
            }
            else
            {
                property_set("streamout.aee.dump", "1");
            }
            break;
        }
        case SET_DUMP_AUDIO_STREAM_OUT:
        {
            ALOGD(" SET_DUMP_AUDIO_STREAM_OUT(%d)", par2);
            if (par2 == 0)
            {
                property_set("streamout.pcm.dump", "0");
                ::ioctl(mFd, AUDDRV_AEE_IOCTL, 0);
            }
            else
            {
                property_set("streamout.pcm.dump", "1");
                ::ioctl(mFd, AUDDRV_AEE_IOCTL, 1);
            }
            break;
        }
        case SET_DUMP_AUDIO_MIXER_BUF:
        {
            ALOGD(" SET_DUMP_AUDIO_MIXER_BUF(%d)", par2);
            if (par2 == 0)
            {
                property_set("af.mixer.pcm", "0");
            }
            else
            {
                property_set("af.mixer.pcm", "1");
            }
            break;
        }
        case SET_DUMP_AUDIO_TRACK_BUF:
        {
            ALOGD(" SET_DUMP_AUDIO_TRACK_BUF(%d)", par2);
            if (par2 == 0)
            {
                property_set("af.track.pcm", "0");
            }
            else
            {
                property_set("af.track.pcm", "1");
            }
            break;
        }
        case SET_DUMP_A2DP_STREAM_OUT:
        {
            ALOGD(" SET_DUMP_A2DP_STREAM_OUT(%d)", par2);
            if (par2 == 0)
            {
                property_set("a2dp.streamout.pcm", "0");
            }
            else
            {
                property_set("a2dp.streamout.pcm", "1");
            }
            break;
        }
        case SET_DUMP_AUDIO_STREAM_IN:
        {
            ALOGD(" SET_DUMP_AUDIO_STREAM_IN(%d)", par2);
            if (par2 == 0)
            {
                property_set("streamin.pcm.dump", "0");
            }
            else
            {
                property_set("streamin.pcm.dump", "1");
            }
            break;
        }
        case SET_DUMP_IDLE_VM_RECORD:
        {
            ALOGD(" SET_DUMP_IDLE_VM_RECORD(%d)", par2);
#if defined(MTK_AUDIO_HD_REC_SUPPORT)
            if (par2 == 0)
            {
                property_set("streamin.vm.dump", "0");
            }
            else
            {
                property_set("streamin.vm.dump", "1");
            }
#endif
            break;
        }
        case AUDIO_USER_TEST:
        {
            if (par2 == 0)
            {
                mAudioFtmInstance->EarphoneTest(true);
            }
            else if (par2 == 1)
            {
                mAudioFtmInstance->EarphoneTest(false);
            }
            else if (par2 == 2)
            {
                mAudioFtmInstance->FTMPMICLoopbackTest(true);
            }
            else if (par2 == 3)
            {
                mAudioFtmInstance->FTMPMICLoopbackTest(false);
            }
            else if (par2 == 4)
            {
                mAudioFtmInstance->LouderSPKTest(true, true);
            }
            else if (par2 == 5)
            {
                mAudioFtmInstance->LouderSPKTest(false, false);
            }
            else if (par2 == 6)
            {
                mAudioFtmInstance->RecieverTest(true);
            }
            else if (par2 == 7)
            {
                mAudioFtmInstance->RecieverTest(false);
            }
            else if (par2 == 8)
            {
                mAudioFtmInstance->FTMPMICEarpieceLoopbackTest(true);
            }
            else if (par2 == 9)
            {
                mAudioFtmInstance->FTMPMICEarpieceLoopbackTest(false);
            }
            else if (par2 == 0x10)
            {
                mAudioFtmInstance->FTMPMICDualModeLoopbackTest(true);
            }
            else if (par2 == 0x11)
            {
                mAudioFtmInstance->FTMPMICDualModeLoopbackTest(false);
            }
            else if (par2 == 0x12)
            {
                //mAudioFtmInstance->PhoneMic_EarphoneLR_Loopback(true);
                LoopbackManager::GetInstance()->SetLoopbackOn(AP_MAIN_MIC_AFE_LOOPBACK, LOOPBACK_OUTPUT_EARPHONE);
            }
            else if (par2 == 0x13)
            {
                //mAudioFtmInstance->PhoneMic_EarphoneLR_Loopback(false);
                LoopbackManager::GetInstance()->SetLoopbackOff();
            }
            else if (par2 == 0x14)
            {
                mAudioFtmInstance->Pmic_I2s_out(true);
            }
            else if (par2 == 0x15)
            {
                mAudioFtmInstance->Pmic_I2s_out(false);
            }
            else if (par2 == 0x16)
            {
                mAudioFtmInstance->FMLoopbackTest(true);
            }
            else if (par2 == 0x17)
            {
                mAudioFtmInstance->FMLoopbackTest(false);
            }
            else if (par2 == 0x18)
            {
                //mAudioFtmInstance->PhoneMic_Receiver_Loopback(true);
                LoopbackManager::GetInstance()->SetLoopbackOn(AP_MAIN_MIC_AFE_LOOPBACK, LOOPBACK_OUTPUT_RECEIVER);
            }
            else if (par2 == 0x19)
            {
                //mAudioFtmInstance->PhoneMic_Receiver_Loopback(false);
                LoopbackManager::GetInstance()->SetLoopbackOff();
            }
            else if (par2 == 0x20)   // same as 0x12 ??
            {
                //mAudioFtmInstance->PhoneMic_EarphoneLR_Loopback(true);
                LoopbackManager::GetInstance()->SetLoopbackOn(AP_MAIN_MIC_AFE_LOOPBACK, LOOPBACK_OUTPUT_EARPHONE);
            }
            else if (par2 == 0x21)   // same as 0x13 ??
            {
                //mAudioFtmInstance->PhoneMic_EarphoneLR_Loopback(false);
                LoopbackManager::GetInstance()->SetLoopbackOff();
            }
            else if (par2 == 0x22)
            {
                //mAudioFtmInstance->PhoneMic_SpkLR_Loopback(true);
                LoopbackManager::GetInstance()->SetLoopbackOn(AP_MAIN_MIC_AFE_LOOPBACK, LOOPBACK_OUTPUT_SPEAKER);
            }
            else if (par2 == 0x23)
            {
                //mAudioFtmInstance->PhoneMic_SpkLR_Loopback(false);
                LoopbackManager::GetInstance()->SetLoopbackOff();
            }
            else if (par2 == 0x24)
            {
                //mAudioFtmInstance->HeadsetMic_EarphoneLR_Loopback(true, true);
                LoopbackManager::GetInstance()->SetLoopbackOn(AP_HEADSET_MIC_AFE_LOOPBACK, LOOPBACK_OUTPUT_EARPHONE);
            }
            else if (par2 == 0x25)
            {
                //mAudioFtmInstance->HeadsetMic_EarphoneLR_Loopback(false, false);
                LoopbackManager::GetInstance()->SetLoopbackOff();
            }
            else if (par2 == 0x26)
            {
                //mAudioFtmInstance->HeadsetMic_SpkLR_Loopback(true);
                LoopbackManager::GetInstance()->SetLoopbackOn(AP_HEADSET_MIC_AFE_LOOPBACK, LOOPBACK_OUTPUT_SPEAKER);
            }
            else if (par2 == 0x27)
            {
                //mAudioFtmInstance->HeadsetMic_SpkLR_Loopback(false);
                LoopbackManager::GetInstance()->SetLoopbackOff();
            }
            else if (par2 == 0x28)
            {
                //mAudioFtmInstance->HeadsetMic_Receiver_Loopback(true, true);
                LoopbackManager::GetInstance()->SetLoopbackOn(AP_HEADSET_MIC_AFE_LOOPBACK, LOOPBACK_OUTPUT_RECEIVER);
            }
            else if (par2 == 0x29)
            {
                //mAudioFtmInstance->HeadsetMic_Receiver_Loopback(false, false);
                LoopbackManager::GetInstance()->SetLoopbackOff();
            }
            else if (par2 == 0x30)
            {
                mAudioResourceManager->StartInputDevice();
                mAudioResourceManager->StartOutputDevice();
                ALOGD("2 0x30 sleep");
                sleep(3);
            }
            else if (par2 == 0x31)
            {
                mAudioResourceManager->StopOutputDevice();
                mAudioResourceManager->StopInputDevice();
            }
            break;
        }
        case AUDIO_SET_PMIC_REG:
        {
            uint32 temp1 = par2;
            uint32 temp2 = (temp1 & 0xffff0000) >> 16; // offset
            uint32 temp3 = temp1 & 0x0000ffff;        //value
            ALOGD(" AUDIO_SET_PMIC_REG par2 = 0x%x temp2 = 0x%x temp3 = 0x%x temp1 = 0x%x",
                  par2, temp2, temp3, temp1);
            mAudioAnaRegInstance->SetAnalogReg(temp2, temp3, 0xffff);
            break;
        }
        default:
            break;
    }
    return NO_ERROR;
}

status_t AudioMTKHardware::GetAudioCommand(int parameters1)
{
    ALOGD("GetAudioCommand parameters1 = %d ", parameters1);
    int result = 0 ;
    char value[PROPERTY_VALUE_MAX];
    switch (parameters1)
    {
        case GETOUTPUTFIRINDEX:
        {
            AUDIO_PARAM_MED_STRUCT pMedPara;
            GetMedParamFromNV(&pMedPara);
            result = pMedPara.select_FIR_output_index[Normal_Coef_Index];
            break;
        }
        case GETAUDIOCUSTOMDATASIZE:
        {
            int AudioCustomDataSize = sizeof(AUDIO_VOLUME_CUSTOM_STRUCT);
            ALOGD("GETAUDIOCUSTOMDATASIZE  AudioCustomDataSize = %d", AudioCustomDataSize);
            return AudioCustomDataSize;
        }
        case GETNORMALOUTPUTFIRINDEX:
        {
            AUDIO_PARAM_MED_STRUCT pMedPara;
            GetMedParamFromNV(&pMedPara);
            result = pMedPara.select_FIR_output_index[Normal_Coef_Index];
            break;
        }
        case GETHEADSETOUTPUTFIRINDEX:
        {
            AUDIO_PARAM_MED_STRUCT pMedPara;
            GetMedParamFromNV(&pMedPara);
            result = pMedPara.select_FIR_output_index[Headset_Coef_Index];
            break;
        }
        case GETSPEAKEROUTPUTFIRINDEX:
        {
            AUDIO_PARAM_MED_STRUCT pMedPara;
            GetMedParamFromNV(&pMedPara);
            result = pMedPara.select_FIR_output_index[Handfree_Coef_Index];
            break;
        }
        case GET_DUMP_AUDIO_AEE_CHECK:
        {
            property_get("streamout.aee.dump", value, "0");
            result = atoi(value);
            ALOGD(" GET_DUMP_AUDIO_STREAM_OUT=%d", result);
            break;
        }
        case GET_DUMP_AUDIO_STREAM_OUT:
        {
            property_get("streamout.pcm.dump", value, "0");
            result = atoi(value);
            ALOGD(" GET_DUMP_AUDIO_STREAM_OUT=%d", result);
            break;
        }
        case GET_DUMP_AUDIO_MIXER_BUF:
        {
            property_get("af.mixer.pcm", value, "0");
            result = atoi(value);
            ALOGD(" GET_DUMP_AUDIO_MIXER_BUF=%d", result);
            break;
        }
        case GET_DUMP_AUDIO_TRACK_BUF:
        {
            property_get("af.track.pcm", value, "0");
            result = atoi(value);
            ALOGD(" GET_DUMP_AUDIO_TRACK_BUF=%d", result);
            break;
        }
        case GET_DUMP_A2DP_STREAM_OUT:
        {
            property_get("a2dp.streamout.pcm", value, "0");
            result = atoi(value);
            ALOGD(" GET_DUMP_A2DP_STREAM_OUT=%d", result);
            break;
        }
        case GET_DUMP_AUDIO_STREAM_IN:
        {
            property_get("streamin.pcm.dump", value, "0");
            result = atoi(value);
            ALOGD(" GET_DUMP_AUDIO_STREAM_IN=%d", result);
            break;
        }
        case GET_DUMP_IDLE_VM_RECORD:
        {
#if defined(MTK_AUDIO_HD_REC_SUPPORT)
            property_get("streamin.vm.dump", value, "0");
            result = atoi(value);
#else
            result = 0;
#endif
            ALOGD(" GET_DUMP_IDLE_VM_RECORD=%d", result);
            break;
        }
        default:
        {
            ALOGD(" GetAudioCommand: Unknown command\n");
            break;
        }
    }
    // call fucntion want to get status adn return it.
    return result;
}


status_t AudioMTKHardware::SetAudioData(int par1, size_t len, void *ptr)
{
    ALOGD("%s(), par1 = 0x%x, len = %d", __FUNCTION__, par1, len);
    switch (par1)
    {
        case HOOK_FM_DEVICE_CALLBACK:
        {
            AudioFMController::GetInstance()->SetFmDeviceCallback((AUDIO_DEVICE_CHANGE_CALLBACK_STRUCT *)ptr);
            break;
        }
        case UNHOOK_FM_DEVICE_CALLBACK:
        {
            AudioFMController::GetInstance()->SetFmDeviceCallback(NULL);
            break;
        }
        case SETMEDDATA:
        {
            ASSERT(len == sizeof(AUDIO_PARAM_MED_STRUCT));
            SetMedParamToNV((AUDIO_PARAM_MED_STRUCT *)ptr);
            break;
        }
        case SETAUDIOCUSTOMDATA:
        {
            ASSERT(len == sizeof(AUDIO_VOLUME_CUSTOM_STRUCT));
            SetAudioCustomParamToNV((AUDIO_VOLUME_CUSTOM_STRUCT *)ptr);
            mAudioVolumeInstance->initVolumeController();
            setMasterVolume(mAudioVolumeInstance->getMasterVolume());
            break;
        }
#if defined(MTK_DUAL_MIC_SUPPORT)
        case SET_DUAL_MIC_PARAMETER:
        {
            ASSERT(len == sizeof(AUDIO_CUSTOM_EXTRA_PARAM_STRUCT));
            SetDualMicSpeechParamToNVRam((AUDIO_CUSTOM_EXTRA_PARAM_STRUCT *)ptr);
            mAudioVolumeInstance->initVolumeController();
            SpeechEnhancementController::GetInstance()->SetDualMicSpeechParametersToAllModem((AUDIO_CUSTOM_EXTRA_PARAM_STRUCT *)ptr);
            break;
        }
#endif

#if defined(MTK_WB_SPEECH_SUPPORT)
        case SET_WB_SPEECH_PARAMETER:
        {
            ASSERT(len == sizeof(AUDIO_CUSTOM_WB_PARAM_STRUCT));
            SetWBSpeechParamToNVRam((AUDIO_CUSTOM_WB_PARAM_STRUCT *)ptr);
            SpeechEnhancementController::GetInstance()->SetWBSpeechParametersToAllModem((AUDIO_CUSTOM_WB_PARAM_STRUCT *)ptr);
            mAudioVolumeInstance->initVolumeController();
            break;
        }
#endif
        case SET_AUDIO_VER1_DATA:
        {
            ASSERT(len == sizeof(AUDIO_VER1_CUSTOM_VOLUME_STRUCT));
            SetVolumeVer1ParamToNV((AUDIO_VER1_CUSTOM_VOLUME_STRUCT *)ptr);
            mAudioVolumeInstance->initVolumeController();
            setMasterVolume(mAudioVolumeInstance->getMasterVolume());
            const sp<IAudioPolicyService> &aps = AudioSystem::get_audio_policy_service();
            aps->SetPolicyManagerParameters(POLICY_LOAD_VOLUME, 0, 0, 0);
            break;
        }

        // for Audio Taste Tuning
        case AUD_TASTE_TUNING:
        {
#if 0
            status_t ret = NO_ERROR;
            AudioTasteTuningStruct audioTasteTuningParam;
            memcpy((void *)&audioTasteTuningParam, ptr, sizeof(AudioTasteTuningStruct));

            switch (audioTasteTuningParam.cmd_type)
            {
                case AUD_TASTE_STOP:
                {

                    mAudioTuningInstance->enableModemPlaybackVIASPHPROC(false);
                    audioTasteTuningParam.wb_mode = mAudioTuningInstance->m_bWBMode;
                    mAudioTuningInstance->updataOutputFIRCoffes(&audioTasteTuningParam);

                    break;
                }
                case AUD_TASTE_START:
                {

                    mAudioTuningInstance->setMode(audioTasteTuningParam.phone_mode);
                    ret = mAudioTuningInstance->setPlaybackFileName(audioTasteTuningParam.input_file);
                    if (ret != NO_ERROR)
                    {
                        return ret;
                    }
                    ret = mAudioTuningInstance->setDLPGA((uint32) audioTasteTuningParam.dlPGA);
                    if (ret != NO_ERROR)
                    {
                        return ret;
                    }
                    mAudioTuningInstance->updataOutputFIRCoffes(&audioTasteTuningParam);
                    ret = mAudioTuningInstance->enableModemPlaybackVIASPHPROC(true, audioTasteTuningParam.wb_mode);
                    if (ret != NO_ERROR)
                    {
                        return ret;
                    }

                    break;
                }
                case AUD_TASTE_DLDG_SETTING:
                case AUD_TASTE_INDEX_SETTING:
                {
                    mAudioTuningInstance->updataOutputFIRCoffes(&audioTasteTuningParam);
                    break;
                }
                case AUD_TASTE_DLPGA_SETTING:
                {
                    mAudioTuningInstance->setMode(audioTasteTuningParam.phone_mode);
                    ret = mAudioTuningInstance->setDLPGA((uint32) audioTasteTuningParam.dlPGA);
                    if (ret != NO_ERROR)
                    {
                        return ret;
                    }

                    break;
                }
                default:
                    break;
            }
#endif
            break;
        }
        default:
            break;
    }
    return NO_ERROR;
}


status_t AudioMTKHardware::GetAudioData(int par1, size_t len, void *ptr)
{
    ALOGD("GetAudioData par1=%d, len=%d", par1, len);
    switch (par1)
    {
        case GETMEDDATA:
        {
            ASSERT(len == sizeof(AUDIO_PARAM_MED_STRUCT));
            GetMedParamFromNV((AUDIO_PARAM_MED_STRUCT *)ptr);
            break;
        }
        case GETAUDIOCUSTOMDATA:
        {
            ASSERT(len == sizeof(AUDIO_VOLUME_CUSTOM_STRUCT));
            GetAudioCustomParamFromNV((AUDIO_VOLUME_CUSTOM_STRUCT *)ptr);
            break;
        }
#if defined(MTK_DUAL_MIC_SUPPORT)
        case GET_DUAL_MIC_PARAMETER:
        {
            ASSERT(len == sizeof(AUDIO_CUSTOM_EXTRA_PARAM_STRUCT));
            GetDualMicSpeechParamFromNVRam((AUDIO_CUSTOM_EXTRA_PARAM_STRUCT *)ptr);
            break;
        }
#endif
#if defined(MTK_WB_SPEECH_SUPPORT)
        case GET_WB_SPEECH_PARAMETER:
        {
            ASSERT(len == sizeof(AUDIO_CUSTOM_WB_PARAM_STRUCT));
            GetWBSpeechParamFromNVRam((AUDIO_CUSTOM_WB_PARAM_STRUCT *) ptr);
            break;
        }
#endif
#if defined(MTK_AUDIO_GAIN_TABLE)
        case GET_GAIN_TABLE_CTRPOINT_NUM:
        {
            int *p = (int *)ptr ;
            if (mAuioDevice != NULL)
            {
                *p = mAuioDevice->getParameters(AUD_AMP_GET_CTRP_NUM, 0, NULL);
            }
            break;
        }
        case GET_GAIN_TABLE_CTRPOINT_BITS:
        {
            int *point = (int *)ptr ;
            int *value = point + 1;
            if (mAuioDevice != NULL)
            {
                *value = mAuioDevice->getParameters(AUD_AMP_GET_CTRP_BITS, *point, NULL);
            }
            LOG_HARDWARE("GetAudioData GET_GAIN_TABLE_CTRPOINT_BITS point %d, value %d", *point, *value);
            break;
        }
        case GET_GAIN_TABLE_CTRPOINT_TABLE:
        {
            char *point = (char *)ptr ;
            int value = *point;
            if (mAuioDevice != NULL)
            {
                mAuioDevice->getParameters(AUD_AMP_GET_CTRP_TABLE, value, ptr);
            }
            break;
        }
#endif
        case GET_AUDIO_VER1_DATA:
        {
            GetVolumeVer1ParamFromNV((AUDIO_VER1_CUSTOM_VOLUME_STRUCT *) ptr);
            break;
        }
        case GET_VOICE_CUST_PARAM:
        {
            GetVoiceRecogCustParamFromNV((VOICE_RECOGNITION_PARAM_STRUCT *)ptr);
            break;
        }
        case GET_VOICE_FIR_COEF:
        {
            AUDIO_HD_RECORD_PARAM_STRUCT custHDRECParam;
            GetHdRecordParamFromNV(&custHDRECParam);
            ASSERT(len == sizeof(custHDRECParam.hd_rec_fir));
            memcpy(ptr, (void *)custHDRECParam.hd_rec_fir, len);
            break;
        }
        case GET_VOICE_GAIN:
        {
            AUDIO_VER1_CUSTOM_VOLUME_STRUCT custGainParam;
            GetVolumeVer1ParamFromNV(&custGainParam);
            uint16_t *pGain = (uint16_t *)ptr;
            *pGain = mAudioVolumeInstance->MappingToDigitalGain(custGainParam.audiovolume_mic[VOLUME_NORMAL_MODE][7]);
            *(pGain + 1) = mAudioVolumeInstance->MappingToDigitalGain(custGainParam.audiovolume_mic[VOLUME_HEADSET_MODE][7]);
            break;
        }
        default:
            break;
    }
    return NO_ERROR;
}



// ACF Preview parameter
status_t AudioMTKHardware::SetACFPreviewParameter(void *ptr , int len)
{
    ALOGD("AudioMTKHardware SetACFPreviewParameter\n");
    mAudioMTKStreamManager->SetACFPreviewParameter(ptr, len);
    return NO_ERROR;
}
status_t AudioMTKHardware::SetHCFPreviewParameter(void *ptr , int len)
{
    ALOGD("AudioMTKHardware SetHCFPreviewParameter\n");
    mAudioMTKStreamManager->SetHCFPreviewParameter(ptr, len);
    return NO_ERROR;
}

//for PCMxWay Interface API
int AudioMTKHardware::xWayPlay_Start(int sample_rate)
{
    ALOGV("AudioMTKHardware xWayPlay_Start");
    return Play2Way::GetInstance()->Start();
}

int AudioMTKHardware::xWayPlay_Stop(void)
{
    ALOGV("AudioMTKHardware xWayPlay_Stop");
    return Play2Way::GetInstance()->Stop();
}

int AudioMTKHardware::xWayPlay_Write(void *buffer, int size_bytes)
{
    ALOGV("AudioMTKHardware xWayPlay_Write");
    return Play2Way::GetInstance()->Write(buffer, size_bytes);
}

int AudioMTKHardware::xWayPlay_GetFreeBufferCount(void)
{
    ALOGV("AudioMTKHardware xWayPlay_GetFreeBufferCount");
    return Play2Way::GetInstance()->GetFreeBufferCount();
}

int AudioMTKHardware::xWayRec_Start(int sample_rate)
{
    ALOGV("AudioMTKHardware xWayRec_Start");
    return Record2Way::GetInstance()->Start();
}

int AudioMTKHardware::xWayRec_Stop(void)
{
    ALOGV("AudioMTKHardware xWayRec_Stop");
    return Record2Way::GetInstance()->Stop();
}

int AudioMTKHardware::xWayRec_Read(void *buffer, int size_bytes)
{
    ALOGV("AudioMTKHardware xWayRec_Read");
    return Record2Way::GetInstance()->Read(buffer, size_bytes);
}

//add by wendy
int AudioMTKHardware::ReadRefFromRing(void *buf, uint32_t datasz, void *DLtime)
{
    AudioVUnlockDL *VInstance = AudioVUnlockDL::getInstance();
    return VInstance->ReadRefFromRing(buf, datasz, DLtime);
}
int AudioMTKHardware::GetVoiceUnlockULTime(void *DLtime)
{
    AudioVUnlockDL *VInstance = AudioVUnlockDL::getInstance();
    return VInstance->GetVoiceUnlockULTime(DLtime);
}

int AudioMTKHardware::SetVoiceUnlockSRC(uint outSR, uint outChannel)
{
    AudioVUnlockDL *VInstance = AudioVUnlockDL::getInstance();
    return VInstance->SetSRC(outSR, outChannel);
}
bool AudioMTKHardware::startVoiceUnlockDL()
{
    AudioVUnlockDL *VInstance = AudioVUnlockDL::getInstance();
    return VInstance->startInput();
}
bool AudioMTKHardware::stopVoiceUnlockDL()
{
    AudioVUnlockDL *VInstance = AudioVUnlockDL::getInstance();
    return VInstance->stopInput();
}
void AudioMTKHardware::freeVoiceUnlockDLInstance()
{
    AudioVUnlockDL::freeInstance();
    return ;
}
bool AudioMTKHardware::getVoiceUnlockDLInstance()
{
    AudioVUnlockDL *VInstance = AudioVUnlockDL::getInstance();
    if (VInstance != NULL)
    {
        return true;
    }
    else
    {
        return false;
    }
}
int AudioMTKHardware::GetVoiceUnlockDLLatency()
{
    AudioVUnlockDL *VInstance = AudioVUnlockDL::getInstance();
    return VInstance->GetLatency();
}
status_t AudioMTKHardware::initCheck()
{
    ALOGD("AudioMTKHardware initCheck\n");
    return NO_ERROR;
}

status_t AudioMTKHardware::setVoiceVolume(float volume)
{
    ALOGD("AudioMTKHardware setVoiceVolume volume = %f\n", volume);
    if ((volume > 1) || (volume < 0))
    {
        ALOGE("AudioMTKHardware setVoiceVolume strange volume level, something wrong!!\n");
        return NO_ERROR;
    }
    mAudioVolumeInstance->setVoiceVolume(volume, mMode, mAudioResourceManager->getDlOutputDevice());
    return NO_ERROR;
}

status_t AudioMTKHardware::setMasterVolume(float volume)
{
    ALOGD("AudioMTKHardware setMasterVolume volume = %f", volume);
    if ((volume > 1) || (volume < 0))
    {
        ALOGE("AudioMTKHardware setMasterVolume strange volume level, something wrong!!\n");
        return NO_ERROR;
    }
    mAudioVolumeInstance->setMasterVolume(volume, mMode, mAudioResourceManager->getDlOutputDevice());
    return NO_ERROR;
}

status_t AudioMTKHardware::ForceAllStandby()
{
    mAudioMTKStreamManager->ForceAllStandby();
    return NO_ERROR;
}

status_t AudioMTKHardware::SetOutputSuspend(bool bEnable)
{
    ALOGD("SetOutputSuspend bEnable = %d", bEnable);
    mAudioMTKStreamManager->SetOutputStreamSuspend(bEnable);
    return NO_ERROR;
}
status_t AudioMTKHardware::SetInputSuspend(bool bEnable)
{
    ALOGD("SetInputSuspend bEnable = %d", bEnable);
    mAudioMTKStreamManager->SetInputStreamSuspend(bEnable);
    return NO_ERROR;
}




bool AudioMTKHardware::ModeInCall(audio_mode_t mode)
{
    return (mode == AUDIO_MODE_IN_CALL ||
            mode == AUDIO_MODE_IN_CALL_2);
}

bool AudioMTKHardware::ModeEnterCall(audio_mode_t Mode)
{
    return ((mMode == AUDIO_MODE_NORMAL ||
             mMode == AUDIO_MODE_RINGTONE ||
             mMode == AUDIO_MODE_IN_COMMUNICATION) &&
            (ModeInCall(Mode) == true));
}

bool AudioMTKHardware::ModeLeaveCall(audio_mode_t Mode)
{
    return ((ModeInCall(mMode)) &&
            (Mode  == AUDIO_MODE_NORMAL ||
             Mode  == AUDIO_MODE_RINGTONE ||
             Mode  == AUDIO_MODE_IN_COMMUNICATION));
}
bool AudioMTKHardware::ModeEnterSipCall(audio_mode_t Mode)
{
    return (mMode != AUDIO_MODE_IN_COMMUNICATION &&
            Mode  == AUDIO_MODE_IN_COMMUNICATION);
}

bool AudioMTKHardware::ModeLeaveSipCall(audio_mode_t Mode)
{
    return (mMode == AUDIO_MODE_IN_COMMUNICATION &&
            Mode  != AUDIO_MODE_IN_COMMUNICATION);
}


bool AudioMTKHardware::ModeCallSwitch(audio_mode_t Mode)
{
    return ((mMode == AUDIO_MODE_IN_CALL &&
             Mode  == AUDIO_MODE_IN_CALL_2) ||
            (mMode == AUDIO_MODE_IN_CALL_2 &&
             Mode  == AUDIO_MODE_IN_CALL));
}

bool AudioMTKHardware::InputStreamExist()
{
    return mAudioMTKStreamManager->InputStreamExist();
}

bool AudioMTKHardware::OutputStreamExist()
{
    return mAudioMTKStreamManager->OutputStreamExist();
}

void AudioMTKHardware::UpdateKernelState()
{
    ::ioctl(mFd, SET_AUDIO_STATE, &mAudio_Control_State);
}

bool AudioMTKHardware::StreamExist()
{
    return mAudioMTKStreamManager->StreamExist();
}

status_t AudioMTKHardware::setMode(int NewMode)
{
    audio_mode_t new_mode = (audio_mode_t)NewMode;
    ALOGD("+%s(), mode = %d", __FUNCTION__, new_mode);

    if ((new_mode < 0) || (new_mode > AUDIO_MODE_MAX))
    {
        return BAD_VALUE;
    }

#if !defined(MTK_ENABLE_MD1) && defined(MTK_ENABLE_MD2) // CTS want to set MODE_IN_CALL, but only modem 2 is available
    if (new_mode == AUDIO_MODE_IN_CALL)
    {
        ALOGE("There is no modem 1 in this project!! Set modem 2 instead!!");
        new_mode = AUDIO_MODE_IN_CALL_2;
    }
#elif !defined(MTK_ENABLE_MD1) && !defined(MTK_ENABLE_MD2) // CTS want to set MODE_IN_CALL, but wifi only project has no modem, just bypass it
    if (new_mode == AUDIO_MODE_IN_CALL)
    {
        ALOGE("There is no modem 1 & modem 2 in this project!! Just bypass AUDIO_MODE_IN_CALL!!");
        return NO_ERROR;
    }
#endif

    if (new_mode == mMode)
    {
        ALOGE("Newmode and Oldmode is the same!!!!");
        return INVALID_OPERATION;
    }


    // check input/output need suspend
    if (ModeEnterCall(new_mode) || ModeCallSwitch(new_mode) || ModeLeaveCall(new_mode))
    {
        SetOutputSuspend(true);
        SetInputSuspend(true);
    }


    // close FM when mode swiching
    AudioFMController *pAudioFMController = AudioFMController::GetInstance();
    if (pAudioFMController->GetFmEnable() == true)
    {
        pAudioFMController->SetFmEnable(false);
    }

    // close mATV when mode swiching
    AudioMATVController *pAudioMATVController = AudioMATVController::GetInstance();
    if (pAudioMATVController->GetMatvEnable() == true)
    {
        pAudioMATVController->SetMatvEnable(false, pAudioMATVController->GetMatvType());
    }

    // Lock
    mAudioResourceManager->EnableAudioLock(AudioResourceManagerInterface::AUDIO_HARDWARE_LOCK, 3000);
    mAudioResourceManager->EnableAudioLock(AudioResourceManagerInterface::AUDIO_MODE_LOCK, 3000);
    mAudioResourceManager->EnableAudioLock(AudioResourceManagerInterface::AUDIO_VOLUME_LOCK, 3000);

    SpeechPhoneCallController *pSpeechPhoneCallController = SpeechPhoneCallController::GetInstance();

    // check input/output need standby
    if (ModeEnterCall(new_mode) || ModeCallSwitch(new_mode) || ModeLeaveCall(new_mode)
        || ModeEnterSipCall(new_mode) || ModeLeaveSipCall(new_mode)
       )
    {
        ForceAllStandby();
    }

    if (ModeEnterCall(new_mode))
    {
        mAudio_Control_State.bSpeechFlag = true;
        UpdateKernelState();
    }

    // set to AudioReourceManager and get input and output device
    mAudioResourceManager->SetAudioMode(new_mode);

    // mMode is previous
    switch (mMode)
    {
        case AUDIO_MODE_NORMAL:
        {
            switch (new_mode)
            {
                case AUDIO_MODE_RINGTONE:         // Normal->Ringtone: MT call incoming. [but not accept yet]
                    break;
                case AUDIO_MODE_IN_CALL:          // Normal->Incall:  MD1 MO call dial out
                case AUDIO_MODE_IN_CALL_2:        // Normal->Incall2: MD2 MO call dial out
                {
                    pSpeechPhoneCallController->OpenModemSpeechControlFlow(new_mode);
                    break;
                }
                case AUDIO_MODE_IN_COMMUNICATION: // Normal->Incommunication: SIP MO call dial out
                    break;
            }
            break;
        }
        case AUDIO_MODE_RINGTONE:
        {
            switch (new_mode)
            {
                case AUDIO_MODE_NORMAL:           // Ringtone->Normal: MT call incoming, and reject. [no other call connected]
                    break;
                case AUDIO_MODE_IN_CALL:          // Ringtone->Incall:  Accept MD1 MT call
                case AUDIO_MODE_IN_CALL_2:        // Ringtone->Incall2: Accept MD2 MT call
                {
                    pSpeechPhoneCallController->OpenModemSpeechControlFlow(new_mode);
                    break;
                }
                case AUDIO_MODE_IN_COMMUNICATION: // Ringtone->Incommunication: Accept SIP MT call
                    break;
            }
            break;
        }
        case AUDIO_MODE_IN_CALL:
        case AUDIO_MODE_IN_CALL_2:
        {
            switch (new_mode)
            {
                case AUDIO_MODE_NORMAL:           // Incall_x->Normal: Hang up MDx call. [no other call connected]
                    pSpeechPhoneCallController->CloseModemSpeechControlFlow(mMode);
                    break;
                case AUDIO_MODE_RINGTONE:         // Incall_x->Ringtone: Accept another MT call
                    pSpeechPhoneCallController->CloseModemSpeechControlFlow(mMode);
                    break;
                case AUDIO_MODE_IN_CALL:          // Incall2->Incall : MD1 dail out & hold MD2, or Hang up MD2 and back to MD1
                case AUDIO_MODE_IN_CALL_2:        // Incall ->Incall2: MD2 dail out & hold MD1, or Hang up MD1 and back to MD2
                {
                    pSpeechPhoneCallController->CloseModemSpeechControlFlow(mMode);
                    pSpeechPhoneCallController->OpenModemSpeechControlFlow(new_mode);
                    break;
                }
                case AUDIO_MODE_IN_COMMUNICATION: // Incall_x->Incommunication: SIP call dail out & Hold MDx, or Hang up MDx and back to SIP call
                    pSpeechPhoneCallController->CloseModemSpeechControlFlow(mMode);
                    break;
            }
            break;
        }
        case AUDIO_MODE_IN_COMMUNICATION:
        {
            switch (new_mode)
            {
                case AUDIO_MODE_NORMAL:           // Incommunication->Normal: Hang  up SIP call. [no other call connected]
                    break;
                case AUDIO_MODE_RINGTONE:         // Incommunication->Ringtone: Accept another MT call
                    break;
                case AUDIO_MODE_IN_CALL:          // Incommunication->Incall: MD1 Dail out & Hold SIP call, or Hang up SIP call and back to MD1
                case AUDIO_MODE_IN_CALL_2:        // Incommunication->Incall2: MD2 Dail out & Hold SIP call, or Hang up SIP call and back to MD2
                {
                    pSpeechPhoneCallController->OpenModemSpeechControlFlow(new_mode);
                    break;
                }
            }
            break;
        }
        default:
            break;
    }

    if (ModeLeaveCall(new_mode))
    {
        mAudio_Control_State.bSpeechFlag = false;
        UpdateKernelState();
    }

    // check input/output need suspend_cb_table
    if (ModeEnterCall(new_mode) || ModeCallSwitch(new_mode) || ModeLeaveCall(new_mode)
        || ModeEnterSipCall(new_mode) || ModeLeaveSipCall(new_mode)
       )
    {
        SetOutputSuspend(false);
        SetInputSuspend(false);
    }

    mMode = new_mode;  // save mode when all things done.
    if (ModeInCall(mMode) == false)
    {
        setMasterVolume(mAudioVolumeInstance->getMasterVolume());
    }

    mAudioResourceManager->DisableAudioLock(AudioResourceManagerInterface::AUDIO_VOLUME_LOCK);
    mAudioResourceManager->DisableAudioLock(AudioResourceManagerInterface::AUDIO_MODE_LOCK);
    mAudioResourceManager->DisableAudioLock(AudioResourceManagerInterface::AUDIO_HARDWARE_LOCK);

    ALOGD("-%s(), mode = %d", __FUNCTION__, new_mode);
    return NO_ERROR;
}

status_t AudioMTKHardware::setMicMute(bool state)
{
    ALOGD("%s(), new state = %d, old mMicMute = %d", __FUNCTION__, state, mMicMute);
    if (ModeInCall(mMode) == true)   // modem phone call
    {
        SpeechPhoneCallController::GetInstance()->SetMicMute(state);
    }
    else   // SIP call
    {
        mStreamInManager->SetInputMute(state);
    }
    mMicMute = state;
    return NO_ERROR;
}

status_t AudioMTKHardware::getMicMute(bool *state)
{
    ALOGD("%s(), mMicMute = %d", __FUNCTION__, mMicMute);
    *state = mMicMute;
    return NO_ERROR;
}

android_audio_legacy::AudioStreamOut *AudioMTKHardware::openOutputStream(
    uint32_t devices,
    int *format,
    uint32_t *channels,
    uint32_t *sampleRate,
    status_t *status)
{
    return mAudioMTKStreamManager->openOutputStream(devices, format, channels, sampleRate, status);
}

void AudioMTKHardware::closeOutputStream(android_audio_legacy::AudioStreamOut *out)
{
    mAudioMTKStreamManager->closeOutputStream(out);
}

android_audio_legacy::AudioStreamIn *AudioMTKHardware::openInputStream(
    uint32_t devices,
    int *format,
    uint32_t *channels,
    uint32_t *sampleRate,
    status_t *status,
    android_audio_legacy::AudioSystem::audio_in_acoustics acoustics)
{
    ALOGD("openInputStream, devices = 0x%x format=0x%x ,channels=0x%x, rate=%d acoustics = 0x%x", devices, *format, *channels, *sampleRate, acoustics);
    return mAudioMTKStreamManager->openInputStream(devices, format,  channels,  sampleRate,  status, acoustics);
}

void AudioMTKHardware::closeInputStream(android_audio_legacy::AudioStreamIn *in)
{
    mAudioMTKStreamManager->closeInputStream(in);
}


status_t AudioMTKHardware::setMasterMute(bool muted)
{
    return INVALID_OPERATION;
}

int AudioMTKHardware::getAudioPort(struct audio_port *port)
{
    //TODO , I think the implementation is designed in aps.
    ALOGW("-%s Unsupport", __FUNCTION__);
    return INVALID_OPERATION;
}

int AudioMTKHardware::setAudioPortConfig(const struct audio_port_config *config)
{
    return INVALID_OPERATION;
}

int AudioMTKHardware::createAudioPatch(unsigned int num_sources,
                                        const struct audio_port_config *sources,
                                        unsigned int num_sinks,
                                        const struct audio_port_config *sinks,
                                        audio_patch_handle_t *handle)
{
    return INVALID_OPERATION;
}

int AudioMTKHardware::releaseAudioPatch(audio_patch_handle_t handle)
{
    return INVALID_OPERATION;
}

status_t AudioMTKHardware::getMasterVolume(float *volume)
{
    return INVALID_OPERATION;
}

android_audio_legacy::AudioStreamOut *AudioMTKHardware::openOutputStreamWithFlags(uint32_t devices,
                                          audio_output_flags_t flags,
                                          int *format,
                                          uint32_t *channels,
                                          uint32_t *sampleRate,
                                          status_t *status)
{
    return openOutputStream(devices, format, channels, sampleRate, status);
}

};


namespace android_audio_legacy
{

status_t AudioStreamOut::getPresentationPosition(uint64_t *frames, struct timespec *timestamp)
{
    return INVALID_OPERATION;
}


};

