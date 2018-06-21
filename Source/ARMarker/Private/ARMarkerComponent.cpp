// Fill out your copyright notice in the Description page of Project Settings.

#include "ARMarkerComponent.h"
#include "ARMarkerPrivatePCH.h"
#include "IARMarker.h"
#include "ARMarkerDevice.h"

UARMarkerComponent::UARMarkerComponent(const class FObjectInitializer& ObjectInitializer)
: Super(ObjectInitializer)
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.bStartWithTickEnabled = true;
    PrimaryComponentTick.TickGroup = TG_PrePhysics;
    bAutoActivate = true;
}

void UARMarkerComponent::TickComponent(float DeltaTime, enum ELevelTick TickType, FActorComponentTickFunction *ThisTickFunction)
{
    IARMarker::GetARMarkerDeviceSafe()->UpdateDevice();
}

FString UARMarkerComponent::Start(){
    return "Start ARMarker";
}

//<================="【GET CAMERA FRAME】"====================>

UTexture2D* UARMarkerComponent::GetCameraFrame(){
    return IARMarker::GetARMarkerDeviceSafe()->GetWebcamTexture();
}

//<================="【GET MARKER】"====================>

void UARMarkerComponent::GetMarker(uint8 markerId, FVector &Position, FRotator &Rotation, FVector &CameraPosition, FRotator &CameraRotation, bool &Visible){
    FMarker* marker = IARMarker::GetARMarkerDeviceSafe()->GetMarker(markerId);
    ProcessMarker(marker, Position, Rotation, CameraPosition, CameraRotation, Visible);
}

//<================="【GET MARKER NFT】"====================>

void UARMarkerComponent::GetMarkerNFT(uint8 markerId, FVector &Position, FRotator &Rotation, FVector &CameraPosition, FRotator &CameraRotation, bool &Visible){
    FMarker* marker = IARMarker::GetARMarkerDeviceSafe()->GetMarkerNFT(markerId);
    ProcessMarker(marker, Position, Rotation, CameraPosition, CameraRotation, Visible);
}
//<================="【PROCESS MARKER】"====================>

void UARMarkerComponent::ProcessMarker(FMarker *marker, FVector &Position, FRotator &Rotation, FVector &CameraPosition, FRotator &CameraRotation, bool &Visible){
    //Position landscape
    Position = marker->position;
    
    if (IARMarker::GetARMarkerDeviceSafe()->GetDeviceOrientation() == EDeviceOrientation::PORTRAIT) {
        Position = FRotator(0, 90, 0).RotateVector(marker->position);
    }
    
    //Rotation
    FRotator rotTemp;
    rotTemp.Yaw = marker->rotation.Yaw;
    rotTemp.Pitch = -marker->rotation.Pitch;
    rotTemp.Roll = 180 - marker->rotation.Roll;
    Rotation = rotTemp;
    
    //Rotation portrait mode
    if (IARMarker::GetARMarkerDeviceSafe()->GetDeviceOrientation() == EDeviceOrientation::PORTRAIT) {
        Rotation = (FQuat(rotTemp)*FQuat(FRotator(0, 90, 0))).Rotator();
    }
    
    //Visibility
    Visible = marker->visible;
    
    //Camera rotation landscape
    rotTemp.Roll = marker->cameraRotation.Roll;
    rotTemp.Pitch = -marker->cameraRotation.Pitch;
    rotTemp.Yaw = -marker->cameraRotation.Yaw;
    CameraRotation = (FQuat(rotTemp)*FQuat(FRotator(90, 0, -90))).Rotator();
    
    //Camera rotation portrait
    if (IARMarker::GetARMarkerDeviceSafe()->GetDeviceOrientation() == EDeviceOrientation::PORTRAIT) {
        CameraRotation.Roll = CameraRotation.Roll - 90;
    }
    
    CameraPosition = marker->cameraPosition;
}

//<================="【INIT】"====================>

bool UARMarkerComponent::Init(bool showPIN, uint8 devNum, EDeviceOrientation deviceOrientation, EArPatternDetectionMode detectionMode, int32 &WebcamResX, int32 &WebcamResY, bool &FirstRunIOS)
{
    switch (deviceOrientation) {
        case EDeviceOrientation::LANDSCAPE:
        {
            return IARMarker::GetARMarkerDeviceSafe()->Init(showPIN, devNum, deviceOrientation,detectionMode, WebcamResX, WebcamResY, FirstRunIOS);
            break;
        }
        case EDeviceOrientation::PORTRAIT:
        {
            return IARMarker::GetARMarkerDeviceSafe()->Init(showPIN, devNum, deviceOrientation,detectionMode, WebcamResX, WebcamResY, FirstRunIOS);
            break;
        }
    }
    return false;
}

//<================="【 CREATE DYNAMİC MATERİAL INSTANCE 】"====================>

UMaterialInstanceDynamic* UARMarkerComponent::CreateDynamicMaterialInstance(UStaticMeshComponent *Mesh, UMaterial *SourceMaterial){
    UMaterialInstanceDynamic* DynamicMaterial = UMaterialInstanceDynamic::Create(SourceMaterial, this);
    Mesh->SetMaterial(0, DynamicMaterial);
    return DynamicMaterial;
}

//<================="【 SET TEXTURE PARAMETER VALUE 】"====================>

void UARMarkerComponent::SetTextureParameterValue(UMaterialInstanceDynamic *SourceMaterial, UTexture2D *Texture, FName Param){
    SourceMaterial->SetTextureParameterValue(Param, Texture);
}
//<================="【 CLEANUP 】"====================>

void UARMarkerComponent::Cleanup(){
    IARMarker::GetARMarkerDeviceSafe()->Cleanup();
}

//<================="【 LOAD MARKERS 】"====================>

bool UARMarkerComponent::LoadMarkers(TArray<FString> markers){
    return IARMarker::GetARMarkerDeviceSafe()->LoadMarkers(markers);
}

//<================="【 LOAD MARKERS NFT 】"====================>

bool UARMarkerComponent::LoadMarkersNFT(TArray<FString> markers){
    return IARMarker::GetARMarkerDeviceSafe()->LoadMarkersNFT(markers);
}

//<================="【 SET THRESHOLD MODE 】"====================>

void UARMarkerComponent::SetThresholdMode(EArLabelingThresholdMode mode){
    IARMarker::GetARMarkerDeviceSafe()->SetThresholdMode(mode);
}

//<================="【 NEW 】"====================>
