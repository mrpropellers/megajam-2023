// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class AntiquatedFutureTarget : TargetRules
{
	public AntiquatedFutureTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;
        bUsesSteam = true;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
		ExtraModuleNames.Add("AntiquatedFuture");
	}
}
