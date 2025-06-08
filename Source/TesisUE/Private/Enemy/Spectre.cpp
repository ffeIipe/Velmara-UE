#include "Enemy/Spectre.h"
#include "Components/AttributeComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/CombatComponent.h"
#include "Kismet/GameplayStatics.h"

#include "Engine/DamageEvents.h"
#include "GameFramework/DamageType.h"
#include "DamageTypes/SpectralTrapDamageType.h"
#include <Kismet/KismetMathLibrary.h>

ASpectre::ASpectre()
{
	GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
}

void ASpectre::BeginPlay()
{
	Super::BeginPlay();

	if (Attributes)
	{
		Attributes->OnEntityDead.AddLambda(
			[this]
			{
				Die(DamageCauserOf);
			}
		);
	}
}

void ASpectre::PerformSpectralAttack()
{
	int RandomValue = UKismetMathLibrary::RandomIntegerInRange(0, SpectralAttackMontages.Max());
	if (SpectralAttackMontages.IsValidIndex(RandomValue))
	{
		PlayAnimMontage(SpectralAttackMontages[RandomValue]);
	}
}

float ASpectre::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (DamageEvent.DamageTypeClass == USpectralTrapDamageType::StaticClass())
	{
		Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	}
	return DamageAmount;
}

bool ASpectre::IsLaunchable_Implementation(ACharacter* Character)
{
	return true;
}

bool ASpectre::CanBeFinished_Implementation()
{
	return false; //no puede ser finisheada esta entidad
}

void ASpectre::GetHit_Implementation(const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType)
{
	Super::GetHit_Implementation(ImpactPoint, DamageType);
}

void ASpectre::Attack(const FInputActionValue& Value)
{
	Super::Attack(Value);

	CombatComponent->Input_Attack(Value);
}
