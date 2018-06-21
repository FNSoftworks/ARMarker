// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "ARMarkerFunctionLibrary.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Components/ActorComponent.h"
#include "ARMarkerComponent.generated.h"

UCLASS( ClassGroup=(Custom), BlueprintType, Blueprintable, meta=(BlueprintSpawnableComponent) )
class UARMarkerComponent : public UActorComponent
{
	GENERATED_UCLASS_BODY()

/*public:
	UARMarkerComponent();*/

public:
    
//<================="【 ARMARKER CAMERA BP FUNCTION 】"====================>

UFUNCTION(BlueprintCallable, Category = "FNSoftworks|ARMarker|Marker::BlueprintLibrary")
void GetMarker(uint8 markerId, FVector& Position, FRotator& Rotation, FVector& CameraPosition, FRotator& CameraRotation, bool& Visible);
    
UFUNCTION(BlueprintCallable, Category = "FNSoftworks|ARMarker|Marker::BlueprintLibrary")
void GetMarkerNFT(uint8 markerId, FVector& Position, FRotator& Rotation, FVector& CameraPosition, FRotator& CameraRotation, bool& Visible);

UFUNCTION(BlueprintCallable, Category = "FNSoftworks|ARMarker|Camera::BlueprintLibrary")
FString Start();

//Camera Open Component
UFUNCTION(BlueprintCallable, Category = "FNSoftworks|ARMarker|Camera::BlueprintLibrary")
bool Init(bool showPIN, uint8 devNum, EDeviceOrientation deviceOrientation,EArPatternDetectionMode detectionMode, int32& WebcamResX, int32& WebcamResY, bool&FirstRunIOS);
    
UFUNCTION(BlueprintCallable, Category = "FNSoftworks|ARMarker|Camera::BlueprintLibrary")
UTexture2D* GetCameraFrame();
    
UFUNCTION(BlueprintCallable, Category = "FNSoftworks|ARMarker|Camera::BlueprintLibrary")
UMaterialInstanceDynamic* CreateDynamicMaterialInstance(UStaticMeshComponent* Mesh, UMaterial* SourceMaterial);
    
UFUNCTION(BlueprintCallable, Category = "FNSoftworks|ARMarker|Camera::BlueprintLibrary")
void SetTextureParameterValue(UMaterialInstanceDynamic* SourceMaterial, UTexture2D* Texture, FName Param);
    
UFUNCTION(BlueprintCallable, Category = "FNSoftworks|ARMarker|Camera::BlueprintLibrary")
void Cleanup();
    
//<================="【 ARMARKER MARKER BP FUNCTION 】"====================>
    
UFUNCTION(BlueprintCallable, Category = "FNSoftworks|ARMarker|Marker::BlueprintLibrary")
bool LoadMarkers(TArray<FString> markers);
    
UFUNCTION(BlueprintCallable, Category = "FNSoftworks|ARMarker|Marker::BlueprintLibrary")
bool LoadMarkersNFT(TArray<FString> markers);
    
UFUNCTION(BlueprintCallable, Category = "FNSoftworks|ARMarker|Marker::BlueprintLibrary")
void SetThresholdMode(EArLabelingThresholdMode mode);

//Camera Open Component

virtual void TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction);
		
protected:
    void ProcessMarker(FMarker* marker, FVector& Position, FRotator& Rotation, FVector& CameraPosition, FRotator& CameraRotation, bool& Visible);
};
