#pragma once

#include "CoreMinimal.h"
#include "StageZeroManifestReflectionTestTypes.generated.h"

class UMaterialInterface;
class UNiagaraSystem;
class USkeletalMesh;
class UStaticMesh;
class UTexture;

USTRUCT()
struct FStageZeroManifestNestedSoftReferences
{
    GENERATED_BODY()

    UPROPERTY()
    TSoftObjectPtr<UTexture> Texture;
};

UCLASS(Transient)
class UStageZeroManifestReflectionTestObject : public UObject
{
    GENERATED_BODY()

public:
    UPROPERTY()
    TSoftObjectPtr<UMaterialInterface> DirectMaterial;

    UPROPERTY()
    TArray<TSoftObjectPtr<UTexture>> TextureArray;

    UPROPERTY()
    TSet<TSoftObjectPtr<UNiagaraSystem>> NiagaraSet;

    UPROPERTY()
    TMap<TSoftObjectPtr<UStaticMesh>, TSoftObjectPtr<USkeletalMesh>> MeshMap;

    UPROPERTY()
    FStageZeroManifestNestedSoftReferences NestedReferences;
};
