// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "IARMarker.h"
#include "Engine.h"

#if PLATFORM_WINDOWS
#include "AllowWindowsPlatformTypes.h"

#include <dshow.h>
#pragma comment(lib, "strmiids")
#endif

#if PLATFORM_MAC || PLATFORM_IOS
#import <AVFoundation/AVFoundation.h>
#import <CoreVideo/CoreVideo.h>
#endif

#include "AR/video.h"
#include "AR/ar.h"
#include "AR/config.h"
#include "AR/param.h"

//NFT
#include "AR/arMulti.h"
#include "AR/arFilterTransMat.h"
#include "AR2/tracking.h"

//KPM
#include "trackingSub.h"

#if PLATFORM_WINDOWS
#include "HideWindowsPlatformTypes.h"
#endif

/**
 * 
 */
class ARMarkerPrivatePCH
{
public:
	ARMarkerPrivatePCH();
	~ARMarkerPrivatePCH();
};
