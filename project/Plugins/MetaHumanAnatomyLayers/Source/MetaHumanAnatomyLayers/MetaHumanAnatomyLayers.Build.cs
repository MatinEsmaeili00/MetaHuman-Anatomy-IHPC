using UnrealBuildTool;

public class MetaHumanAnatomyLayers : ModuleRules
{
	public MetaHumanAnatomyLayers(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine" });
	}
}
