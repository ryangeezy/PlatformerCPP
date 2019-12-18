// Copyright Ryan Gourley 2019

using UnrealBuildTool;
using System.Collections.Generic;

public class PlatformerCPPEditorTarget : TargetRules
{
	public PlatformerCPPEditorTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Editor;

		ExtraModuleNames.AddRange( new string[] { "PlatformerCPP" } );
	}
}
