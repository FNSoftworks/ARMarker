#pragma once

#include "ModuleManager.h"


class IARMarker : public IModuleInterface
{
public:
    
    static inline IARMarker& Get()
    {
        return FModuleManager::LoadModuleChecked<IARMarker>("ARMarker");
    }
    
    static inline bool IsAvailable()
    {
        return FModuleManager::Get().IsModuleLoaded("ARMarker");
    }
    
    FORCEINLINE TSharedPtr<class FARMarkerDevice> GetARMarkerDevice() const
    {
        return ARMarkerDevice;
    }
    
   static FARMarkerDevice* GetARMarkerDeviceSafe()
    {
#if WITH_EDITOR
          FARMarkerDevice* ARMarkerDevice = IARMarker::IsAvailable() ? IARMarker::Get().GetARMarkerDevice().Get() : nullptr;
#else
          FARMarkerDevice* ARMarkerDevice = IARMarker::Get().GetARMarkerDevice().Get();
#endif
        return ARMarkerDevice;
    }
    
protected:
   TSharedPtr<class FARMarkerDevice> ARMarkerDevice;
};
