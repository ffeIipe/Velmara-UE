using UnrealBuildTool;

public class TesisUE : ModuleRules
{
	public TesisUE(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		PublicDependencyModuleNames.AddRange(new string[]
		{
			"Core",
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
			"UMG",
			"Slate",
			"SlateCore",
			"CommonUI",
			"CommonInput",
			"AudioModulation"
		});

		PrivateDependencyModuleNames.AddRange(new string[]
		{
			"DeveloperSettings",
			"GeometryCollectionEngine",
			"ChaosSolverEngine",
			"StageZero",
			"ApplicationCore",
			"MoviePlayer"
		});

		if (Target.bBuildEditor)
		{
			PrivateDependencyModuleNames.AddRange(new string[]
			{
				"FractureEngine",
				"DataflowCore",
				"ClothingSystemRuntimeCommon",
				"ClothingSystemRuntimeInterface",
				"ClothingSystemEditorInterface",
				"AssetRegistry"
			});
		}

		bUseUnity = true;
	}
}
