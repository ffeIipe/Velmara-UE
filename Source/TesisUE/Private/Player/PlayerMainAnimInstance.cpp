#include "Player/PlayerMainAnimInstance.h"
#include "Player/PlayerMain.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/SpectralWeaponComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/CharacterStateComponent.h"

void UPlayerMainAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	PlayerMain = Cast<APlayerMain>(TryGetPawnOwner());
	if (PlayerMain)
	{
		PlayerMainCharacterMovement = PlayerMain->GetCharacterMovement();
		SpectralWeaponComponent = PlayerMain->GetComponentByClass<USpectralWeaponComponent>();
	}
}

void UPlayerMainAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (PlayerMainCharacterMovement)
	{
		GroundSpeed = UKismetMathLibrary::VSizeXY(PlayerMainCharacterMovement->Velocity);
		IsFalling = PlayerMainCharacterMovement->IsFalling();
		CharacterState = PlayerMain->CharacterStateComponent->GetCurrentCharacterState().State;
		CharacterForm = PlayerMain->CharacterStateComponent->GetCurrentCharacterState().Form;
		SpectralWeaponState = SpectralWeaponComponent->GetSpectralWeaponState();

		WalkRight = CalculateWalkRight();
		WalkForward = CalculateWalkForward();
	}
}

float const UPlayerMainAnimInstance::CalculateWalkRight()
{
	return UKismetMathLibrary::Sin(FMath::DegreesToRadians(Direction)) * UKismetMathLibrary::Clamp(GroundSpeed, 0, 1);
}

float const UPlayerMainAnimInstance::CalculateWalkForward()
{
	return UKismetMathLibrary::Cos(FMath::DegreesToRadians(Direction)) * UKismetMathLibrary::Clamp(GroundSpeed, 0, 1);
}
