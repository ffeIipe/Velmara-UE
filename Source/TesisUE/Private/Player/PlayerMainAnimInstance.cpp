#include "Player/PlayerMainAnimInstance.h"
#include "KismetAnimationLibrary.h"
#include "Player/PlayerMain.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SpectralWeaponComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/CharacterStateComponent.h"
#include "Components/CombatComponent.h"
#include "Components/ExtraMovementComponent.h"
#include "DataAssets/EntityData.h"

void UPlayerMainAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	PlayerMain = Cast<APlayerMain>(TryGetPawnOwner());
	if (PlayerMain)
	{
		PlayerMainCharacterMovement = PlayerMain->GetCharacterMovement();
		SpectralWeaponComponent = PlayerMain->GetComponentByClass<USpectralWeaponComponent>();
		MaxWalkSpeed = PlayerMainCharacterMovement->MaxWalkSpeed;
		CombatComponent = PlayerMain->GetCombatComponent();		
		
		if (PlayerMain->EntityData)
		{
			MaxRunSpeed = PlayerMain->EntityData->MovementData.MaxRunSpeed;	
		}
	}
}

void UPlayerMainAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (PlayerMainCharacterMovement)
	{
		GroundSpeed = UKismetMathLibrary::VSizeXY(PlayerMainCharacterMovement->Velocity);
		IsFalling = PlayerMainCharacterMovement->IsFalling();

		bHasAcceleration = PlayerMainCharacterMovement->GetCurrentAcceleration().SizeSquared2D() > SMALL_NUMBER;

		CharacterState = PlayerMain->GetCharacterStateComponent()->GetCurrentCharacterState().State;
		CharacterForm = PlayerMain->GetCharacterStateComponent()->GetCurrentCharacterState().Form;
		SpectralState = PlayerMain->GetCharacterStateComponent()->GetCurrentCharacterState().SpectralState;
		
		bIsLocking = CombatComponent->bIsHardLocking;
		
		Direction = UKismetAnimationLibrary::CalculateDirection(PlayerMain->GetVelocity(), PlayerMain->GetActorRotation());
	}
}
