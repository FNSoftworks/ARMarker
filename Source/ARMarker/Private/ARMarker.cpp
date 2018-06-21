// Fill out your copyright notice in the Description page of Project Settings.

#include "IARMarker.h"
#include "ARMarkerPrivatePCH.h"
#include "ARMarkerDevice.h"

#if PLATFORM_ANDROID
#include "../../../Core/Public/Android/AndroidApplication.h"
#include "../../../Launch/Public/Android/AndroidJNI.h"
#include <android/log.h>

#define LOG_TAG "CameraLOG"

int SetupJNICamera(JNIEnv* env);
JNIEnv* ENV = NULL;

static jmethodID AndroidThunkJava_CamStart;
static jmethodID AndroidThunkJava_CamStop;
static jmethodID AndroidThunkJava_UnpackData;

int FrameWidth = 320;
int FrameHeight = 240;
bool newFrame = false;
bool processing = false;

unsigned char* Buffer =  new unsigned char[FrameWidth*FrameHeight];
signed char* BufferTmp = new signed char[FrameWidth*FrameHeight];

#endif

bool processingTexture = false;

class FARMarker : public IARMarker{
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
};

IMPLEMENT_MODULE( FARMarker, ARMarker )

void FARMarker::StartupModule(){
#if PLATFORM_ANDROID
    JNIEnv* env = FAndroidApplication::GetJavaEnv();
    SetupJNICamera(env);
#endif
    
    TSharedPtr<FARMarkerDevice> ARMarkerStartup(new FARMarkerDevice);
    
    if (ARMarkerStartup->StartupDevice()) {
        ARMarkerDevice = ARMarkerStartup;
    }
    //TODO: error handling    if dll cannot be loaded
}

void FARMarker::ShutdownModule()
{
    if(ARMarkerDevice.IsValid())
    {
        ARMarkerDevice->ShutdownDevice();
        ARMarkerDevice = nullptr;
    }
    
#if PLATFORM_ANDROID
    FMemory::Free(Buffer);
    FMemory::Free(BufferTmp);
#endif
    
}

#if PLATFORM_ANDROID

int SetupJNICamera(JNIEnv* env)
{
    if (!env) return JNI_ERR;
    
    ENV = env;
    
    AndroidThunkJava_CamStart = FJavaWrapper::FindMethod(ENV, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_CamStart", "()V", false);
    if (!AndroidThunkJava_CamStart)
    {
        UE_LOG(LogTemp, Log, TEXT("ERROR: CamStart"));
        return JNI_ERR;
    }
    
    AndroidThunkJava_CamStop = FJavaWrapper::FindMethod(ENV, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_CamStop", "()V", false);
    if (!AndroidThunkJava_CamStop)
    {
        UE_LOG(LogTemp, Log, TEXT("ERROR: CamStop"));
        return JNI_ERR;
    }
    
    AndroidThunkJava_UnpackData = FJavaWrapper::FindMethod(ENV, FJavaWrapper::GameActivityClassID, "AndroidThunkJava_UnpackData", "()V", false);
    if (!AndroidThunkJava_UnpackData)
    {
        UE_LOG(LogTemp, Log, TEXT("ERROR: UnpackData"));
        return JNI_ERR;
    }
    
    return JNI_OK;
}
void AndroidThunkCpp_CamStart()
{
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, AndroidThunkJava_CamStart);
    }
}

void AndroidThunkCpp_CamStop()
{
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, AndroidThunkJava_CamStop);
    }
}

void AndroidThunkCpp_UnpackData()
{
    if (JNIEnv* Env = FAndroidApplication::GetJavaEnv())
    {
        FJavaWrapper::CallVoidMethod(Env, FJavaWrapper::GameActivityThis, AndroidThunkJava_UnpackData);
    }
}

extern "C" bool Java_com_epicgames_ue4_GameActivity_nativeCameraFrameArrived(JNIEnv* LocalJNIEnv, jobject LocalThiz, jint frameWidth, jint frameHeight, jbyteArray data)
{
    if (!processingTexture){
        int len = LocalJNIEnv->GetArrayLength(data);
        
        //Copy webcam data to the buffer
        LocalJNIEnv->GetByteArrayRegion(data, 0, len, reinterpret_cast<jbyte*>(BufferTmp));
        Buffer = (unsigned char*)BufferTmp;
        newFrame = true;
        //FPlatformMisc::LowLevelOutputDebugStringf(TEXT("Native Camera frame arrived: '%d %d'\n"), frameWidth, frameHeight);
    }
    
    return JNI_TRUE;
}
#endif

