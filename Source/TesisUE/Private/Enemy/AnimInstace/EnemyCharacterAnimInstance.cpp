#include "Enemy/AnimInstace/EnemyCharacterAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "DataAssets/EntityData.h"

void UEnemyCharacterAnimInstance::NativeInitializeAnimation()
{
	OwningEnemy = Cast<AEnemy>(TryGetPawnOwner()); 

	if (OwningEnemy)
	{
		OwningMovementComponent = OwningEnemy->GetCharacterMovement();
		MaxWalkSpeed = OwningMovementComponent->MaxWalkSpeed;
		if (OwningEnemy->EntityData)
		{
			//MaxRunSpeed = OwningEnemy->EntityData->MovementData.MaxRunSpeed;
		}
		/*else if (GEngine)
			GEngine->AddOnScreenDebugMessage(INDEX_NONE,3.f,FColor::Red,"Entity Data not found!");*/
	}
}

void UEnemyCharacterAnimInstance::NativeThreadSafeUpdateAnimation(float DeltaSeconds)
{
	if (!OwningEnemy || !OwningMovementComponent)
	{
		return;
	}

	GroundSpeed = OwningEnemy->GetVelocity().Size2D();
	bIsFalling = OwningMovementComponent->IsFalling();
	bHasAcceleration = OwningMovementComponent->GetCurrentAcceleration().SizeSquared2D() > 0.f;
	CurrentEnemyState = OwningEnemy->GetEnemyState();
}