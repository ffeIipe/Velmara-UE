// Copyright (c) 2025 71 Studios. All rights reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "StageZeroProjectSettings.h"
#include "StageZeroWarmup.h"

#include "Components/PostProcessComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Engine/World.h"
#include "Materials/Material.h"
#include "Misc/AutomationTest.h"
#include "PackedLevelActor/PackedLevelActor.h"
#include "UObject/Package.h"

namespace
{
template <typename AssetType, typename ExpectedType>
bool ContainsLoadedAsset(const TArray<TSoftObjectPtr<AssetType>>& Assets, const ExpectedType* Expected)
{
    const AssetType* ExpectedAsset = Expected;
    return Assets.ContainsByPredicate([ExpectedAsset](const TSoftObjectPtr<AssetType>& Asset)
    {
        return Asset.Get() == ExpectedAsset;
    });
}

UStaticMeshComponent* AddMaterialComponent(AActor* Owner, UMaterialInterface* Material)
{
    UStaticMeshComponent* Component = NewObject<UStaticMeshComponent>(Owner);
    Owner->AddInstanceComponent(Component);
    Component->SetMaterial(0, Material);
    return Component;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageZeroLoadedWorldCollectorTest,
    "StageZero.Compatibility.LoadedWorldCollector",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageZeroLoadedWorldCollectorTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
    TestNotNull(TEXT("A transient test world can be created"), World);
    if (!World)
    {
        return false;
    }
    World->AddToRoot();

    UStageZeroProjectSettings* Settings = GetMutableDefault<UStageZeroProjectSettings>();
    const bool bPreviousLevelInstances = Settings->bWarmUpLevelInstances;
    const bool bPreviousPackedLevelActors = Settings->bWarmUpPackedLevelActors;
    Settings->bWarmUpLevelInstances = true;
    Settings->bWarmUpPackedLevelActors = true;

    AActor* Actor = World->SpawnActor<AActor>();
    UMaterial* Material = NewObject<UMaterial>(GetTransientPackage());
    UTexture2D* Texture = NewObject<UTexture2D>(GetTransientPackage());
    TestNotNull(TEXT("A test actor can be spawned"), Actor);
    TestNotNull(TEXT("A test material can be created"), Material);
    TestNotNull(TEXT("A test texture can be created"), Texture);

    if (Actor && Material && Texture)
    {
        AddMaterialComponent(Actor, Material);
        UPostProcessComponent* PostProcess = NewObject<UPostProcessComponent>(Actor);
        Actor->AddInstanceComponent(PostProcess);
        FWeightedBlendable& TextureBlendable = PostProcess->Settings.WeightedBlendables.Array.AddDefaulted_GetRef();
        TextureBlendable.Weight = 1.0f;
        TextureBlendable.Object = Texture;

        FStageZeroWarmupSet LoadedWorldSet;
        UStageZeroWarmup::AutoCollectAssets(
            TEXT("Not/A/Package"), LoadedWorldSet, World, 0,
            /*bUseLoadedWorldFallback*/ true,
            /*bAllowPackageFallback*/ false);
        TestTrue(TEXT("Already-loaded component material is discovered"), ContainsLoadedAsset(LoadedWorldSet.Materials, Material));
        TestTrue(TEXT("Already-loaded post-process texture is discovered"), ContainsLoadedAsset(LoadedWorldSet.Textures, Texture));

        FStageZeroWarmupSet DisabledSet;
        UStageZeroWarmup::AutoCollectAssets(
            TEXT("Not/A/Package"), DisabledSet, World, 0,
            /*bUseLoadedWorldFallback*/ false,
            /*bAllowPackageFallback*/ false);
        TestEqual(TEXT("Disabling both discovery routes produces no assets"), DisabledSet.Materials.Num() + DisabledSet.Textures.Num(), 0);

        FStageZeroWarmupSet CappedSet;
        UStageZeroWarmup::AutoCollectAssets(
            TEXT("Not/A/Package"), CappedSet, World, 1,
            /*bUseLoadedWorldFallback*/ true,
            /*bAllowPackageFallback*/ false);
        TestEqual(TEXT("Loaded-world discovery honors the shared material/texture cap"), CappedSet.Materials.Num() + CappedSet.Textures.Num(), 1);
    }

    APackedLevelActor* PackedActor = World->SpawnActor<APackedLevelActor>();
    UMaterial* PackedMaterial = NewObject<UMaterial>(GetTransientPackage());
    TestNotNull(TEXT("A packed-level actor can be spawned"), PackedActor);
    if (PackedActor && PackedMaterial)
    {
        AddMaterialComponent(PackedActor, PackedMaterial);

        Settings->bWarmUpPackedLevelActors = false;
        FStageZeroWarmupSet ExcludedPackedSet;
        UStageZeroWarmup::AutoCollectAssets(
            FString(), ExcludedPackedSet, World, 0,
            /*bUseLoadedWorldFallback*/ true,
            /*bAllowPackageFallback*/ false);
        TestFalse(TEXT("Packed Level Actor assets obey the exclusion setting"), ContainsLoadedAsset(ExcludedPackedSet.Materials, PackedMaterial));

        Settings->bWarmUpPackedLevelActors = true;
        FStageZeroWarmupSet IncludedPackedSet;
        UStageZeroWarmup::AutoCollectAssets(
            FString(), IncludedPackedSet, World, 0,
            /*bUseLoadedWorldFallback*/ true,
            /*bAllowPackageFallback*/ false);
        TestTrue(TEXT("Packed Level Actor assets obey the inclusion setting"), ContainsLoadedAsset(IncludedPackedSet.Materials, PackedMaterial));
    }

    Settings->bWarmUpLevelInstances = bPreviousLevelInstances;
    Settings->bWarmUpPackedLevelActors = bPreviousPackedLevelActors;
    World->DestroyWorld(false);
    World->RemoveFromRoot();

    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
