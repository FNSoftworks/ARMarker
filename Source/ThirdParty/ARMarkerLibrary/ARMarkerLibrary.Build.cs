// Fill out your copyright notice in the Description page of Project Settings.

using System;
using System.IO;
using UnrealBuildTool;

public class ARMarkerLibrary : ModuleRules
{
	public ARMarkerLibrary(ReadOnlyTargetRules ROTargetRules) : base(ROTargetRules)
	{
		Type = ModuleType.External;

        string SDKDIR = ModuleDirectory;
        SDKDIR = SDKDIR.Replace("\\", "/");

        if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Mac", "include/OSX/"));
            string LibPath = SDKDIR + "/Mac" + "/lib/OSX/";

            PublicLibraryPaths.Add(Path.Combine(ModuleDirectory, "Mac", "lib/OSX/"));
            PublicAdditionalLibraries.Add(LibPath + "macosx-universal/libjpeg.a");
            PublicAdditionalLibraries.Add(LibPath + "libAR.a");
            PublicAdditionalLibraries.Add(LibPath + "libAR2.a");
            PublicAdditionalLibraries.Add(LibPath + "libARgsub_lite.a");
            PublicAdditionalLibraries.Add(LibPath + "libARgsub.a");
            PublicAdditionalLibraries.Add(LibPath + "libARICP.a");
            PublicAdditionalLibraries.Add(LibPath + "libARMulti.a");
            PublicAdditionalLibraries.Add(LibPath + "libARosg.a");
            PublicAdditionalLibraries.Add(LibPath + "libARUtil.a");
            PublicAdditionalLibraries.Add(LibPath + "libARvideo.a");
            PublicAdditionalLibraries.Add(LibPath + "libEden.a");
            PublicAdditionalLibraries.Add(LibPath + "libKPM.a");

            PublicDelayLoadDLLs.Add(Path.Combine(ModuleDirectory, "Mac", "Release", "libExampleLibrary.dylib"));

            PublicFrameworks.AddRange(
            new string[] {
            "QTKit",
            "CoreVideo",
            "Accelerate"
            });
        }
        else if (Target.Platform == UnrealTargetPlatform.IOS)
        {
            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "iOS", "include/iOS/"));

            string LibPath = SDKDIR + "/iOS" + "/lib/iOS/";

            PublicLibraryPaths.Add(Path.Combine(ModuleDirectory, "iOS", "lib/iOS/"));

            PublicAdditionalLibraries.Add(LibPath + "libKPM.a");
            PublicAdditionalLibraries.Add(LibPath + "ios511/libjpeg.a");
            PublicAdditionalLibraries.Add(LibPath + "libAR.a");
            PublicAdditionalLibraries.Add(LibPath + "libAR2.a");
            PublicAdditionalLibraries.Add(LibPath + "libARgsub_es.a");
            PublicAdditionalLibraries.Add(LibPath + "libARgsub_es2.a");
            PublicAdditionalLibraries.Add(LibPath + "libARICP.a");
            PublicAdditionalLibraries.Add(LibPath + "libARMulti.a");
            PublicAdditionalLibraries.Add(LibPath + "libARosg.a");
            PublicAdditionalLibraries.Add(LibPath + "libARUtil.a");
            PublicAdditionalLibraries.Add(LibPath + "libARvideo.a");
            PublicAdditionalLibraries.Add(LibPath + "libARWrapper.a");
            PublicAdditionalLibraries.Add(LibPath + "libEden.a");
            PublicAdditionalLibraries.Add(LibPath + "libc++.dylib");


            PublicFrameworks.AddRange(
            new string[] {
            "CoreVideo",
            "Accelerate",
            "AVFoundation"
            });
        }
		else if (Target.Platform == UnrealTargetPlatform.Win64)
		{
            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "x64", "include/x64/"));

            string LibPath = SDKDIR + "/x64" + "/lib/vs2015/";

            PublicLibraryPaths.Add(Path.Combine(ModuleDirectory, "x64", "lib/x64/"));
            PublicAdditionalLibraries.Add(LibPath + "AR.lib");
            PublicAdditionalLibraries.Add(LibPath + "AR.lib");
            PublicAdditionalLibraries.Add(LibPath + "AR2.lib");
            PublicAdditionalLibraries.Add(LibPath + "ARICP.lib");
            PublicAdditionalLibraries.Add(LibPath + "ARMulti.lib");
            PublicAdditionalLibraries.Add(LibPath + "ARUtil.lib");
            PublicAdditionalLibraries.Add(LibPath + "ARvideo.lib");
            PublicAdditionalLibraries.Add(LibPath + "KPM.lib");
            PublicAdditionalLibraries.Add(LibPath + "libjpeg.lib");
            PublicAdditionalLibraries.Add(LibPath + "pthreadVC2.lib");

			PublicLibraryPaths.Add(Path.Combine(ModuleDirectory, "x64", "Release"));
			PublicAdditionalLibraries.Add("ExampleLibrary.lib");

            PublicDelayLoadDLLs.AddRange(new string[] {"ExampleLibrary.dll", "ARvideo.dll", "DSVL.dll", "pthreadVC2.dll"});
		}
        else if (Target.Platform == UnrealTargetPlatform.Android)
        {
            PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "Android", "include/Android/"));

            string LibPath = SDKDIR + "/Android" + "/lib/Android/armeabi-v7a/";

            PublicLibraryPaths.Add(Path.Combine(ModuleDirectory, "Android", "lib/Android/armeabi-v7a/"));
            PublicAdditionalLibraries.Add(LibPath + "libar.a");
            PublicAdditionalLibraries.Add(LibPath + "libar.a");
            PublicAdditionalLibraries.Add(LibPath + "libar.a");
            PublicAdditionalLibraries.Add(LibPath + "libar2.a");
            PublicAdditionalLibraries.Add(LibPath + "libaricp.a");
            PublicAdditionalLibraries.Add(LibPath + "libarmulti.a");
            PublicAdditionalLibraries.Add(LibPath + "libjpeg.a");
            PublicAdditionalLibraries.Add(LibPath + "libkpm.a");
            PublicAdditionalLibraries.Add(LibPath + "libutil.a");

            PublicAdditionalLibraries.Add(LibPath + "libc++_shared.so");
        }
	}
}
