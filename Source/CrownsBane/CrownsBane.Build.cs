// Copyright 2024 Crown's Bane. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class CrownsBane : ModuleRules
{
	public CrownsBane(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// Use ModuleDirectory so "#include "Ship/ShipPawn.h"" works from any subfolder
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"AIModule",
			"NavigationSystem",
			"GameplayTasks",
			"UMG",
			"Slate",
			"SlateCore",
			"Niagara"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"RenderCore"
		});
	}
}
