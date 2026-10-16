using UnrealBuildTool;
using System.IO;

public class ProjectFPSAnimGraph : ModuleRules
{
	public ProjectFPSAnimGraph(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
		PublicDependencyModuleNames.AddRange(new string[] { "Core", "CoreUObject", "Engine", "AnimGraph", "AnimGraphRuntime", "ProjectFPS" });
		PrivateDependencyModuleNames.AddRange(new string[] { "UnrealEd", "BlueprintGraph", "KismetCompiler" });
		PublicIncludePaths.Add(Path.Combine(ModuleDirectory, "../ProjectFPS"));
	}
}
