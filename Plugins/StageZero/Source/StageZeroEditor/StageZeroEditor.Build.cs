using UnrealBuildTool;

public class StageZeroEditor : ModuleRules
{
    public StageZeroEditor(ReadOnlyTargetRules Target) : base(Target)
    {
        Type = ModuleType.CPlusPlus;

        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

        PublicDependencyModuleNames.AddRange(
            new string[]
            {
                "Core",
                "StageZero"
            }
        );

        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "CoreUObject",
                "Engine",
                "Slate",
                "SlateCore",
                "InputCore",
                "EditorStyle",
                "Projects",
                "UnrealEd",
                "AssetRegistry",
                "Niagara",
                "RHI",
                "ToolMenus",
                "PropertyEditor",
                "ContentBrowser",
                "DeveloperToolSettings"
            }
        );
    }
}
