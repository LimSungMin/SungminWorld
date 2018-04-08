// Copyright 1998-2018 Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class SungminWorld : ModuleRules
{
	public SungminWorld(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "InputCore", "HeadMountedDisplay", "UMG","Slate", "SlateCore" });

        //PublicIncludePaths.Add(@"C:\Users\lsm_o\Documents\Unreal Projects\SungminWorld\PacketDll");
        //PublicLibraryPaths.Add(@"C:\Users\lsm_o\Documents\Unreal Projects\SungminWorld\x64\Release\PacketDll");
        //PublicAdditionalLibraries.Add(@"C:\Users\lsm_o\Documents\Unreal Projects\SungminWorld\x64\Release\PacketDll.lib");
        //PublicDelayLoadDLLs.Add(@"C:\Users\lsm_o\Documents\Unreal Projects\SungminWorld\x64\Release\PacketDll.dll");
    }
}
