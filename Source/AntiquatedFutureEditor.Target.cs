// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class AntiquatedFutureEditorTarget : TargetRules
{
	public AntiquatedFutureEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;
		bUsesSteam = true;
		DefaultBuildSettings = BuildSettingsVersion.V4;
		IncludeOrderVersion = EngineIncludeOrderVersion.Unreal5_3;
		ExtraModuleNames.Add("AntiquatedFuture");
	}
}
