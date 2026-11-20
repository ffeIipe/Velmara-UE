using UnrealBuildTool;

public class TesisUE : ModuleRules
{
	public TesisUE(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
	
		PublicDependencyModuleNames.AddRange(new string[] { "Core",
			"CoreUObject",
			"Engine",
			"InputCore",
			"EnhancedInput",
			"Niagara",
			"MediaAssets",
			"AdpcmAudioDecoder",
			"FieldSystemEngine",
			"Chaos",
			"GameplayTags",
			"GameplayTasks",
			"GameplayAbilities",
			"MotionWarping",
			"MotionTrajectory",
			"PoseSearch",
			"NetCore",
			"AnimGraphRuntime", 
			"AIModule",
			"NavigationSystem",
			"UMG"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"AnimGraphRuntime",
			"DeveloperSettings",
			"GeometryCollectionEngine",
			"ChaosSolverEngine",
			"StageZero"
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[] { "FractureEngine", "DataflowCore", "AssetRegistry" });
		}

        bUseUnity = true;
		
		PrivateDependencyModuleNames.AddRange(new string[] { "Slate", "SlateCore" });

    }
}
