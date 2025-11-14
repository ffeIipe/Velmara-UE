#include "Enemy/Spectre.h"
#include "Components/AttributeComponent.h"
#include "Components/CapsuleComponent.h"
#include "DamageTypes/PistolDamage.h"

#include "Engine/DamageEvents.h"
#include "GameFramework/DamageType.h"
#include "Kismet/KismetMathLibrary.h"

ASpectre::ASpectre()
{
	GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
}

void ASpectre::PerformSpectralAttack()
{
	if (const int RandomValue = UKismetMathLibrary::RandomIntegerInRange(0, SpectralAttackMontages.Max()); SpectralAttackMontages.IsValidIndex(RandomValue))
	{
		Execute_PlayAnimMontage(this, SpectralAttackMontages[RandomValue], 1.f, "Default");
	}
}

void ASpectre::HandleEnemyCollision(const bool bEnable)
{
	Super::HandleEnemyCollision(bEnable);

	if (bEnable)
	{
		GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

		GetMesh()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);
		GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);

		GetAttributeComponent()->GetShieldMeshComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Block);
	}
}

float ASpectre::TakeDamage(const float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	if (DamageEvent.DamageTypeClass->IsChildOf(UPistolDamage::StaticClass()))
	{
		Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);
	}
	return DamageAmount;
}

bool ASpectre::IsLaunchable()
{
	return true;
}

void ASpectre::ApplyPossessionParameters(bool bShouldEnable)
{
	
}
