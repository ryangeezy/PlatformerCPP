// Copyright 2020 Ryan Gourley

using UnrealBuildTool;
using System.Collections.Generic;

public class PlatformerCPPTarget : TargetRules
{
	public PlatformerCPPTarget(TargetInfo Target) : base(Target)
	{
		Type = TargetType.Game;

		ExtraModuleNames.AddRange( new string[] { "PlatformerCPP" } );
	}
}
