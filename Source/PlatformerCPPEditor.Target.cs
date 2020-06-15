// Copyright 2020 Ryan Gourley

using UnrealBuildTool;
using System.Collections.Generic;

public class PlatformerCPPEditorTarget : TargetRules
{
	public PlatformerCPPEditorTarget(TargetInfo Target) : base(Target)
	{
		DefaultBuildSettings = BuildSettingsVersion.V2; //added as recommended in Build Output 2/1/2020
		Type = TargetType.Editor;

		ExtraModuleNames.AddRange( new string[] { "PlatformerCPP" } );
	}
}
