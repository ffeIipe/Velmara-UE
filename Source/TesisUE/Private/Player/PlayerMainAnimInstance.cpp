#include "Player/PlayerMainAnimInstance.h"
#include "Player/PlayerMain.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Components/CharacterStateComponent.h"

void UPlayerMainAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	PlayerMain = Cast<APlayerMain>(TryGetPawnOwner());
	if (PlayerMain)
	{
		PlayerMainCharacterMovement = PlayerMain->GetCharacterMovement();
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
	}
}
