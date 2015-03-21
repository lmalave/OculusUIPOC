// Copyright 1998-2015 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class OculusUIPOC : ModuleRules
{
	public OculusUIPOC(TargetInfo Target)
	{
        PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay" });
        PrivateDependencyModuleNames.AddRange(new string[] { "CoherentUIPlugin", "CoherentUI" });
        if (Target.Platform == UnrealTargetPlatform.Win64)
        {
            PublicAdditionalLibraries.Add("D:/LeapSDK/lib/x64/Leap.lib");
            PublicIncludePaths.AddRange(new string[] { "D:/LeapSDK/include" });
        }
        else if (Target.Platform == UnrealTargetPlatform.Mac)
        {
            PublicAdditionalLibraries.Add("/Users/lmalave/Documents/LeapSDK/lib/libLeap.dylib");
            PublicIncludePaths.AddRange(new string[] { "/Users/lmalave/Documents/LeapSDK/include" });
        }
    }
}
