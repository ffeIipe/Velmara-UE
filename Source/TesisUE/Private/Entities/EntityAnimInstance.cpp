#include "Entities/EntityAnimInstance.h"

#include "KismetAnimationLibrary.h"
#include "Entities/Entity.h"
#include "Kismet/KismetMathLibrary.h"

void UEntityAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	EntityOwner = Cast<AEntity>(TryGetPawnOwner());

	if (EntityOwner)
	{
		CharacterMovementComponent = EntityOwner->GetCharacterMovementComponent();
	}
	
	if (CharacterMovementComponent)
	{
		MaxWalkSpeed = CharacterMovementComponent->MaxWalkSpeed;
	}
}

void UEntityAnimInstance::NativeUpdateAnimation(const float DeltaTime)
{
	Super::NativeUpdateAnimation(DeltaTime);

	if (CharacterMovementComponent)
	{
		GroundSpeed = UKismetMathLibrary::VSizeXY(CharacterMovementComponent->Velocity);

		bIsFalling = CharacterMovementComponent->IsFalling();

		bHasAcceleration = CharacterMovementComponent->GetCurrentAcceleration().SizeSquared2D() > SMALL_NUMBER;
		
		//bIsLocking = EntityOwner->IsLocking(); TODO: use GAS to establish this value 
		
		Direction = UKismetAnimationLibrary::CalculateDirection(CharacterMovementComponent->Velocity, EntityOwner->GetActorRotation());
	}

	if (bIsFalling)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(323, 1.f, FColor::Cyan, "Falling!");
	}
	else
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(323, 1.f, FColor::Red, "Not Falling!");
	}
}