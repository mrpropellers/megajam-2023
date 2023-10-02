// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class AntiquatedFuture : ModuleRules
{
	public AntiquatedFuture(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{ 
			"Core", 
			"CoreUObject", 
			"Engine", 
			"InputCore", 
			"EnhancedInput",
			"Niagara",
			"OnlineSubsystem",
			"OnlineSubsystemEOS",
			"OnlineSubsystemUtils",
			"OnlineSubsystemEOSPlus",
			"OnlineSubsystemSteam"
		});
	}
}
