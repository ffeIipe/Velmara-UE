#include "Entities/WeaponOverlayAnimInstance.h"

#include "Animation/AnimSequence.h"
#include "Components/MeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

void UWeaponOverlayAnimInstance::LinkToCharacter(const ACharacter* Character, const UWeaponData* WeaponData, const UMeshComponent* WeaponMesh)
{
	if (!Character || !WeaponData || !IsValid(WeaponData->AnimLayer)) return;

	UAnimInstance* MainAnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (!MainAnimInstance) return;

	MainAnimInstance->LinkAnimClassLayers(WeaponData->AnimLayer);

	if (UWeaponOverlayAnimInstance* Overlay = Cast<UWeaponOverlayAnimInstance>(MainAnimInstance->GetLinkedAnimLayerInstanceByClass(WeaponData->AnimLayer)))
	{
		Overlay->ApplyOverlay(WeaponData->Overlay, WeaponMesh);
	}
}

void UWeaponOverlayAnimInstance::UnlinkFromCharacter(const ACharacter* Character, const UWeaponData* WeaponData)
{
	if (!Character || !WeaponData || !IsValid(WeaponData->AnimLayer)) return;

	if (UAnimInstance* MainAnimInstance = Character->GetMesh() ? Character->GetMesh()->GetAnimInstance() : nullptr)
	{
		MainAnimInstance->UnlinkAnimClassLayers(WeaponData->AnimLayer);
	}
}

void UWeaponOverlayAnimInstance::ApplyOverlay(const FWeaponOverlaySettings& InSettings, const UMeshComponent* WeaponMesh)
{
	Settings = InSettings;

	USkeletalMeshComponent* OwnerMesh = GetSkelMeshComponent();
	bHasLeftHandIKSocket = IsValid(WeaponMesh) && OwnerMesh && WeaponMesh->DoesSocketExist(Settings.LeftHandIKSocket);

	if (bHasLeftHandIKSocket)
	{
		LeftHandIKLocation = WeaponMesh->GetSocketTransform(Settings.LeftHandIKSocket)
			.GetRelativeTransform(OwnerMesh->GetSocketTransform("hand_r"))
			.GetLocation();
	}
}

void UWeaponOverlayAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const ACharacter* CharacterOwner = Cast<ACharacter>(TryGetPawnOwner());
	const bool bFlying = CharacterOwner && CharacterOwner->GetCharacterMovement() && CharacterOwner->GetCharacterMovement()->MovementMode == MOVE_Flying;
	const float Target = bFlying ? 0.f : 1.f;

	LocomotionBlendAlpha = FMath::FInterpTo(LocomotionBlendAlpha, Target, DeltaSeconds, OverlayBlendSpeed);

	const UAnimInstance* MainAnimInstance = GetSkelMeshComponent() ? GetSkelMeshComponent()->GetAnimInstance() : nullptr;
	const float MontageWeight = MainAnimInstance ? MainAnimInstance->GetSlotMontageGlobalWeight(FullBodyMontageSlot) : 0.f;

	OverlayAlpha = LocomotionBlendAlpha * (1.f - MontageWeight);
	EffectiveLeftHandIKWeight = bHasLeftHandIKSocket ? Settings.LeftHandIKWeight * OverlayAlpha : 0.f;

	const UCharacterMovementComponent* Movement = CharacterOwner ? CharacterOwner->GetCharacterMovement() : nullptr;
	const bool bStandingStill = Movement && Movement->IsMovingOnGround() && Movement->Velocity.Size2D() < IdleSpeedThreshold;
	IdleLegsAlpha = FMath::FInterpTo(IdleLegsAlpha, bStandingStill ? 1.f : 0.f, DeltaSeconds, IdleLegsBlendSpeed);
	EffectiveLegsWeight = Settings.IdleLegsWeight * IdleLegsAlpha * OverlayAlpha;
}
