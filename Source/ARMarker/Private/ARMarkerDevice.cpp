// Fill out your copyright notice in the Description page of Project Settings.

#include "ARMarkerDevice.h"
#include "ARMarkerPrivatePCH.h"
#include "Runtime/Core/Public/Misc/Paths.h"

#include <string>
#include <iostream>
#include <sstream>

#if PLATFORM_MAC || PLATFORM_IOS
#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>
#endif

//ARMarker General Log
DEFINE_LOG_CATEGORY(ARMarker);

using namespace std;

//<================="【VS VERSION】"====================>

#if PLATFORM_WINDOWS
#if (_MSC_VER >= 1900)

FILE _iob[] = { *stdin, *stdout, *stderr };

extern "C" FILE * __cdecl __iob_func(void)
{
    return _iob;
}

#endif
#endif

//-------------------------------------------------------------------------------------//

#if PLATFORM_ANDROID
#include "AndroidJNI.h"
#endif

#include "Core.h"

//-------------------------------------------------------------------------------------//

#if PLATFORM_ANDROID

extern void AndroidThunkCpp_CamStart();
extern void AndroidThunkCpp_CamStop();
extern void AndroidThunkCpp_UnpackData();

extern int FrameWidth;
extern int FrameHeight;

extern bool newFrame;
extern unsigned char* Buffer;

#endif

//<=================="【Değişkenler】"===================>

extern bool processingTexture;

ARHandle* FARMarkerDevice::arHandle;
ARPattHandle* FARMarkerDevice::arPattHandle;
AR3DHandle* FARMarkerDevice::ar3DHandle;

ARUint8* FARMarkerDevice::gARTImage = NULL;

int FARMarkerDevice::xsize;
int FARMarkerDevice::ysize;
int FARMarkerDevice::mWebcamResX = 640;
int FARMarkerDevice::mWebcamResY = 480;
double FARMarkerDevice::patt_width;
ARParamLT* FARMarkerDevice::gCparamLT;

FString FARMarkerDevice::DataPath;

//NFT
THREAD_HANDLE_T* FARMarkerDevice::threadHandle = NULL;
AR2HandleT* FARMarkerDevice::ar2Handle = NULL;
KpmHandle* FARMarkerDevice::kpmHandle = NULL;
int FARMarkerDevice::surfaceSetCount = 0;
AR2SurfaceSetT* FARMarkerDevice::surfaceSet[PAGES_MAX];


int FARMarkerDevice::filterSampleRate = 30;
int FARMarkerDevice::filterCutOffFreq = 15;
bool FARMarkerDevice::filterEnabled = true;

int FARMarkerDevice::debugMode = false;
int FARMarkerDevice::threshold;

//<==============="【FARMarkerDevice】"==================>

FARMarkerDevice::FARMarkerDevice()
{
    initiated = 0;
    patt_width = 80.0;
    gCparamLT = NULL;
    paused = false;
    
    patternDetectionMode = EArPatternDetectionMode::TEMPLATE_MATCHING_COLOR;
    deviceOrientation = EDeviceOrientation::LANDSCAPE;
    
    iPhoneLaunched = true;
    
    UE_LOG(ARMarker, Warning, TEXT("Ayarlar Kontrol Edildi."));
   
}

//<==============="【~FARMarkerDevice】"==================>

FARMarkerDevice::~FARMarkerDevice()
{
    UE_LOG(ARMarker, Warning, TEXT("Cihaz Kapatıldı"));
    Cleanup();
}

//<================="【StartupDevice】"====================>

bool FARMarkerDevice::StartupDevice(){
    UE_LOG(ARMarker, Warning, TEXT("Cihaz Başlatıldı."));
    
    //-------------------------------------------------------------------------------------//
    // VERI dizini yolu ayarları
    //-------------------------------------------------------------------------------------//
    
    //=======>
    //【ANDROID】
    //=======>
    
#if PLATFORM_ANDROID
    FString AbsoluteContentPath = GFilePathBase + TEXT("/UE4Game/") + FApp::GetGameName() + TEXT("/") + FString::Printf(TEXT("%s/Content/"), FApp::GetGameName());
    DataPath = AbsoluteContentPath + FString("ARMarker/");
#endif
    
    //=======>
    // 【WIN64】-【MAC OSX】
    //=======>
#if PLATFORM_MAC || PLATFORM_WINDOWS
    DataPath = FPaths::GamePluginsDir() + "ARMarker/Content/ARMarker/";
    DataPath = FPaths::ConvertRelativePathToFull(DataPath); //Absolute path
    UE_LOG(ARMarker,Warning,TEXT("ARMarker Data Path: %s"), *DataPath);
#endif
    
    //=======>
    // 【IPHONE】
    //=======>
    
#if PLATFORM_IOS
    DataPath = ConvertToIOSPath(FString::Printf(TEXT("%s"), FApp::GetGameName()).ToLower() + FString("ARMarker/Content/ARMarker/"), 0);
    DataPath = FPaths::ConvertRelativePathToFull(DataPath); //Absolute path
    
    UE_LOG(LogTemp, Log, TEXT("IOS Datapath:%s"),*DataPath);
#endif
    
    //Default texture size
    
#if PLATFORM_IOS
    int width = 320;
    int height = 240;
#endif
    
#if PLATFORM_MAC || PLATFORM_WINDOWS
    xsize = 640;
    ysize = 480;
#endif
    int pixelSize = 3;
    return true;
}

//<==============="【GET WEBCAM TEXTURE】"==================>

UTexture2D* FARMarkerDevice::GetWebcamTexture(){
    if(this->initiated)
    {
        return this->WebcamTexture;
    }
    else
    {
        return this->DummyTexture;
    }
}

//<==============="【ShutdownDevice】"==================>

void FARMarkerDevice::ShutdownDevice(){
    Cleanup();
}

//<================="【UpdateDevice】"====================>

void FARMarkerDevice::UpdateDevice(){
    if(initiated == false) return;
    
#if PLATFORM_ANDROID
    UpdateTextureAndroid();
#endif
    
#if PLATFORM_MAC || PLATFORM_IOS
    UpdateTextureApple();
#endif
    
#if PLATFORM_WINDOWS
    UpdateTextureWindows();
#endif
    
    switch (this->patternDetectionMode) {
            //TEMPLATE
        case EArPatternDetectionMode::TEMPLATE_MATCHING_COLOR: {
            if (this->Markers.Num() > 0) {
                DetectMarkers();
            }
        };
            break;
            
            //MATRIX
        case EArPatternDetectionMode::MATRIX_CODE_DETECTION: {
            if (this->MarkersMATRIX.Num() > 0) {
                DetectMarkersMATRIX();
            }
        };
            break;
    }
    //NFT
    if (this->MarkersNFT.Num() > 0 ) DetectMarkersNFT();
    }

//<================="【YUVtoRGB】"====================>

int FARMarkerDevice::YUVtoRGB(int y, int u, int v)
{
    int r, g, b;
    r = y + (int)1.402f*v;
    g = y - (int)(0.344f*u + 0.714f*v);
    b = y + (int)1.772f*u;
    r = r>255 ? 255 : r<0 ? 0 : r;
    g = g>255 ? 255 : g<0 ? 0 : g;
    b = b>255 ? 255 : b<0 ? 0 : b;
    return 0xff000000 | (b << 16) | (g << 8) | r;
}

//<================="【GET DEBUG TEXTURE】"====================>

uint8* FARMarkerDevice::GetDebugTexture(int width, int height){
    if (arHandle->labelInfo.bwImage != NULL) {
        RGBQUAD* WebcamRGBXDebug = new RGBQUAD[width*height]; //Will be deleted after texture update!!!
        
        //Debug image
        uint8* Src = (uint8*)arHandle->labelInfo.bwImage;
        
        int u = 0;
        for (int i = 0; i < (width * height); i += 1) {
            
            WebcamRGBXDebug[u].rgbRed = Src[i];
            WebcamRGBXDebug[u].rgbGreen = Src[i];
            WebcamRGBXDebug[u].rgbBlue = Src[i];
            WebcamRGBXDebug[u].rgbReserved = 255;
            u++;
        }
        return (uint8*) WebcamRGBXDebug;
    }
    else
    {
        return NULL;
    }
}

//<================="【UPDATE TEXTURE WİNDOWS】"====================>

void FARMarkerDevice::UpdateTextureWindows(){
#if PLATFORM_WINDOWS
    ARUint8 *image;
    
    if ((image = arVideoGetImage()) == NULL) return;
    
    const size_t Size = arHandle->xsize * arHandle->ysize* arHandle->arPixelSize;
    gARTImage = image;
    
    //Region
    FUpdateTextureRegion2D* region = new FUpdateTextureRegion2D(0, 0, 0, 0, xsize, ysize);
    
    uint8* debugBuffer = GetDebugTexture(arHandle->xsize, arHandle->ysize);
    
    if (debugBuffer != NULL) {
        //Debug image
        UpdateTexture(WebcamTexture, 0, 1, region, (uint32)(4 * arHandle->xsize), 4, debugBuffer, true);
    }
    else {
        //Color image
        int u = 0;
        for (int i = 0; i < Size; i += arHandle->arPixelSize) {
            WebcamRGBX[u].rgbRed = image[i + 2];
            WebcamRGBX[u].rgbGreen = image[i + 1];
            WebcamRGBX[u].rgbBlue = image[i];
            WebcamRGBX[u].rgbReserved = 255;
            u++;
        }
        
        UpdateTexture(WebcamTexture, 0, 1, region, (uint32)(4 * xsize), 4, (uint8*)WebcamRGBX, false);
    }
#endif
}

//<==============="【UPDATE TEXTURE ANDROID】"==================>

void FARMarkerDevice::UpdateTextureAndroid(){
#if PLATFORM_ANDROID
    if (newFrame == false) return;
    processingTexture = true;
    int width = FrameWidth;
    int height = FrameHeight;
    
    char* yuv420sp = (char*)Buffer;
    int* rgb = new int[width * height]; //width and height of the image to be converted
    
    if (!Buffer) return;
    
    //Region
    FUpdateTextureRegion2D* region = new FUpdateTextureRegion2D(0, 0, 0, 0, FrameWidth, FrameHeight);
    
    //Debug image
    uint8* debugBuffer = GetDebugTexture(FrameWidth, FrameHeight);
    if (debugBuffer != NULL) {
        //Debug image
        UpdateTexture(WebcamTexture, 0, 1, region, (uint32)(4 * FrameWidth), 4, debugBuffer, true);
    }
    else {
        //YUV -> RGB converison
        
        int size = width*height;
        int offset = size;
        
        int u, v, y1, y2, y3, y4;
        
        for (int i = 0, k = 0; i < size; i += 2, k += 2) {
            y1 = yuv420sp[i] & 0xff;
            y2 = yuv420sp[i + 1] & 0xff;
            y3 = yuv420sp[width + i] & 0xff;
            y4 = yuv420sp[width + i + 1] & 0xff;
            
            u = yuv420sp[offset + k] & 0xff;
            v = yuv420sp[offset + k + 1] & 0xff;
            u = u - 128;
            v = v - 128;
            
            rgb[i] = YUVtoRGB(y1, u, v);
            rgb[i + 1] = YUVtoRGB(y2, u, v);
            rgb[width + i] = YUVtoRGB(y3, u, v);
            rgb[width + i + 1] = YUVtoRGB(y4, u, v);
            
            
            if (i != 0 && (i + 2) % width == 0)
                i += width;
        }
        
        UpdateTexture(WebcamTexture, 0, 1, region, (uint32)(4 * FrameWidth), 4, (uint8*)rgb, true);
    }
    
    newFrame = false; //Frame processed waiting for a new one
    gARTImage = (ARUint8*)Buffer; //for AR Toolkit
#endif
}

//<==============="【UPDATE TEXTURE APPLE】"==================>

void FARMarkerDevice::UpdateTextureApple(){
#if PLATFORM_MAC || PLATFORM_IOS
    ARUint8 *image;
    
    // update dynamic texture
    if ((image = arVideoGetImage()) != NULL)
    {
        gARTImage = image;    // Save the fetched image.
        
        const size_t SizeWebcamRGBX = arHandle->xsize * arHandle->ysize* sizeof(RGBQUAD);
        
        //Region
        FUpdateTextureRegion2D* region = new FUpdateTextureRegion2D(0, 0, 0, 0, xsize, ysize);
        
        //Debug image
        uint8* debugBuffer = GetDebugTexture(xsize, ysize);
        
        if (debugBuffer != NULL)
        {
            //Debug image
            UpdateTexture(WebcamTexture, 0, 1, region, (uint32)(4 * arHandle->xsize), 4, debugBuffer, true);
        }
        else
        {
            
            UpdateTexture(WebcamTexture, 0, 1, region, (uint32)(4 * xsize), 4, (uint8*)image, false);
        }
    }
#endif
}

//<==============="【UPDATE TEXTURE】"==================>

void FARMarkerDevice::UpdateTexture(UTexture2D *Texture, int32 MipIndex, uint32 NumRegions, FUpdateTextureRegion2D *Regions, uint32 SrcPitch, uint32 SrcBpp, uint8 *SrcData, bool bFreeData){
    if (SrcData == nullptr) return;
    if (Texture->Resource)
    {
        struct FUpdateTextureRegionsData
        {
            FTexture2DResource* Texture2DResource;
            int32 MipIndex;
            uint32 NumRegions;
            FUpdateTextureRegion2D* Regions;
            uint32 SrcPitch;
            uint32 SrcBpp;
            uint8* SrcData;
        };
        
        FUpdateTextureRegionsData* RegionData = new FUpdateTextureRegionsData;
        
        RegionData->Texture2DResource = (FTexture2DResource*)Texture->Resource;
        RegionData->MipIndex = MipIndex;
        RegionData->NumRegions = NumRegions;
        RegionData->Regions = Regions;
        RegionData->SrcPitch = SrcPitch;
        RegionData->SrcBpp = SrcBpp;
        RegionData->SrcData = SrcData;
        
        ENQUEUE_UNIQUE_RENDER_COMMAND_TWOPARAMETER(
        UpdateTextureRegionsData,
        FUpdateTextureRegionsData*, RegionData, RegionData,
        bool, bFreeData, bFreeData,
        {
        for (uint32 RegionIndex = 0; RegionIndex < RegionData->NumRegions; ++RegionIndex)
        {
         int32 CurrentFirstMip = RegionData->Texture2DResource->GetCurrentFirstMip();
            if (RegionData->MipIndex >= CurrentFirstMip)
            {
            RHIUpdateTexture2D(
            RegionData->Texture2DResource->GetTexture2DRHI(),
            RegionData->MipIndex - CurrentFirstMip,
            RegionData->Regions[RegionIndex],
            RegionData->SrcPitch,
            RegionData->SrcData
            + RegionData->Regions[RegionIndex].SrcY * RegionData->SrcPitch
            + RegionData->Regions[RegionIndex].SrcX * RegionData->SrcBpp
            );
            }
         }
            if (bFreeData)
            {
             FMemory::Free(RegionData->Regions);
             FMemory::Free(RegionData->SrcData);
            }
        delete RegionData;
                                                       
        processingTexture = false;
                                                       
        });
    }
}

//<==============="【INIT】"==================>

bool FARMarkerDevice::Init(bool showPIN, int devNum, EDeviceOrientation deviceOri, EArPatternDetectionMode detectionMode, int32 &WebcamResX, int32 &WebcamResY, bool &fr){
    
    fr = false;
    
    //-------------------------------------------------------------------------------------//
    // 【CIHAZ Rotasyon Kontrolu】
    //-------------------------------------------------------------------------------------//
    
    //=======>
    // 【ANDROID】
    //=======>
    
    FString AndroidOrientation;
    
    GConfig->GetString(TEXT("/Script/AndroidRuntimeSettings.AndroidRuntimeSettings"), TEXT("Orientation"), AndroidOrientation, GEngineIni);
    
    UE_LOG(LogTemp, Log, TEXT("Android Orientation: %s"), *AndroidOrientation);
    
    this->deviceOrientation = deviceOri;
    
#if PLATFORM_ANDROID
    AndroidThunkCpp_UnpackData();
#endif
    
#if PLATFORM_WINDOWS
    numWebcams = 0;
    
    IEnumMoniker *pEnum;
    
    HRESULT hr = EnumerateDevices(CLSID_VideoInputDeviceCategory, &pEnum);
    
    if (SUCCEEDED(hr))
    {
        DisplayDeviceInformation(pEnum);
        pEnum->Release();
    }
    
    UE_LOG(LogTemp, Log, TEXT("Number of webcams: %d"),numWebcams);
    
    if (devNum <= 0 && numWebcams > 0) devNum = 1;
    if (devNum > numWebcams) devNum = 1;
    if (numWebcams == 0) {
        UE_LOG(LogTemp, Error, TEXT("No webcam detected!"));
        return 0;
    }
#endif
    
    UE_LOG(LogTemp, Log, TEXT("Init Started"));
    
    ARParam cparam;
    AR_PIXEL_FORMAT pixFormat;
    
    //<====="Webcam texture herhangi bir nedenden dolayi oluşmuyorsa bunu geri gönderin."=====>//
    
    DummyTexture = UTexture2D::CreateTransient(640, 480);
    DummyTexture->SRGB = 1;
    DummyTexture->UpdateResource();
    
    xsize = WebcamResX = 640;
    ysize = WebcamResY = 480;
#if PLATFORM_MAC || PLATFORM_WINDOWS
     const char* pathData = TCHAR_TO_UTF8(*(DataPath + TEXT("Data/camera_para.dat")));
     FString MypathData(pathData);
     UE_LOG(ARMarker,Warning,TEXT("ARMarker Path Data: %s"), *MypathData);
#endif
    //<=============【KAMERAYI AYARLARI VE VİDEO DOKUSU OLUŞTURMA】============================>//
    
    //=======>
    // ANDROID
    //=======>
    
#if PLATFORM_ANDROID
    pixFormat = AR_PIXEL_FORMAT_NV21;
    WebcamTexture = UTexture2D::CreateTransient(FrameWidth,FrameHeight);
    WebcamTexture->SRGB = 1;
    WebcamTexture->UpdateResource();
    
    WebcamResX = FrameWidth;
    WebcamResY = FrameHeight;
    
    //Video texture size
    xsize = FrameWidth;
    ysize = FrameHeight;
    
    //Create webcam video texture
    WebcamRGBX = new RGBQUAD[xsize*ysize];
#endif
    
    //<==============="WINDOWS | iPHONE | OSX için ARVideo.lib Kullanımı"==================>

#if PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_IOS
    
    //<==============="WINDOWS Kamerayi Açmak İçin Parametre Ayarlari==================>
#if PLATFORM_WINDOWS

     stringstream config;
    
    config << "-devNum=" << devNum << "-flipV";
    
    if(showPIN) config << "-showDialog";
    
    if (arVideoOpen(config.str().c_str() < 0)) {
        arVideoClose();
        return 0;
    }
#endif
    
    //<==============="MAC Kamerayi Açmak İçin Parametre Ayarlari ==================>
    
#if PLATFORM_MAC
    
    std::stringstream config;
    //Kamera Cözünürlük Ayarı
    config << " -width=640 -height=480 ";
    if (showPIN) config << "-dialog";
    
    if (arVideoOpen(config.str().c_str()) < 0) return 0;
#endif
    
    //<==============="IPHONE Kamerayi Açmak İçin Parametre Ayarlari==================>
    
#if PLATFORM_IOS
    
    __block bool CameraAccessGranted = false;
    __block bool CameraAccessDenied = false;
    __block bool FirstRun = false;
    
    AVAuthorizationStatus authStatus = [AVCaptureDevice authorizationStatusForMediaType:AVMediaTypeVideo];
    if(authStatus == AVAuthorizationStatusAuthorized)
    {
        // do your logic
        CameraAccessGranted = true;
    }
    else if(authStatus == AVAuthorizationStatusDenied)
    {
        // denied
    }
    else if(authStatus == AVAuthorizationStatusRestricted)
    {
        // restricted, normally won't happen
    }
    else if(authStatus == AVAuthorizationStatusNotDetermined)
    {
        // not determined?!
        [AVCaptureDevice requestAccessForMediaType:AVMediaTypeVideo completionHandler:^(BOOL granted) {
            if (granted)
            {
                NSLog(@"Granted access to %@", AVMediaTypeVideo);
                CameraAccessGranted = true;
            }
            else
            {
                NSLog(@"Not granted access to %@", AVMediaTypeVideo);
                CameraAccessDenied = true;
            }
        }];
        while (!CameraAccessGranted && !CameraAccessDenied)
        {
            [[NSRunLoop currentRunLoop] runUntilDate:[NSDate dateWithTimeIntervalSinceNow : 0.1]];
        }
        if (!CameraAccessDenied)
        {
            FirstRun = true;
        }
    }
    else
    {
        // impossible, unknown authorization status
    }
    if (!CameraAccessGranted) return 0;
    if (FirstRun)
    {
        fr = true;
        return 0;
    }
    UE_LOG(LogTemp, Log, TEXT("Accessing iOS camera start"));
    //-------------------------------------------------------------------------------------//
    NSString *flipV;
    NSString *flipH;
    if(deviceOrientation == EDeviceOrientation::PORTRAIT)
    {
        flipV=[NSString stringWithFormat : @"-noflipv"];
        flipH=[NSString stringWithFormat : @"-nofliph"];
    }
    else
    {
        flipV=[NSString stringWithFormat : @"-flipv"];
        flipH=[NSString stringWithFormat : @"-fliph"];
    }
    
    NSString *vconf = [NSString stringWithFormat : @"%@ %@ %@ %@ %@", @"-format=BGRA", flipH, flipV, @"-preset=cif", @"-position=rear"];
    
    NSLog(@"%@",vconf);
    
    if (arVideoOpen(vconf.UTF8String))
    {
        UE_LOG(LogTemp, Error, TEXT("Error: Unable to open connection to camera.\n"));
        return 0;
    }
    UE_LOG(LogTemp, Log, TEXT("Camera ok.\n"));
#endif
    
    //-------------------------------------------------------------------------------------//
    // WINDOWS || OSX için kamera çözünürlüğü ve piksel formatı
    //-------------------------------------------------------------------------------------//
    
#if PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_IOS
    //Kamera çözünürlüğünü al
    if (arVideoGetSize(&xsize, &ysize) < 0) return 0;
    
    UE_LOG(LogTemp, Log, TEXT("Kamera çözünürlüğünü (%d,%d)\n"), xsize, ysize);
    
    //Kamera pikseli formatını al
    if ((pixFormat = arVideoGetPixelFormat()) < 0) return 0;
    
    UE_LOG(LogTemp, Log, TEXT("Kamera pixel format: %d"), (uint32)pixFormat);
#endif
    
    //IPHONE

#if PLATFORM_IOS
    arVideoSetParami(AR_VIDEO_PARAM_IOS_FOCUS, AR_VIDEO_IOS_FOCUS_0_3M);
    
    arParamClear(&cparam, xsize, ysize, AR_DIST_FUNCTION_VERSION_DEFAULT);
    
    //  UE_LOG(LogTemp, Log, TEXT("Iphone Camera Init");
#endif
           
    //-------------------------------------------------------------------------------------//
    // WEBCAM Video Dokusu Oluşturma
    //-------------------------------------------------------------------------------------//
           
    WebcamRGBX = new RGBQUAD[xsize*ysize];
    WebcamTexture = UTexture2D::CreateTransient(xsize, ysize);
    WebcamTexture->SRGB = 1;
    WebcamTexture->UpdateResource();
           
    WebcamResX = xsize;
    WebcamResY = ysize;
           
    UE_LOG(LogTemp, Log, TEXT("Webcam Video Texture Olusturuldu (%d,%d)\n"), xsize, ysize);
           
#endif
//SON
           
//
//Load camera param file
//
           
    UE_LOG(LogTemp, Log, TEXT("Param load start"));
           
//-------------------------------------------------------------------------------------//
//Kamerayi Ayarla Data Dosyasi İçin
//-------------------------------------------------------------------------------------//
           
    string path="";
    string SDataPath(TCHAR_TO_UTF8(*DataPath));
           
//IPHONE
        
#if PLATFORM_IOS
           if(iPhoneLaunched)
           {
               path=SDataPath+"data/camera_para.dat";
           }
           else
           {
               path=SDataPath+"Data/camera_para.dat";
           }
           
           UE_LOG(LogTemp, Log, TEXT("ios full path camera data: %s\n"), *FString(path.c_str()));
#endif
//-------------------------------------------------------------------------------------//
//
//WINDOWS, MAX OSX , ANDROID
//
//-------------------------------------------------------------------------------------//
#if PLATFORM_MAC || PLATFORM_WINDOWS || PLATFORM_ANDROID
           path = string(pathData);
#endif
           
//Load the camera data file
           
   if (arParamLoad(path.c_str(), 1, &cparam) < 0) {
       UE_LOG(LogTemp, Error, TEXT("Camera parameter load error !!\n"));
       return 0;
   }
           
   UE_LOG(LogTemp, Log, TEXT("Param load success"));
   UE_LOG(LogTemp, Log, TEXT("Camera init succesfull!"));
           
//
// Configure AR Toolkit
//
           
    arParamChangeSize(&cparam, xsize, ysize, &cparam);
           
    //Create param
    if((gCparamLT = arParamLTCreate(&cparam, AR_PARAM_LT_DEFAULT_OFFSET)) == NULL)
    {
        UE_LOG(LogTemp, Error, TEXT("Error: arParamLTCreate.\n"));
        return 0;
    }
           
    //Create AR handle
    if((arHandle = arCreateHandle(gCparamLT)) == NULL)
    {
        UE_LOG(LogTemp, Error, TEXT("Error: arCreateHandle.\n"));
        return 0;
    }
           
    //Set pixel format
    if(arSetPixelFormat(arHandle, pixFormat) < 0)
    {
        UE_LOG(LogTemp, Log, TEXT("Error: arSetPixelFormat.\n"));
        return 0;
    }
           
    //Create Patt handle
    if((arPattHandle = arPattCreateHandle()) == NULL)
    {
        UE_LOG(LogTemp, Error, TEXT("Error: arPattCreateHandle.\n"));
        return 0;
    }
           else
           {
               UE_LOG(LogTemp, Log, TEXT("AR Patt Handle created\n"));
           }
           //Create 3D handle
           if((ar3DHandle = ar3DCreateHandle(&cparam)) == NULL)
           {
               UE_LOG(LogTemp, Error, TEXT("Error:  ar3DCreateHandle.\n"));
               return 0;
           }
           else
           {
               UE_LOG(LogTemp, Log, TEXT("3D Handle created\n"));
           }
           
           switch(detectionMode)
           {
               case EArPatternDetectionMode::MATRIX_CODE_DETECTION:
               {
                   //Create MATRIX marker objects 0-63 (3x3 default size)
                   for (int i = 0; i < 64; i++) {
                       //Setup marker data in Markers array
                       FMarker* marker = new FMarker();
                       marker->name = FName(*FString::FromInt(i));
                       marker->position = FVector::ZeroVector;
                       marker->rotation = FRotator::ZeroRotator;
                       marker->cameraPosition = FVector::ZeroVector;
                       marker->cameraRotation = FRotator::ZeroRotator;
                       marker->visible = false;
                       marker->resetFilter = true;
                       marker->ftmi = 0;
                       marker->pageNo = 0;
                       marker->filterCutoffFrequency = 0;
                       marker->filterSampleRate = 0;
                       
                       //Filtering
                       ApplyFilter(marker);
                       MarkersMATRIX.Add(marker);
                   }
                   arSetPatternDetectionMode(arHandle, AR_MATRIX_CODE_DETECTION);
                   UE_LOG(LogTemp, Log, TEXT("Detection mode : MATRIX"));
                   break;
               }
               case EArPatternDetectionMode::TEMPLATE_MATCHING_COLOR:
               {
                   arSetPatternDetectionMode(arHandle, AR_TEMPLATE_MATCHING_COLOR);
                   UE_LOG(LogTemp, Log, TEXT("Detection mode : TEMPLATE"));
                   break;
               }
           }
           
           this->patternDetectionMode = detectionMode;
           
           UE_LOG(LogTemp, Log, TEXT("ARToolkit init succesfull!"));
           
           //////////////////////////////////////////////////////////////////////////////////////
           // NFT init.
           //////////////////////////////////////////////////////////////////////////////////////
           
           UE_LOG(LogTemp, Log, TEXT("NFT init started..."));
           
           // KPM init.
           kpmHandle = kpmCreateHandle(gCparamLT, pixFormat);
           if (!kpmHandle)
           {
               UE_LOG(LogTemp, Error, TEXT("Error: kpmCreateHandle.\n"));
               return 0;
           }
           if((ar2Handle = ar2CreateHandle(gCparamLT, pixFormat, AR2_TRACKING_DEFAULT_THREAD_NUM)) == NULL)
           {
               UE_LOG(LogTemp, Error, TEXT("Error: ar2CreateHandle.\n"));
               kpmDeleteHandle(&kpmHandle);
               return 0;
           }
           if(threadGetCPU() <= 1)
           {
               UE_LOG(LogTemp, Log, TEXT("Using NFT tracking settings for a single CPU.\n"));
               ar2SetTrackingThresh(ar2Handle, 5.0);
               ar2SetSimThresh(ar2Handle, 0.50);
               ar2SetSearchFeatureNum(ar2Handle, 16);
               ar2SetSearchSize(ar2Handle, 6);
               ar2SetTemplateSize1(ar2Handle, 6);
               ar2SetTemplateSize2(ar2Handle, 6);
           }
           else
           {
               UE_LOG(LogTemp, Log, TEXT("Using NFT tracking settings for more than one CPU.\n"));
               ar2SetTrackingThresh(ar2Handle, 5.0);
               ar2SetSimThresh(ar2Handle, 0.50);
               ar2SetSearchFeatureNum(ar2Handle, 16);
               ar2SetSearchSize(ar2Handle, 12);
               ar2SetTemplateSize1(ar2Handle, 6);
               ar2SetTemplateSize2(ar2Handle, 6);
           }
           //Multi NFT
           nftMultiMode = true;
           kpmRequired = true;
           kpmBusy = false;
           
           UE_LOG(LogTemp, Log, TEXT("NFT init successfull..."));
           
           //////////////////////////////////////////////////////////////////////////////////////
           // END of NFT init.
           //////////////////////////////////////////////////////////////////////////////////////
           
           //
           // Start video capture
           //
           
           //Android
#if PLATFORM_ANDROID
           //AndroidThunkCpp_Vibrate(1000);
           AndroidThunkCpp_CamStart();
#endif
           
           //Windows , MAX OSX, iPhone
#if PLATFORM_MAC || PLATFORM_WINDOWS
           if (arVideoCapStart() != 0) {
               UE_LOG(LogTemp, Error, TEXT("Error: video capture start error !!\n"));
               arVideoClose();
               return 0;
           }
#endif
           
           //Get threshold
           arGetLabelingThresh(arHandle, &threshold);
           
           initiated = 1;
           
           UE_LOG(LogTemp, Log, TEXT("Init complete !!\n"));
           
           this->mWebcamResX = WebcamResX;
           this->mWebcamResY = WebcamResY;
           
           return 1; //Succesfull init
}
           
//<==============="【SET THRESHOL MODE】"==================>
           
void FARMarkerDevice::SetThresholdMode(EArLabelingThresholdMode mode)
{
    switch (mode) {
        case EArLabelingThresholdMode::MANUAL: arSetLabelingThreshMode(arHandle, AR_LABELING_THRESH_MODE_MANUAL); break;
        case EArLabelingThresholdMode::AUTO_MEDIAN: arSetLabelingThreshMode(arHandle, AR_LABELING_THRESH_MODE_AUTO_MEDIAN); break;
        case EArLabelingThresholdMode::AUTO_OTSU: arSetLabelingThreshMode(arHandle, AR_LABELING_THRESH_MODE_AUTO_OTSU); break;
        case EArLabelingThresholdMode::AUTO_ADAPTIVE: arSetLabelingThreshMode(arHandle, AR_LABELING_THRESH_MODE_AUTO_ADAPTIVE); break;
        case EArLabelingThresholdMode::AUTO_BRACKETING: {
            arSetDebugMode(arHandle, AR_DEBUG_DISABLE);
            arSetLabelingThreshMode(arHandle, AR_LABELING_THRESH_MODE_AUTO_BRACKETING);
                       if (this->debugMode == 1) {
                           arSetDebugMode(arHandle, AR_DEBUG_ENABLE);
                       }
                       break;
        }
        default: arSetLabelingThreshMode(arHandle, AR_LABELING_THRESH_MODE_DEFAULT); break;
    };
}
           
//<==============="【SET THRES HOLD】"==================>
           
void FARMarkerDevice::SetThreshold(int thresholdValue)
    {
        arSetLabelingThreshMode(arHandle, AR_LABELING_THRESH_MODE_MANUAL);
        arSetLabelingThresh(arHandle, thresholdValue);
        threshold = thresholdValue;
        
    }
           
//<==============="【GET THRES HOLD】"==================>
           
int FARMarkerDevice::GetThreshold(){
    arGetLabelingThresh(arHandle, &threshold);
    return threshold;
}
           
//<================="【SET DEBUG MODE】"====================>
           
void FARMarkerDevice::SetDebugMode(bool mode){
    //Set debug mode
    if (mode == true)
    {
        arSetDebugMode(arHandle, AR_DEBUG_ENABLE);
        this->debugMode = 1;
    }
    else
    {
        arSetDebugMode(arHandle, AR_DEBUG_DISABLE);
        this->debugMode = 0;
    }
}
           
//<==============="【CLEANUP】"==================>
           
void FARMarkerDevice::Cleanup(void)
{
    if(initiated == false) return;
               
#if PLATFORM_ANDROID
    AndroidThunkCpp_CamStop();
#endif
    
    arPattDetach(arHandle);
    arPattDeleteHandle(arPattHandle);
    ar3DDeleteHandle(&ar3DHandle);
    arDeleteHandle(arHandle);
    
    // NFT cleanup.
    UnloadNFTData();
    ar2DeleteHandle(&ar2Handle);
    kpmDeleteHandle(&kpmHandle);
    arParamLTFree(&gCparamLT);
    
    
    //Reset marker arrays
    Markers.Reset();
    MarkersNFT.Reset();
    MarkersMATRIX.Reset();
    
    
#if PLATFORM_MAC || PLATFORM_IOS || PLATFORM_WINDOWS
    arVideoCapStop();
    arVideoClose();
    
    UE_LOG(ARMarker, Warning, TEXT("Video Stop !!\n"));
#endif
    
    if (WebcamRGBX)
    {
        delete[] WebcamRGBX;
        WebcamRGBX = NULL;
    }
    
    initiated = 0;
    
    UE_LOG(LogTemp, Log, TEXT("Cleanup ready !!\n"));
}
           
//<==============="【LOAD MARKERS】"==================>
           
bool FARMarkerDevice::LoadMarkers(TArray<FString> markerNames){
    if (!initiated) return false;
    for (int32 i = 0; i != markerNames.Num(); ++i)
    {
        FString markerName = markerNames[i];
        string path = "";
        string SDataPath(TCHAR_TO_UTF8(*DataPath));
        string SMarkerName(TCHAR_TO_UTF8(*markerName.ToLower()));
        
#if PLATFORM_IOS
        //Load Marker
        if (iPhoneLaunched){
            path=SDataPath+"data/patt."+SMarkerName;
        }
        else
        {
            path=SDataPath+"Data/patt."+SMarkerName;
        }
#endif
                   
#if PLATFORM_WINDOWS || PLATFORM_MAC || PLATFORM_ANDROID
        path = TCHAR_TO_UTF8(*(DataPath + "Data/patt." + markerName));
#endif
        
        UE_LOG(LogTemp, Log, TEXT("Reading %s"), *FString(UTF8_TO_TCHAR(path.c_str())));
        
        if ((arPattLoad(arPattHandle,path.c_str())) < 0) {
            
            UE_LOG(LogTemp, Error, TEXT("Error %s  pattern load error !!\n"), *markerName);
            
            if (GEngine){
                GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("Unable to load marker: %s"), *markerName));
            }
            return 0;
        }
        else{
            arPattAttach(arHandle, arPattHandle);
        }
        
        //Setup marker data in Markers array
        FMarker* marker = new FMarker();
        
        marker->name = FName(*markerName);
        marker->position = FVector::ZeroVector;
        marker->rotation = FRotator::ZeroRotator;
        marker->cameraPosition = FVector::ZeroVector;
        marker->cameraRotation = FRotator::ZeroRotator;
        marker->visible = false;
        marker->resetFilter = true;
        marker->ftmi = 0;
        marker->pageNo = 0;
        marker->filterCutoffFrequency = 0;
        marker->filterSampleRate = 0;
                   
        //Filtering
        ApplyFilter(marker);
                   
        Markers.Add(marker);
                   
        UE_LOG(LogTemp, Log, TEXT("%s loaded\n"), *markerName);
        
        }
        return 1;
}
           
//<==============="【LOAD MARKERS NFT】"==================>
           
bool FARMarkerDevice::LoadMarkersNFT(TArray<FString> markerNames){
    
    if (!initiated) return false;
    KpmRefDataSet* refDataSet;
               
    // If data was already loaded, stop KPM tracking thread and unload previously loaded data.
    
    if (threadHandle) {
        
        UE_LOG(LogTemp, Log, TEXT("Reloading NFT data.\n"));
        
        UnloadNFTData();
        
    }
    
    refDataSet = NULL;
               
    for (int32 i = 0; i != markerNames.Num(); i++){
        
        FString markerName = markerNames[i];
        
        UE_LOG(LogTemp, Log, TEXT("Loading NFT data: %s.\n"), *markerName);
        
        // Load KPM data.
        
        KpmRefDataSet  *refDataSet2;
        
        //Dataset Path
        
        string path = "";
        string SDataPath(TCHAR_TO_UTF8(*DataPath));
        string SMarkerName(TCHAR_TO_UTF8(*markerName.ToLower()));
                   
#if PLATFORM_IOS
        //Load Marker
        
        if (iPhoneLaunched)
        {
            path=SDataPath+"datanft/"+SMarkerName;
        }
        else
        {
            path=SDataPath+"DataNFT/"+SMarkerName;
        }
#endif
                   
#if PLATFORM_WINDOWS || PLATFORM_MAC || PLATFORM_ANDROID
        path = TCHAR_TO_UTF8(*(DataPath + "DataNFT/" + markerName));
#endif
        //Setup NFT marker in MarkersNFT array
        FMarker* markerNFT = new FMarker();
        
        markerNFT->name = FName(*markerName);
        markerNFT->position = FVector::ZeroVector;
        markerNFT->rotation = FRotator::ZeroRotator;
        markerNFT->cameraPosition = FVector::ZeroVector;
        markerNFT->cameraRotation = FRotator::ZeroRotator;
        markerNFT->visible = false;
        markerNFT->resetFilter = true;
        markerNFT->ftmi = 0;
        markerNFT->pageNo = 0;
        markerNFT->filterCutoffFrequency = 0;
        markerNFT->filterSampleRate = 0;
        
        UE_LOG(LogTemp, Log, TEXT("Reading %s.fset3\n"), *FString(UTF8_TO_TCHAR(path.c_str())));
        
        if (kpmLoadRefDataSet(path.c_str(), "fset3", &refDataSet2) < 0)
        {
            UE_LOG(LogTemp, Error, TEXT("Error reading KPM data from %s.fset3\n"), *FString(UTF8_TO_TCHAR(path.c_str())));
            
            markerNFT->pageNo = -1;
            return 0;
        }
        
        markerNFT->pageNo = surfaceSetCount;
        
        UE_LOG(LogTemp, Log, TEXT("Assigned page no. %d.\n"), surfaceSetCount);
        
        if (kpmChangePageNoOfRefDataSet(refDataSet2, KpmChangePageNoAllPages, surfaceSetCount) < 0)
        {
            
            UE_LOG(LogTemp, Error, TEXT("Error: kpmChangePageNoOfRefDataSet\n"));
            
            return 0;
        }
        if (kpmMergeRefDataSet(&refDataSet, &refDataSet2) < 0) {
            
            UE_LOG(LogTemp, Error, TEXT("Error: kpmMergeRefDataSet\n"));
            
            return 0;
        }
        
        UE_LOG(LogTemp, Log, TEXT("Done\n"));
        
        // Load AR2 data.
        
        UE_LOG(LogTemp, Log, TEXT("Reading %s.fset\n"), *FString(UTF8_TO_TCHAR(path.c_str())));
                   
        
        if ((surfaceSet[surfaceSetCount] = ar2ReadSurfaceSet(path.c_str(), "fset", NULL)) == NULL)
            
        {
            UE_LOG(LogTemp, Error, TEXT("Error reading data from %s.fset\n"), *FString(UTF8_TO_TCHAR(path.c_str())));
            
        }
        
        UE_LOG(LogTemp, Log, TEXT("Done\n"));
                   
        //Validity default
        markerNFT->valid = false;
        markerNFT->validPrev = false;
                   
        //Filtering
        ApplyFilter(markerNFT);
        
        //Add to markersNFT array
        MarkersNFT.Add(markerNFT);
        
        UE_LOG(LogTemp, Log, TEXT("NTF Marker Yükleme %s Tamamlandı.\n"), *markerName);
                   
        surfaceSetCount++;
        if (surfaceSetCount == PAGES_MAX) break;
        }
    
    if (kpmSetRefDataSet(kpmHandle, refDataSet) < 0)
    {
        UE_LOG(LogTemp, Error, TEXT("Error: kpmSetRefDataSet\n"));
        
        return 0;
    }
    
    kpmDeleteRefDataSet(&refDataSet);
               
    // Start the KPM tracking thread.
    threadHandle = trackingInitInit(kpmHandle);
    if (!threadHandle)
    {
        return 0;
    }
    else
    {
        UE_LOG(LogTemp, Log, TEXT("KPM Thread handle initialised\n"));
    }
    return 1;
}
           
//<==============="【DETECT MARKERS MATRIX】"==================>
           
void FARMarkerDevice::DetectMarkersMATRIX(){
    if (gARTImage == NULL) return;
    
    //Return if camera image buffer is NULL
    //////////////////////////////////////////////////////////////////////
    //
    // Performance optimisation (don't check markers in every frame)
    //
    /////////////////////////////////////////////////////////////////////
    
    static double ms_prev;
    double ms;
    double s_elapsed;
               
    // Find out how long since mainLoop() last ran.
    ms = FPlatformTime::Seconds();
    s_elapsed = ms - ms_prev;
               
#if PLATFORM_WINDOWS || PLATFORM_MAC
    if (s_elapsed < 0.01f) return; // Don't update more often than 100 Hz.
#endif
               
#if PLATFORM_ANDROID || PLATFORM_IOS
    if (s_elapsed < 0.033f) return; // Don't update more often than 30Hz
#endif
    ms_prev = ms;
               
    TArray<int> visibleMarkers;
               
    /* detect the markers in the video frame */
    if (arDetectMarker(arHandle, gARTImage) < 0) {
        
        UE_LOG(LogTemp, Error, TEXT("Error: marker detection error !!\n"));
        
        return;
    }
    
    int markerNum = arGetMarkerNum(arHandle);
    
    /* check for object visibility */
    
    ARMarkerInfo   *markerInfo;
    markerInfo = arGetMarker(arHandle);
               
    for (int j = 0; j < markerNum; j++) {
                   
    //Matrix markers only
        
        if (markerInfo[j].idMatrix != -1) {
            //UE_LOG(ARToolkit, Log, TEXT("Marker idMatrix : %d"), markerInfo[j].idMatrix);
                       
            ARdouble quaternions[4];
            ARdouble positions[3];
            ARdouble patt_trans_inv[3][4];
            ARdouble patt_trans[3][4];
            ARdouble err;
            
            if (MarkersMATRIX[markerInfo[j].idMatrix]->visible == false) {
                //NEW
                MarkersMATRIX[markerInfo[j].idMatrix]->resetFilter = true;
                err = arGetTransMatSquare(ar3DHandle, &(markerInfo[j]), patt_width, patt_trans);
                
                //UE_LOG(ARToolkit, Log, TEXT("New tracking %d ERR: %f"), markerInfo[j].idMatrix, (float)err);
                
            }
            else
            {
                //CONTINUE FROM LAST FRAME
                memcpy(patt_trans, MarkersMATRIX[markerInfo[j].idMatrix]->matrix, sizeof(ARdouble) * 3 * 4);
                err = arGetTransMatSquareCont(ar3DHandle, &(markerInfo[j]), patt_trans, patt_width, patt_trans);
                
                //UE_LOG(ARToolkit, Log, TEXT("Continue tracking %d ERR: %f"), markerInfo[j].idMatrix, (float)err);

            }
            
            memcpy(MarkersMATRIX[markerInfo[j].idMatrix]->matrix, patt_trans, sizeof(ARdouble) * 3 * 4);
        
            // Filter the pose estimate.
            
           if (MarkersMATRIX[markerInfo[j].idMatrix]->ftmi && filterEnabled)
           {
               bool reset = false;
               
               if (MarkersMATRIX[markerInfo[j].idMatrix]->resetFilter)
               {
                   reset = true;
                   MarkersMATRIX[markerInfo[j].idMatrix]->resetFilter = false;
               }
               
               if (arFilterTransMat(MarkersMATRIX[markerInfo[j].idMatrix]->ftmi, MarkersMATRIX[markerInfo[j].idMatrix]->matrix, reset) < 0)
               {
                               
                   
               }
           }
            
            memcpy(patt_trans, MarkersMATRIX[markerInfo[j].idMatrix]->matrix, sizeof(ARdouble) * 3 * 4);
            
            arUtilMatInv(patt_trans, patt_trans_inv);
            arUtilMat2QuatPos(patt_trans_inv, quaternions, positions);
                       
            MarkersMATRIX[markerInfo[j].idMatrix]->position = FVector(patt_trans[0][3], patt_trans[1][3], -patt_trans[2][3]);
            MarkersMATRIX[markerInfo[j].idMatrix]->rotation = FQuat(quaternions[0], quaternions[1], quaternions[2], quaternions[3]).Rotator();
                       
            //Camera position, rotation
            MarkersMATRIX[markerInfo[j].idMatrix]->cameraPosition = FVector(-positions[0], positions[1], positions[2]);
            arUtilMat2QuatPos(patt_trans, quaternions, positions);
            MarkersMATRIX[markerInfo[j].idMatrix]->cameraRotation = FQuat(quaternions[0], quaternions[1], quaternions[2], quaternions[3]).Rotator();
                       
            visibleMarkers.Add(markerInfo[j].idMatrix);
        }
    }
    
    //Reset marker visibility
    for (int c = 0; c < MarkersMATRIX.Num(); c++)
    {
        MarkersMATRIX[c]->visible = false;
    }
    if (visibleMarkers.Num() > 0) {
        //Mark tracked markers
        for (int z = 0; z < visibleMarkers.Num(); z++)
        {
            MarkersMATRIX[visibleMarkers[z]]->visible = true;
            
        }
    }
}

//<==============="【DETECT MARKERS】"==================>
           
void FARMarkerDevice::DetectMarkers(){
    
    if (gARTImage == NULL) return; //Return if camera image buffer is NULL
    
    //////////////////////////////////////////////////////////////////////
    //
    // Performance optimisation (don't check markers in every frame)
    //
    /////////////////////////////////////////////////////////////////////
               
    static double ms_prev;
    double ms;
    double s_elapsed;
               
    // Find out how long since mainLoop() last ran.
    ms = FPlatformTime::Seconds();
    s_elapsed = ms - ms_prev;
               
#if PLATFORM_WINDOWS || PLATFORM_MAC
    if (s_elapsed < 0.01f) return; // Don't update more often than 100 Hz.
#endif
               
#if PLATFORM_ANDROID || PLATFORM_IOS
    if (s_elapsed < 0.033f) return; // Don't update more often than 30Hz
#endif
    
    ms_prev = ms;
    
    /////////////////////////////////////////////////////////////////////
    
    TArray<int> visibleMarkers;
    
    /* detect the markers in the video frame */
    
    if (arDetectMarker(arHandle, gARTImage) < 0)
    {
        
        UE_LOG(LogTemp, Error, TEXT("Error: marker detection error !!\n"));
        
        return;
    }
    
    int markerNum = arGetMarkerNum(arHandle);
               
    /* check for object visibility */
    ARMarkerInfo   *markerInfo;
    markerInfo = arGetMarker(arHandle);
    
    for (int j = 0; j < markerNum; j++)
    {
        if (markerInfo[j].cf>0.7 && markerInfo[j].id != -1)
        {
                       
        ARdouble quaternions[4];
        ARdouble positions[3];
        ARdouble patt_trans_inv[3][4];
        ARdouble patt_trans[3][4];
        ARdouble err;
            
            //UE_LOG(ARToolkit, Log, TEXT("Last id: %d"), markerInfo[j].id);
            
            if (markerInfo[j].id > Markers.Num()) return;
            if (Markers[markerInfo[j].id]->visible == false)
            {
                //NEW
                Markers[markerInfo[j].id]->resetFilter = true;
                err = arGetTransMatSquare(ar3DHandle, &(markerInfo[j]), patt_width, patt_trans);
                //UE_LOG(ARToolkit, Log, TEXT("New tracking %d ERR: %f"), markerInfo[j].id, (float)err);
            }
            else
            {
                //CONTINUE FROM LAST FRAME
                memcpy(patt_trans, Markers[markerInfo[j].id]->matrix, sizeof(ARdouble) * 3 * 4);
                err = arGetTransMatSquareCont(ar3DHandle, &(markerInfo[j]), patt_trans, patt_width, patt_trans);
                           
                //UE_LOG(ARToolkit, Log, TEXT("Continue tracking %d ERR: %f"), markerInfo[j].id, (float)err);
    
            }
            memcpy(Markers[markerInfo[j].id]->matrix,patt_trans, sizeof(ARdouble) * 3 * 4);
            
            // Filter the pose estimate.
            if (Markers[markerInfo[j].id]->ftmi && filterEnabled)
            {
                bool reset = false;
                if (Markers[markerInfo[j].id]->resetFilter)
                {
                    reset = true;
                    Markers[markerInfo[j].id]->resetFilter = false;
                }
                if (arFilterTransMat(Markers[markerInfo[j].id]->ftmi, Markers[markerInfo[j].id]->matrix, reset) < 0)
                {
                    
                }
            }
            memcpy(patt_trans, Markers[markerInfo[j].id]->matrix, sizeof(ARdouble) * 3 * 4);
            
            arUtilMatInv(patt_trans, patt_trans_inv);
            arUtilMat2QuatPos(patt_trans_inv, quaternions, positions);
            
            Markers[markerInfo[j].id]->position = FVector(patt_trans[0][3], patt_trans[1][3], -patt_trans[2][3]);
            Markers[markerInfo[j].id]->rotation = FQuat(quaternions[0], quaternions[1], quaternions[2], quaternions[3]).Rotator();
                       
            //Camera position, rotation
            Markers[markerInfo[j].id]->cameraPosition = FVector(-positions[0], positions[1], positions[2]);
            arUtilMat2QuatPos(patt_trans, quaternions, positions);
            Markers[markerInfo[j].id]->cameraRotation = FQuat(quaternions[0], quaternions[1], quaternions[2], quaternions[3]).Rotator();
            
            visibleMarkers.Add(markerInfo[j].id);
            
        }
    }
    //Reset marker visibility
    for (int c = 0; c < Markers.Num(); c++)
    {
        Markers[c]->visible = false;
    }
    if (visibleMarkers.Num() > 0)
    {
        //Mark tracked markers
        for (int z = 0; z < visibleMarkers.Num(); z++)
        {
            Markers[visibleMarkers[z]]->visible = true;
        }
    }
}

//<==============="【GET MARKER】"==================>

FMarker* FARMarkerDevice::GetMarker(uint8 markerId)
    {
    if(Markers.Num()>markerId)
    {
        return Markers[markerId];
    }
    else
    {
        FMarker* marker = new FMarker();
        marker->name = "";
        marker->position = FVector::ZeroVector;
        marker->rotation = FRotator::ZeroRotator;
        marker->cameraPosition = FVector::ZeroVector;
        marker->cameraRotation = FRotator::ZeroRotator;
        marker->visible = false;
        
        return marker;
    }
}
           
//<==============="【GET MARKER MATRIX】"==================>
           
FMarker* FARMarkerDevice::GetMarkerMATRIX(uint8 markerId)
    {
        if (MarkersMATRIX.Num() > markerId)
        {
            return MarkersMATRIX[markerId];
        }
        else
        {
            FMarker* marker = new FMarker();
            
            marker->name = "";
            marker->position = FVector::ZeroVector;
            marker->rotation = FRotator::ZeroRotator;
            marker->cameraPosition = FVector::ZeroVector;
            marker->cameraRotation = FRotator::ZeroRotator;
            marker->visible = false;
            
            return marker;
        }
    }
           
//<==============="【GET MARKER NFT】"==================>

FMarker* FARMarkerDevice::GetMarkerNFT(uint8 markerId)
{
    if (MarkersNFT.Num() > markerId) {
        return MarkersNFT[markerId];
        UE_LOG(LogTemp, Log, TEXT("Get Marker NFT"));
    }
    else{
        FMarker* marker = new FMarker();
        
        marker->name = "";
        marker->position = FVector::ZeroVector;
        marker->rotation = FRotator::ZeroRotator;
        marker->cameraPosition = FVector::ZeroVector;
        marker->cameraRotation = FRotator::ZeroRotator;
        marker->visible = false;
        
        return marker;
    }
}


//<==============="【GET RELATIVE TRANSFORMATION】"==================>

bool FARMarkerDevice::GetRelativeTransformation(uint8 MarkerID1, uint8 MarkerID2, FVector &RelativePosition, FRotator &RelativeRotation)
    {
    RelativePosition = FVector::ZeroVector;
    RelativeRotation = FRotator::ZeroRotator;
    
    FMarker* Marker1 = GetMarker(MarkerID1);
    FMarker* Marker2 = GetMarker(MarkerID2);
    
    if (Marker1->visible == true && Marker2->visible == true)
    {
        ARdouble wmat1[3][4], wmat2[3][4];
        ARdouble quaternions[4];
        ARdouble positions[3];
        
        arUtilMatInv(Marker1->matrix, wmat1);
        arUtilMatMul(wmat1, Marker2->matrix, wmat2);
        
        arUtilMat2QuatPos(wmat2, quaternions, positions);
        
        RelativePosition = FVector(positions[0], -positions[1], -positions[2]);
        RelativeRotation = FQuat(quaternions[0], quaternions[1], quaternions[2], quaternions[3]).Rotator();
        
        return 1;
    }
    else{
        return 0;
    }
}
           
//<==============="【GET RELATIVE TRANSFORMATION NFT】"==================>
           
bool FARMarkerDevice::GetRelativeTransformationNFT(uint8 MarkerID1, uint8 MarkerIdNFT, FVector &RelativePosition, FRotator &RelativeRotation)
    {
        
    RelativePosition = FVector::ZeroVector;
    RelativeRotation = FRotator::ZeroRotator;
    
    FMarker* Marker1 = GetMarker(MarkerID1);
    FMarker* Marker2 = GetMarkerNFT(MarkerIdNFT);
    
    if (Marker1->visible == true && Marker2->visible == true)
    {
        ARdouble wmat1[3][4], wmat2[3][4];
        ARdouble quaternions[4];
        ARdouble positions[3];
        
        arUtilMatInv(Marker1->matrix, wmat1);
        arUtilMatMul(wmat1, Marker2->matrix, wmat2);
        
        
        arUtilMat2QuatPos(wmat2, quaternions, positions);
        
        RelativePosition = FVector(positions[0], -positions[1], -positions[2]);
        RelativeRotation = FQuat(quaternions[0], quaternions[1], quaternions[2], quaternions[3]).Rotator();
        
        return 1;
        
    }
    else
    {
        return 0;
    }
}

//<==============="【UNLOAD NFT DATA】"==================>
           
int FARMarkerDevice::UnloadNFTData(void){
    int i, j;
    
    if(threadHandle){
        UE_LOG(LogTemp, Log, TEXT("Stopping NFT2 tracking thread.\n"));
        trackingInitQuit(&threadHandle);
    }
    
    j = 0;
    
    for(i = 0; i < surfaceSetCount; i++){
        if(j == 0) UE_LOG(LogTemp, Log, TEXT("Unloading NFT tracking surfaces.\n"));
        ar2FreeSurfaceSet(&surfaceSet[i]);
        j++;
    }
    
    if(j > 0) UE_LOG(LogTemp, Log, TEXT("Unloaded %d NFT tracking surfaces.\n"), j);
    surfaceSetCount = 0;
    
    return 0;
}

//<==============="【DETECT MARKERS NFT】"==================>

void FARMarkerDevice::DetectMarkersNFT(void)
    {
        if (gARTImage == NULL) return;
        
        //////////////////////////////////////////////////////////////////////
        //
        // Performance optimisation (don't check markers in every frame),
        //
        /////////////////////////////////////////////////////////////////////
        
        static double ms_prev_nft;
        double ms_nft;
        double s_elapsed_nft;
        
        // Find out how long since mainLoop() last ran.
        ms_nft = FPlatformTime::Seconds();
        s_elapsed_nft = ms_nft - ms_prev_nft;
               
#if PLATFORM_WINDOWS || PLATFORM_MAC
        if (s_elapsed_nft < 0.01f) return; // Don't update more often than 100 Hz.
#endif
               
#if PLATFORM_ANDROID || PLATFORM_IOS
        if (s_elapsed_nft < 0.033f) return; // Don't update more often than 30Hz
#endif
               
        ms_prev_nft = ms_nft;
               
        ////////////////////////////////////////////////////////////////////
        // NFT results.
        // Process video frame.
        // Run marker detection on frame
               
        if (threadHandle) {
        
        // Do KPM tracking.
        float err;
        static float trackingTrans[3][4];
                   
        if (kpmRequired)
        {
        if (!kpmBusy)
        {
        trackingInitStart(threadHandle, gARTImage);
        kpmBusy = true;
        }
        else
        {
            int ret;
            int pageNo;
            ret = trackingInitGetResult(threadHandle, trackingTrans, &pageNo);
            if (ret != 0)
            {
                kpmBusy = false;
                if (ret == 1)
                {
                    if (pageNo >= 0 && pageNo < PAGES_MAX)
                    {
                        if (surfaceSet[pageNo]->contNum < 1)
                        {
                            UE_LOG(LogTemp,Log,TEXT("Detected page %d.\n"), pageNo);
                            
                            //NEW
                            MarkersNFT[pageNo]->resetFilter = true;
                            
                            ar2SetInitTrans(surfaceSet[pageNo], trackingTrans); // Sets surfaceSet[page]->contNum = 1.
                        }
                    }
                    else
                    {
                        
                        UE_LOG(LogTemp, Log, TEXT("ARController::update(): Detected bad page %d"), pageNo);
            
                    }
                    
                }
                else
                {
                    //UE_LOG(ARToolkit, Log, TEXT("No page detected."));
                }
            }
        }
        }
            // Do AR2 tracking and update NFT markers.
            int page = 0;
            int pagesTracked = 0;
            bool success = true;
                   
            for (int i = 0; i < MarkersNFT.Num(); i++)
            {
                ARdouble patt_trans[3][4];
                ARdouble patt_trans_inv[3][4];
                ARdouble quaternions[4];
                ARdouble positions[3];
                
                if (surfaceSet[page]->contNum > 0)
                {
                    if (ar2Tracking(ar2Handle, surfaceSet[page], gARTImage, trackingTrans, &err) < 0)
                    {
                        UE_LOG(LogTemp, Log, TEXT("Tracking lost on page %d."), page);
                        
                        MarkersNFT[i]->visible = false;
                        
                    }
                    else
                    {
                        //UE_LOG(ARToolkit, Log, TEXT("Tracked page %d (pos = {% 4f, % 4f, % 4f}).\n"), page, trackingTrans[0][3], trackingTrans[1][3], trackingTrans[2][3]);
                        for (int j = 0; j < 3; j++) for (int k = 0; k < 4; k++) MarkersNFT[i]->matrix[j][k] = trackingTrans[j][k];
                               
                        MarkersNFT[i]->visible = true;
                               
                        // Filter the pose estimate.
                        if (MarkersNFT[i]->ftmi && filterEnabled)
                        {
                            bool reset = false;
                            if (MarkersNFT[i]->resetFilter)
                            {
                                reset = true;
                                MarkersNFT[i]->resetFilter = false;
                            }
                            if (arFilterTransMat(MarkersNFT[i]->ftmi, MarkersNFT[i]->matrix, reset) < 0)
                            {
                                       //Do nothing for now....
                            }
                        }
                        memcpy(patt_trans, MarkersNFT[i]->matrix, sizeof(ARdouble) * 3 * 4);
                               
                        arUtilMatInv(patt_trans, patt_trans_inv);
                        arUtilMat2QuatPos(patt_trans_inv, quaternions, positions);
                               
                        MarkersNFT[i]->position = FVector(patt_trans[0][3], patt_trans[1][3], -patt_trans[2][3]);
                        MarkersNFT[i]->rotation = FQuat(quaternions[0], quaternions[1], quaternions[2], quaternions[3]).Rotator();
                               
                        //Camera position, rotation
                        MarkersNFT[i]->cameraPosition = FVector(-positions[0], positions[1], positions[2]);
                        arUtilMat2QuatPos(patt_trans, quaternions, positions);
                        MarkersNFT[i]->cameraRotation = FQuat(quaternions[0], quaternions[1], quaternions[2], quaternions[3]).Rotator();
                               
                        pagesTracked++;
                        
                    }
                }
                page++;
            }
            kpmRequired = (pagesTracked < (nftMultiMode ? page : 1));
        } // threadHandle
    }

//<==============="【CONVERT TO IOS PATH】"==================>

#if PLATFORM_IOS
FString FARMarkerDevice::ConvertToIOSPath(const FString& Filename, bool bForWrite)
    {
        FString Result = Filename;
        Result.ReplaceInline(TEXT("../"), TEXT(""));
        Result.ReplaceInline(TEXT(".."), TEXT(""));
        Result.ReplaceInline(FPlatformProcess::BaseDir(), TEXT(""));
        
        if(bForWrite)
        {
            static FString WritePathBase = FString([NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0]) + TEXT("/");
            
            return WritePathBase + Result;
        }
        else
        {
            // if filehostip exists in the command line, cook on the fly read path should be used
            FString Value;
            // Cache this value as the command line doesn't change...
            
            static bool bHasHostIP = FParse::Value(FCommandLine::Get(), TEXT("filehostip"), Value) || FParse::Value(FCommandLine::Get(), TEXT("streaminghostip"), Value);
            
            static bool bIsIterative = FParse::Value(FCommandLine::Get(), TEXT("iterative"), Value);
            
            if (bHasHostIP)
                   {
                       static FString ReadPathBase = FString([NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0]) + TEXT("/");
                       return ReadPathBase + Result.ToLower();
                   }
            else if (bIsIterative)
            {
                static FString ReadPathBase = FString([NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES) objectAtIndex:0]) + TEXT("/");
                
                return ReadPathBase + Result.ToLower();
            }
            else
            {
                static FString ReadPathBase = FString([[NSBundle mainBundle] bundlePath]) + TEXT("/cookeddata/");
                iPhoneLaunched = false;
                
                return ReadPathBase + Result;
            }
        }
        return Result;
    }
#endif

//<==============="【SET FİLTER】"==================>
           
void FARMarkerDevice::SetFilter(bool enabled, int sampleRate, int cutOffFreq)
    {
        //Sample rate
        
        if (sampleRate != 0)
        {
            this->filterSampleRate = sampleRate;
        }
        else
        {
            this->filterSampleRate = 1;
        }
               
        //Cut off frequency
        
        if (cutOffFreq != 0)
        {
            this->filterCutOffFreq = cutOffFreq;
            
        }
        else
        {
            this->filterCutOffFreq = 1;
        }
        
        //Filter on/off
        
        this->filterEnabled = enabled;
        
        //Apply filter for all NFT markers
        
        for (int i = 0; i < this->MarkersNFT.Num(); i++)
        {
            ApplyFilter(this->MarkersNFT[i]);
        }
        
        //Apply filter for all fiducial markers
        
        for (int i = 0; i < this->Markers.Num(); i++)
        {
            ApplyFilter(this->Markers[i]);
            
        }
    }
           
//<==============="【APPLY FİLTER】"==================>
           
void FARMarkerDevice::ApplyFilter(FMarker *marker)
    {
    marker->resetFilter = true;
        if (this->filterEnabled)
        {
            if (marker->ftmi != 0)
            {
                arFilterTransMatSetParams(marker->ftmi,this->filterSampleRate, this->filterCutOffFreq);
            }
            
            else
            {
                marker->ftmi = arFilterTransMatInit(this->filterSampleRate, this->filterCutOffFreq);
            }
        }
        else
        {
            marker->ftmi = 0;
            
        }
    }
           
//<==============="【GET FİLTER】"==================>
           
void FARMarkerDevice::GetFilter(bool &enabled, int &sampleRate, int &cutOffFreq)
    {
        sampleRate=this->filterSampleRate;
        cutOffFreq = this->filterCutOffFreq;
        enabled=this->filterEnabled;
    }
           
//<==============="【TOGGLE PAUSE】"==================>
           
void FARMarkerDevice::TogglePause(){
#if PLATFORM_IOS
    UIApplicationState state = [[UIApplication sharedApplication] applicationState];
    if (state == UIApplicationStateBackground || state == UIApplicationStateInactive)
    {
        if (!paused){
            //Stop camera
            arVideoCapStop();
            paused=true;
        }
    } else{
        if (paused){
            
            paused=false;
            //Restart camera feed
            arVideoCapStart();
        }
    }
#endif
}
           

int FARMarkerDevice::GetCameraResX(){
    return this->mWebcamResX;
}
int FARMarkerDevice::GetCameraResY(){
    return this->mWebcamResY;
}

EDeviceOrientation FARMarkerDevice::GetDeviceOrientation() {
    return this->deviceOrientation;
}
           
    

