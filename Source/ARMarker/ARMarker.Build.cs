// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using System;
using System.IO;

namespace UnrealBuildTool.Rules
{
public class ARMarker : ModuleRules
{
	public ARMarker(ReadOnlyTargetRules ROTargetRules) : base(ROTargetRules)
	{
		PCHUsage = ModuleRules.PCHUsageMode.UseExplicitOrSharedPCHs;
		
		PublicIncludePaths.AddRange(
			new string[] {
				"ARMarker/Public",
                "Runtime/Core/Public/Android",
                "Runtime/Launch/Private/Android",
                "Runtime/Launch/Public/Android"
			}
			);
				
		
		PrivateIncludePaths.AddRange(
			new string[] {
				"ARMarker/Private",
			}
			);
			
		
		PublicDependencyModuleNames.AddRange(
			new string[]
			{
				"Core",
                "CoreUObject",
                "Engine",
                "EngineSettings",
                "Slate",
                "SlateCore",
                "RHI",
                "RenderCore",
                "ShaderCore",
				"ARMarkerLibrary",
				"Projects"
			}
			);

        if (Target.Platform == UnrealTargetPlatform.Android)
           {
           // Additional dependencies on android...
           PrivateDependencyModuleNames.Add("Launch");

           // Register Plugin Language
           AdditionalPropertiesForReceipt.Add("AndroidPlugin", Path.Combine(ModuleDirectory, "ARMarker_APL.xml"));
           }
	}
}
}
