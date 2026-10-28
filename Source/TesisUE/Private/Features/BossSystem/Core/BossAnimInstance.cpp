#include "Features/BossSystem/Core/BossAnimInstance.h"

#include "Features/BossSystem/Components/BossCombatComponent.h"
#include "Features/BossSystem/Core/BossEnemy.h"
#include "GameFramework/CharacterMovementComponent.h"

void UBossAnimInstance::NativeUpdateAnimation(const float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	const APawn* Pawn = TryGetPawnOwner();
	if (!Pawn) return;

	GroundSpeed = Pawn->GetVelocity().Size2D();

	if (const ACharacter* Character = Cast<ACharacter>(Pawn))
	{
		bIsInAir = Character->GetCharacterMovement() && Character->GetCharacterMovement()->IsFalling();
	}

	if (const ABossEnemy* Boss = Cast<ABossEnemy>(Pawn))
	{
		bIsDead = Boss->IsDefeated();

		if (const UBossCombatComponent* Combat = Boss->GetCombatComponent())
		{
			bIsBusy = Combat->IsBusy();
			Stage = Combat->GetStage();
		}
	}
}
