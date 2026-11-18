using UnrealBuildTool;

public class StageZeroBootstrap : ModuleRules
{
    public StageZeroBootstrap(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PrecompileForTargets = PrecompileTargetsType.Any;

        PublicDependencyModuleNames.Add("Core");
        PrivateDependencyModuleNames.AddRange(
            new string[]
            {
                "RenderCore",
                "RHI"
            });
    }
}
