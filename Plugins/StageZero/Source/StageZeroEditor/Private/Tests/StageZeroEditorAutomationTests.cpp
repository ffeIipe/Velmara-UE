#if WITH_DEV_AUTOMATION_TESTS

#include "Misc/AutomationTest.h"
#include "UObject/Package.h"
#include "UObject/UnrealType.h"
#include "StageZeroManifestLogParser.h"
#include "StageZeroManifestBuilder.h"
#include "StageZeroManifestOverrideSerialization.h"
#include "StageZeroManifestReflectionTestTypes.h"
#include "StageZeroWarmupManifest.h"
#include "StageZeroWarmupUtils.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageZeroWorldPartitionAuthoringOptionsTest,
    "StageZero.Editor.Manifest.WorldPartitionAuthoringOptions",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageZeroWorldPartitionAuthoringOptionsTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FStageZeroManifestBuildOptions Defaults;
    TestTrue(TEXT("World Partition collection is enabled by default"), Defaults.bIncludeWorldPartition);
    TestTrue(TEXT("Anchor and spline route authoring is enabled by default"), Defaults.bIncludeAnchorAndSplineRoutes);

    const FProperty* RouteProperty = FStageZeroManifestBuildOptions::StaticStruct()->FindPropertyByName(
        GET_MEMBER_NAME_CHECKED(FStageZeroManifestBuildOptions, bIncludeAnchorAndSplineRoutes));
    TestNotNull(TEXT("Anchor and spline route option is reflected"), RouteProperty);
    if (RouteProperty)
    {
        TestTrue(TEXT("Anchor and spline route option remains configurable"), RouteProperty->HasAnyPropertyFlags(CPF_Config));
        TestTrue(TEXT("Anchor and spline route option remains editable"), RouteProperty->HasAnyPropertyFlags(CPF_Edit));
        TestFalse(TEXT("Anchor and spline route option is not deprecated"), RouteProperty->HasAnyPropertyFlags(CPF_Deprecated));
    }

    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageZeroManualManifestOverrideSchemaTest,
    "StageZero.Editor.Manifest.ManualOverrideSchema",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageZeroManualManifestOverrideSchemaTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FStageZeroManifestBuildOptions Defaults;
    TestEqual(TEXT("Global includes are opt-in"), Defaults.GlobalAdditionalAssets.Num(), 0);
    TestEqual(TEXT("Global exclusions are opt-in"), Defaults.GlobalExcludedAssets.Num(), 0);
    TestEqual(TEXT("Per-map overrides are opt-in"), Defaults.PerMapAssetOverrides.Num(), 0);

    const UScriptStruct* OptionsStruct = FStageZeroManifestBuildOptions::StaticStruct();
    const FProperty* GlobalIncludes = OptionsStruct->FindPropertyByName(
        GET_MEMBER_NAME_CHECKED(FStageZeroManifestBuildOptions, GlobalAdditionalAssets));
    const FProperty* GlobalExcludes = OptionsStruct->FindPropertyByName(
        GET_MEMBER_NAME_CHECKED(FStageZeroManifestBuildOptions, GlobalExcludedAssets));
    const FProperty* PerMapOverrides = OptionsStruct->FindPropertyByName(
        GET_MEMBER_NAME_CHECKED(FStageZeroManifestBuildOptions, PerMapAssetOverrides));
    TestNotNull(TEXT("Global include authoring input is reflected"), GlobalIncludes);
    TestNotNull(TEXT("Global exclude authoring input is reflected"), GlobalExcludes);
    TestNotNull(TEXT("Per-map authoring input is reflected"), PerMapOverrides);
    for (const FProperty* Property : { GlobalIncludes, GlobalExcludes, PerMapOverrides })
    {
        if (Property)
        {
            TestTrue(TEXT("Manual override input is editable"), Property->HasAnyPropertyFlags(CPF_Edit));
            TestTrue(TEXT("Manual override input is configurable"), Property->HasAnyPropertyFlags(CPF_Config));
            TestFalse(TEXT("Manual override input is not deprecated"), Property->HasAnyPropertyFlags(CPF_Deprecated));
        }
    }

    const UScriptStruct* MapOverrideStruct = FStageZeroManifestMapAssetOverrides::StaticStruct();
    TestNotNull(TEXT("Per-map override has an exact map key"), MapOverrideStruct->FindPropertyByName(
        GET_MEMBER_NAME_CHECKED(FStageZeroManifestMapAssetOverrides, Map)));
    TestNotNull(TEXT("Per-map override has additional assets"), MapOverrideStruct->FindPropertyByName(
        GET_MEMBER_NAME_CHECKED(FStageZeroManifestMapAssetOverrides, AdditionalAssets)));
    TestNotNull(TEXT("Per-map override has excluded assets"), MapOverrideStruct->FindPropertyByName(
        GET_MEMBER_NAME_CHECKED(FStageZeroManifestMapAssetOverrides, ExcludedAssets)));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageZeroManualManifestOverrideSerializationTest,
    "StageZero.Editor.Manifest.ManualOverrideSerialization",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageZeroManualManifestOverrideSerializationTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    FStageZeroManifestMapAssetOverrides Override;
    Override.Map = FSoftObjectPath(TEXT("/Game/Maps/L_Override.L_Override"));
    const FSoftObjectPath Material(TEXT("/Game/Materials/M_Override.M_Override"));
    const FSoftObjectPath Texture(TEXT("/Game/Textures/T_Override.T_Override"));
    Override.AdditionalAssets = { Texture, Material, Material };
    Override.ExcludedAssets = { Texture };

    FString Encoded;
    FString Error;
    TestTrue(TEXT("A valid per-map override serializes"),
        StageZeroManifestOverrideSerialization::EncodeMapOverride(Override, Encoded, Error));

    FStageZeroManifestMapAssetOverrides Decoded;
    TestTrue(TEXT("A serialized per-map override parses"),
        StageZeroManifestOverrideSerialization::DecodeMapOverride(Encoded, Decoded, Error));
    TestEqual(TEXT("Map identity survives round trip"), Decoded.Map.ToString(), Override.Map.ToString());
    TestEqual(TEXT("Duplicate includes are normalized"), Decoded.AdditionalAssets.Num(), 2);
    TestTrue(TEXT("Material include survives round trip"), Decoded.AdditionalAssets.Contains(Material));
    TestTrue(TEXT("Texture include survives round trip"), Decoded.AdditionalAssets.Contains(Texture));
    TestEqual(TEXT("Exclusion survives round trip"), Decoded.ExcludedAssets.Num(), 1);

    TestFalse(TEXT("Malformed per-map records are rejected"),
        StageZeroManifestOverrideSerialization::DecodeMapOverride(TEXT("/Game/Maps/L_Bad.L_Bad|only-two-fields"), Decoded, Error));
    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageZeroWorldPartitionManifestValidationTest,
    "StageZero.Editor.Manifest.WorldPartitionRecordsRequireAssets",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageZeroWorldPartitionManifestValidationTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UStageZeroWarmupManifest* Manifest = NewObject<UStageZeroWarmupManifest>(GetTransientPackage());
    TestNotNull(TEXT("Manifest validation fixture can be constructed"), Manifest);
    if (!Manifest)
    {
        return false;
    }

    const FName MapPackage(TEXT("/Game/StageZeroTests/WP_Validation"));
    const FSoftObjectPath MaterialPath(TEXT("/Game/StageZeroTests/M_Validation.M_Validation"));
    FStageZeroMapMatTexEntry& MapEntry = Manifest->MapEntries.AddDefaulted_GetRef();
    MapEntry.MapPackageName = MapPackage;
    MapEntry.Materials.Add(MaterialPath);
    Manifest->AllMaterials.Add(MaterialPath);

    FString FailureReason;
    TestTrue(TEXT("Baseline manifest fixture is valid"),
        FStageZeroManifestBuilder::ValidateManifestContentsForTesting(Manifest, 1, FailureReason));

    FStageZeroWPCellEntry& EmptyCell = Manifest->WPCells.AddDefaulted_GetRef();
    EmptyCell.MapPackageName = MapPackage;
    EmptyCell.CellId = TEXT("ActorCoverage");
    EmptyCell.Bounds = FBox(
        FVector(-100.0, -100.0, -100.0),
        FVector(100.0, 100.0, 100.0));
    TestFalse(TEXT("Empty World Partition coverage records are rejected"),
        FStageZeroManifestBuilder::ValidateManifestContentsForTesting(Manifest, 1, FailureReason));
    EmptyCell.Materials.Add(MaterialPath);
    TestTrue(TEXT("Typed World Partition coverage records are accepted"),
        FStageZeroManifestBuilder::ValidateManifestContentsForTesting(Manifest, 1, FailureReason));
    Manifest->WPCells.Reset();

    FStageZeroWPAnchorGroup& EmptyAnchor = Manifest->WPAnchorGroups.AddDefaulted_GetRef();
    EmptyAnchor.MapPackageName = MapPackage;
    EmptyAnchor.AnchorId = TEXT("AnchorValidation");
    EmptyAnchor.Radius = 100.0f;
    TestFalse(TEXT("Empty World Partition anchor groups are rejected"),
        FStageZeroManifestBuilder::ValidateManifestContentsForTesting(Manifest, 1, FailureReason));
    EmptyAnchor.Materials.Add(MaterialPath);
    TestTrue(TEXT("Typed World Partition anchor groups are accepted"),
        FStageZeroManifestBuilder::ValidateManifestContentsForTesting(Manifest, 1, FailureReason));
    Manifest->WPAnchorGroups.Reset();

    FStageZeroWPSplineGroup& EmptyRoute = Manifest->WPSplineGroups.AddDefaulted_GetRef();
    EmptyRoute.MapPackageName = MapPackage;
    EmptyRoute.RouteId = TEXT("RouteValidation");
    EmptyRoute.SampleRadius = 100.0f;
    TestFalse(TEXT("Empty World Partition spline groups are rejected"),
        FStageZeroManifestBuilder::ValidateManifestContentsForTesting(Manifest, 1, FailureReason));
    EmptyRoute.Materials.Add(MaterialPath);
    TestTrue(TEXT("Typed World Partition spline groups are accepted"),
        FStageZeroManifestBuilder::ValidateManifestContentsForTesting(Manifest, 1, FailureReason));

    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageZeroManifestSummaryContractTest,
    "StageZero.Editor.Manifest.StructuredSummaryContract",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageZeroManifestSummaryContractTest::RunTest(const FString& Parameters)
{
    const FString CompleteSummary = TEXT(
        "prefix\nStageZeroBuildManifestSummary: Schema=2 Requested=2 Resolved=2 Missing=0 Processed=2 MapsWithAssets=2 ZeroAssetMaps=0 WorldLoadFailures=0 Saved=1 Preserved=1 ForcedAllZeroSave=0 Succeeded=1 MissingMaps=None MissingReasons=None Reason=None Complete=1\n");
    FStageZeroManifestBuildResult Result;
    TestTrue(TEXT("A complete schema-2 summary parses"), StageZeroManifestLog::ParseSummary(CompleteSummary, Result));
    TestTrue(TEXT("Success flag is preserved"), Result.bSucceeded);
    TestEqual(TEXT("Requested count is preserved"), Result.NumRequestedMaps, 2);

    const FString TruncatedSummary = TEXT(
        "StageZeroBuildManifestSummary: Schema=2 Requested=1 Resolved=1 Missing=0 Processed=1 Saved=1 Succeeded=1 Complete=1");
    TestFalse(TEXT("A truncated summary is never accepted as success"), StageZeroManifestLog::ParseSummary(TruncatedSummary, Result));

    const FString MissingSelectionSummary = TEXT(
        "StageZeroBuildManifestSummary: Schema=2 Requested=2 Resolved=1 Missing=1 Processed=0 MapsWithAssets=0 ZeroAssetMaps=0 WorldLoadFailures=0 Saved=0 Preserved=1 ForcedAllZeroSave=0 Succeeded=0 MissingMaps=/Game/Missing.Missing MissingReasons=/Game/Missing.Missing:NotFound Reason=Strict_selection_failed Complete=1");
    TestTrue(TEXT("A complete failure summary remains machine-readable"), StageZeroManifestLog::ParseSummary(MissingSelectionSummary, Result));
    TestFalse(TEXT("Failure is not promoted to success"), Result.bSucceeded);
    TestEqual(TEXT("Missing map detail is retained"), Result.MissingRequestedMaps.Num(), 1);

    const FString ContradictorySuccessSummary = TEXT(
        "StageZeroBuildManifestSummary: Schema=2 Requested=2 Resolved=1 Missing=1 Processed=1 MapsWithAssets=1 ZeroAssetMaps=0 WorldLoadFailures=0 Saved=1 Preserved=1 ForcedAllZeroSave=0 Succeeded=1 MissingMaps=/Game/Missing.Missing MissingReasons=/Game/Missing.Missing:NotFound Reason=None Complete=1");
    TestFalse(TEXT("A contradictory success summary is rejected"), StageZeroManifestLog::ParseSummary(ContradictorySuccessSummary, Result));

    const FString InvalidBooleanSummary = TEXT(
        "StageZeroBuildManifestSummary: Schema=2 Requested=1 Resolved=1 Missing=0 Processed=1 MapsWithAssets=1 ZeroAssetMaps=0 WorldLoadFailures=0 Saved=2 Preserved=1 ForcedAllZeroSave=0 Succeeded=1 MissingMaps=None MissingReasons=None Reason=None Complete=1");
    TestFalse(TEXT("Non-boolean status fields are rejected"), StageZeroManifestLog::ParseSummary(InvalidBooleanSummary, Result));

    const FString FailedWorldLoadSummary = TEXT(
        "StageZeroBuildManifestSummary: Schema=2 Requested=1 Resolved=1 Missing=0 Processed=1 MapsWithAssets=1 ZeroAssetMaps=0 WorldLoadFailures=1 Saved=0 Preserved=1 ForcedAllZeroSave=0 Succeeded=0 MissingMaps=None MissingReasons=None Reason=World_load_failed Complete=1");
    TestTrue(TEXT("A complete world-load failure summary remains machine-readable"), StageZeroManifestLog::ParseSummary(FailedWorldLoadSummary, Result));
    TestFalse(TEXT("A world-load failure is never promoted to success"), Result.bSucceeded);

    const FString InvalidIntegerSummary = TEXT(
        "StageZeroBuildManifestSummary: Schema=2 Requested=1.0 Resolved=1 Missing=0 Processed=1 MapsWithAssets=1 ZeroAssetMaps=0 WorldLoadFailures=0 Saved=1 Preserved=1 ForcedAllZeroSave=0 Succeeded=1 MissingMaps=None MissingReasons=None Reason=None Complete=1");
    TestFalse(TEXT("Non-canonical integer fields are rejected"), StageZeroManifestLog::ParseSummary(InvalidIntegerSummary, Result));

    const FString TrailingIntegerJunkSummary = TEXT(
        "StageZeroBuildManifestSummary: Schema=2 Requested=1 Resolved=1 Missing=0 Processed=1 MapsWithAssets=1 ZeroAssetMaps=0 WorldLoadFailures=0 Saved=1junk Preserved=1 ForcedAllZeroSave=0 Succeeded=1 MissingMaps=None MissingReasons=None Reason=None Complete=1");
    TestFalse(TEXT("Integer fields with trailing junk are rejected"), StageZeroManifestLog::ParseSummary(TrailingIntegerJunkSummary, Result));

    const FString DuplicateFieldSummary = TEXT(
        "StageZeroBuildManifestSummary: Schema=2 Requested=1 Requested=1 Resolved=1 Missing=0 Processed=1 MapsWithAssets=1 ZeroAssetMaps=0 WorldLoadFailures=0 Saved=1 Preserved=1 ForcedAllZeroSave=0 Succeeded=1 MissingMaps=None MissingReasons=None Reason=None Complete=1");
    TestFalse(TEXT("Duplicate structured-summary fields are rejected"), StageZeroManifestLog::ParseSummary(DuplicateFieldSummary, Result));

    const FString ForcedAllZeroSummary = TEXT(
        "StageZeroBuildManifestSummary: Schema=2 Requested=1 Resolved=1 Missing=0 Processed=1 MapsWithAssets=0 ZeroAssetMaps=1 WorldLoadFailures=0 Saved=1 Preserved=1 ForcedAllZeroSave=1 Succeeded=1 MissingMaps=None MissingReasons=None Reason=None Complete=1");
    TestFalse(TEXT("Diagnostic all-zero saves are no longer an accepted production result"), StageZeroManifestLog::ParseSummary(ForcedAllZeroSummary, Result));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageZeroRuntimePackagePolicyTest,
    "StageZero.Editor.Manifest.RuntimePackagePolicy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageZeroRuntimePackagePolicyTest::RunTest(const FString& Parameters)
{
    TestTrue(TEXT("Game assets are eligible"), FStageZeroWarmupUtils::IsRuntimeWarmupPackageAllowed(FName(TEXT("/Game/Runtime/M_Runtime"))));
    TestFalse(TEXT("Developer assets are excluded"), FStageZeroWarmupUtils::IsRuntimeWarmupPackageAllowed(FName(TEXT("/Game/Developers/User/M_Test"))));
    TestFalse(TEXT("Editor engine assets are excluded"), FStageZeroWarmupUtils::IsRuntimeWarmupPackageAllowed(FName(TEXT("/Engine/EditorMaterials/M_Editor"))));
    TestFalse(TEXT("Script packages are excluded"), FStageZeroWarmupUtils::IsRuntimeWarmupPackageAllowed(FName(TEXT("/Script/Engine"))));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageZeroUnloadedSoftReferenceTraversalTest,
    "StageZero.Editor.Manifest.UnloadedSoftReferenceTraversal",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageZeroUnloadedSoftReferenceTraversalTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FSoftObjectPath MaterialPath(TEXT("/Game/StageZeroTests/M_Unloaded.M_Unloaded"));
    const FSoftObjectPath TexturePath(TEXT("/Game/StageZeroTests/T_Unloaded.T_Unloaded"));
    const FSoftObjectPath NestedTexturePath(TEXT("/Game/StageZeroTests/T_NestedUnloaded.T_NestedUnloaded"));
    const FSoftObjectPath NiagaraPath(TEXT("/Game/StageZeroTests/NS_Unloaded.NS_Unloaded"));
    const FSoftObjectPath StaticMeshPath(TEXT("/Game/StageZeroTests/SM_Unloaded.SM_Unloaded"));
    const FSoftObjectPath SkeletalMeshPath(TEXT("/Game/StageZeroTests/SK_Unloaded.SK_Unloaded"));

    UStageZeroManifestReflectionTestObject* Source =
        NewObject<UStageZeroManifestReflectionTestObject>(GetTransientPackage());
    TestNotNull(TEXT("Reflection traversal fixture can be constructed"), Source);
    if (!Source)
    {
        return false;
    }

    Source->DirectMaterial = TSoftObjectPtr<UMaterialInterface>(MaterialPath);
    Source->TextureArray.Add(TSoftObjectPtr<UTexture>(TexturePath));
    Source->NiagaraSet.Add(TSoftObjectPtr<UNiagaraSystem>(NiagaraPath));
    Source->MeshMap.Add(
        TSoftObjectPtr<UStaticMesh>(StaticMeshPath),
        TSoftObjectPtr<USkeletalMesh>(SkeletalMeshPath));
    Source->NestedReferences.Texture = TSoftObjectPtr<UTexture>(NestedTexturePath);

    TestNull(TEXT("Material fixture remains unloaded"), MaterialPath.ResolveObject());
    TestNull(TEXT("Texture fixture remains unloaded"), TexturePath.ResolveObject());
    TestNull(TEXT("Nested texture fixture remains unloaded"), NestedTexturePath.ResolveObject());
    TestNull(TEXT("Niagara fixture remains unloaded"), NiagaraPath.ResolveObject());
    TestNull(TEXT("Static-mesh fixture remains unloaded"), StaticMeshPath.ResolveObject());
    TestNull(TEXT("Skeletal-mesh fixture remains unloaded"), SkeletalMeshPath.ResolveObject());

    const FStageZeroManifestReflectedAssetReferences References =
        FStageZeroManifestBuilder::CollectReflectedAssetReferencesForTesting(Source, true);

    TestTrue(TEXT("Direct unloaded material reference is retained"), References.Materials.Contains(MaterialPath));
    TestTrue(TEXT("Array unloaded texture reference is retained"), References.Textures.Contains(TexturePath));
    TestTrue(TEXT("Nested-struct unloaded texture reference is retained"), References.Textures.Contains(NestedTexturePath));
    TestTrue(TEXT("Set unloaded Niagara reference is retained"), References.NiagaraSystems.Contains(NiagaraPath));
    TestTrue(TEXT("Map-key unloaded static-mesh reference is retained"), References.StaticMeshes.Contains(StaticMeshPath));
    TestTrue(TEXT("Map-value unloaded skeletal-mesh reference is retained"), References.SkeletalMeshes.Contains(SkeletalMeshPath));

    return !HasAnyErrors();
}

#endif
