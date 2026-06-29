// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;
using System.Collections.Generic;

public class CrownsBaneTarget : TargetRules
{
    public CrownsBaneTarget(TargetInfo Target) : base(Target)
    {
        Type = TargetType.Game;
        DefaultBuildSettings = BuildSettingsVersion.V6;
        IncludeOrderVersion = EngineIncludeOrderVersion.Latest;
        ExtraModuleNames.Add("CrownsBane");

        // Ігноруємо конфлікт налаштувань для лаунчер-версії рушія
        bOverrideBuildEnvironment = true;
    }
}
