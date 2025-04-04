#include "Player/PlayerMainAnimInstance.h"
#include "Player/PlayerMain.h"
#include "Components/PlayerFormComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void UPlayerMainAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	PlayerMain = Cast<APlayerMain>(TryGetPawnOwner());
	if (PlayerMain)
	{
		PlayerMainCharacterMovement = PlayerMain->GetCharacterMovement();
		PlayerFormComponent = PlayerMain->PlayerFormComponent;
	}
}

void UPlayerMainAnimInstance::NativeUpdateAnimation(float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (PlayerMainCharacterMovement)
	{
		GroundSpeed = UKismetMathLibrary::VSizeXY(PlayerMainCharacterMovement->Velocity);
		IsFalling = PlayerMainCharacterMovement->IsFalling();
		CharacterState = PlayerMain->GetCharacterState();
	}

	if (PlayerFormComponent)
	{
		PlayerForm = PlayerFormComponent->GetCharacterForm();
	}
}
