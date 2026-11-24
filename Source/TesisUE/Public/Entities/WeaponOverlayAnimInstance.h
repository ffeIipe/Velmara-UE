#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "DataAssets/Items/Weapons/WeaponData.h"
#include "WeaponOverlayAnimInstance.generated.h"

class UAnimSequence;
class UMeshComponent;
class ACharacter;
class UWeaponData;

UCLASS()
class TESISUE_API UWeaponOverlayAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	static void LinkToCharacter(const ACharacter* Character, const UWeaponData* WeaponData, const UMeshComponent* WeaponMesh);

	static void UnlinkFromCharacter(const ACharacter* Character, const UWeaponData* WeaponData);

	void ApplyOverlay(const FWeaponOverlaySettings& InSettings, const UMeshComponent* WeaponMesh);

	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	FWeaponOverlaySettings Settings;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Overlay")
	TObjectPtr<UAnimSequence> BasePose;

	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	float OverlayAlpha = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	FVector LeftHandIKLocation = FVector::ZeroVector;

	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	float EffectiveLeftHandIKWeight = 0.f;

	UPROPERTY(BlueprintReadOnly, Category = "Overlay")
	float EffectiveLegsWeight = 0.f;

	UPROPERTY(EditDefaultsOnly, Category = "Overlay")
	float OverlayBlendSpeed = 8.f;

	UPROPERTY(EditDefaultsOnly, Category = "Overlay")
	FName FullBodyMontageSlot = "DefaultSlot";

	UPROPERTY(EditDefaultsOnly, Category = "Overlay")
	float IdleSpeedThreshold = 10.f;

	UPROPERTY(EditDefaultsOnly, Category = "Overlay")
	float IdleLegsBlendSpeed = 6.f;

private:
	bool bHasLeftHandIKSocket = false;

	float LocomotionBlendAlpha = 0.f;

	float IdleLegsAlpha = 0.f;
};
