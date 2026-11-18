// Copyright (c) 2025 71 Studios for games Development. All rights reserved. 
using UnrealBuildTool;

public class StageZero : ModuleRules
{
    public StageZero(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrecompileForTargets = PrecompileTargetsType.Any;
        
        // Define debug macro by configuration (disable heavy diagnostics in Shipping)
        if (Target.Configuration == UnrealTargetConfiguration.Shipping)
        {
            PublicDefinitions.Add("STAGEZERO_ENABLE_DEBUG=0");
        }
        else
        {
            PublicDefinitions.Add("STAGEZERO_ENABLE_DEBUG=1");
        }

        // Core dependencies required for all builds
        PublicDependencyModuleNames.AddRange(new string[] {
            "Core",
            "CoreUObject",
            "Engine",
            "AssetRegistry",
            "DeveloperSettings",
            "InputCore",
            "MediaAssets",
            "SlateCore",
            "UMG" // Required for widget functionality
        });

        // Private implementation dependencies
        PrivateDependencyModuleNames.AddRange(new string[] {
            "StageZeroBootstrap",
            "AudioMixer",
            "RenderCore",
            "RHI",
            "Slate",
            "MoviePlayer",
            "Niagara"
        });

        if (Target.bBuildEditor)
        {
            // Used only by the editor-side cook callback to reject project packaging
            // exclusions that would otherwise silently suppress required packages.
            PrivateDependencyModuleNames.Add("DeveloperToolSettings");
        }
    }
}
