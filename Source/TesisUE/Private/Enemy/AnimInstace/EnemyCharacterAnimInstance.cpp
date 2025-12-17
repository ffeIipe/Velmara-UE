#include "Enemy/AnimInstace/EnemyCharacterAnimInstance.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include <Kismet/KismetMathLibrary.h>

void UEnemyCharacterAnimInstance::NativeInitializeAnimation()
{
	OwningEnemy = Cast<AEnemy>(TryGetPawnOwner()); 

	if (OwningEnemy)
	{
		OwningMovementComponent = OwningEnemy->GetCharacterMovement();
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

	//WalkRight = CalculateWalkRight();
	//WalkForward = CalculateWalkForward();
}

//float const UEnemyCharacterAnimInstance::CalculateWalkRight()
//{
//	if (CurrentEnemyState == EEnemyState::EES_Walk)
//	{
//		return (UKismetMathLibrary::Sin(FMath::DegreesToRadians(Direction)) * UKismetMathLibrary::Clamp(GroundSpeed, 0, 1)) / 2;
//	}
//	else if (CurrentEnemyState == EEnemyState::EES_Run)
//	{
//		return UKismetMathLibrary::Sin(FMath::DegreesToRadians(Direction)) * UKismetMathLibrary::Clamp(GroundSpeed, 0, 1);
//	}
//	return 0.f;
//}
//
//float const UEnemyCharacterAnimInstance::CalculateWalkForward()
//{
//	if (CurrentEnemyState == EEnemyState::EES_Walk)
//	{
//		return (UKismetMathLibrary::Cos(FMath::DegreesToRadians(Direction)) * UKismetMathLibrary::Clamp(GroundSpeed, 0, 1)) / 2;
//	}
//	else if (CurrentEnemyState == EEnemyState::EES_Run)
//	{
//		return UKismetMathLibrary::Cos(FMath::DegreesToRadians(Direction)) * UKismetMathLibrary::Clamp(GroundSpeed, 0, 1);
//	}
//	return 0.f;
//}
