// Copyright 2024 Crown's Bane. All Rights Reserved.

using UnrealBuildTool;
using System.IO;

public class CrownsBane : ModuleRules
{
    public CrownsBane(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        // Використовуємо ModuleDirectory для автоматичного правильного шляху від кореня
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
            "Niagara" // ДОДАНО: Для візуальних ефектів вибухів та стрільби
		});

        PrivateDependencyModuleNames.AddRange(new string[]
        {
            "RenderCore"
        });
    }
}