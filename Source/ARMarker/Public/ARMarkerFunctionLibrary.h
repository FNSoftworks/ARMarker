// Fill out your copyright notice in the Description page of Project Settings.

#pragma once
#include <stdio.h>
#include "Runtime/Core/Public/GenericPlatform/GenericPlatform.h"
#include "ARMarkerPrivatePCH.h"
#include "ARMarkerFunctionLibrary.generated.h"


class ARMARKER_API ARMarkerFunctionLibrary
{
public:
	ARMarkerFunctionLibrary();
	~ARMarkerFunctionLibrary();
};

UENUM(BlueprintType)
enum class EArLabelingThresholdMode:uint8
{
    MANUAL UMETA(DisplayName = "Manual"),
    AUTO_MEDIAN UMETA(DisplayName = "Auto Median"),
    AUTO_OTSU UMETA(DisplayName = "Auto Otsu"),
    AUTO_ADAPTIVE UMETA(DisplayName = "Auto Adaptive"),
    AUTO_BRACKETING UMETA(DisplayName = "Auto Bracketing")
};

UENUM(BlueprintType)
enum class EArPatternDetectionMode:uint8
{
    TEMPLATE_MATCHING_COLOR UMETA(DisplayName = "Template Matching Color"),
    MATRIX_CODE_DETECTION UMETA(DisplayName = "Matrix code detection")
};

UENUM(BlueprintType)
enum class EDeviceOrientation:uint8
{
    LANDSCAPE UMETA(DisplayName = "Landscape"),
    PORTRAIT UMETA(DisplayName = "Portrait")
};

USTRUCT()
struct FMarker {
    
    GENERATED_USTRUCT_BODY()
    
    UPROPERTY()
    FName name;
    
    UPROPERTY()
    FVector position;
    
    UPROPERTY()
    FRotator rotation;
    
    UPROPERTY()
    FVector cameraPosition;
    
    UPROPERTY()
    FRotator cameraRotation;
    
    UPROPERTY()
    bool visible;
    
    ARdouble               matrix[3][4];
    
    //For NFT only
    int                        pageNo;
    
    ARFilterTransMatInfo*    ftmi;
    ARdouble                filterCutoffFrequency;
    ARdouble                filterSampleRate;
    
    bool                    valid;
    bool                    validPrev;
    
    bool                    resetFilter;
};



