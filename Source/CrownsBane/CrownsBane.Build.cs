// Copyright 2024 Crown's Bane. All Rights Reserved.

using UnrealBuildTool;

public class CrownsBane : ModuleRules
{
	public CrownsBane(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicIncludePaths.AddRange(new string[] {
			"CrownsBane",
			"CrownsBane/Ship",
			"CrownsBane/Combat",
			"CrownsBane/Components",
			"CrownsBane/AI",
			"CrownsBane/Loot",
			"CrownsBane/Player",
			"CrownsBane/Upgrades",
			"CrownsBane/Docks",
			"CrownsBane/Systems",
			"CrownsBane/UI"
		});

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
			"SlateCore"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"RenderCore",
			"Niagara"
		});
	}
}
