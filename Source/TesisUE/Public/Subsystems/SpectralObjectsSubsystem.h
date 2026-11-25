#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "SpectralObjectsSubsystem.generated.h"

class ISpectral;
class UMeshComponent;
class UMaterialInterface;
class UMaterialInstanceDynamic;
class UNiagaraSystem;
class USoundBase;
class USoundAttenuation;

UCLASS()
class TESISUE_API USpectralObjectsSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()


public:
	UFUNCTION(BlueprintCallable)
	void AddSpectralObject(AActor* Actor);

	UFUNCTION(BlueprintCallable)
	void ActivateSpectralObjects();

	UFUNCTION(BlueprintCallable)
	void ActivateSpectralObjectsInRange(const FVector& Origin, float Radius, float Duration = 8.f);

	UFUNCTION(BlueprintCallable)
	void DeactivateSpectralObjects();

	UFUNCTION(BlueprintPure, Category = "Spectral")
	bool IsActorRevealed(const AActor* Actor) const;

	UFUNCTION(BlueprintPure, Category = "Spectral")
	int32 GetRevealedObjectCount() const;

private:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	struct FMeshVisualBackup
	{
		TWeakObjectPtr<UMeshComponent> Mesh;
		TWeakObjectPtr<UMaterialInterface> PreviousOverlay;
		TArray<TWeakObjectPtr<UMaterialInterface>> OriginalMaterials;
	};

	struct FRevealedEntry
	{
		double ExpireTime = 0.0;
		double LastTickTime = 0.0;
		float PulsePhase = 0.f;
		bool bWarningActive = false;
		bool bDissolveActive = false;
		TArray<FMeshVisualBackup> MeshBackups;
		TWeakObjectPtr<UMaterialInstanceDynamic> OverlayMID;
		TWeakObjectPtr<UMaterialInstanceDynamic> DissolveMID;
	};

	void UpdateRevealedObjects();
	void EnsureUpdateTimer();
	void EnsureVisualAssetsLoaded();
	void CaptureMeshBackups(AActor* Actor, FRevealedEntry& Entry) const;
	void RestoreActorVisuals(AActor* Actor, FRevealedEntry& Entry);
	void RestoreAllVisuals();

	TArray<TScriptInterface<ISpectral>> SpectralObjects;

	TMap<TWeakObjectPtr<AActor>, FRevealedEntry> RevealedEntries;
	double LastVanishSoundTime = -1000.0;
	FTimerHandle UpdateTimer;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DecayOverlayMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UMaterialInterface> DissolveMaterial;

	UPROPERTY(Transient)
	TObjectPtr<UNiagaraSystem> VanishNiagaraSystem;

	UPROPERTY(Transient)
	TObjectPtr<USoundBase> VanishSound;

	UPROPERTY(Transient)
	TObjectPtr<USoundAttenuation> VanishAttenuation;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UMaterialInstanceDynamic>> ActiveDynamicMaterials;
};
