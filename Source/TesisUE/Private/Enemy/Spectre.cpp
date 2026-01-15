#include "Enemy/Spectre.h"
#include "Components/CapsuleComponent.h"

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
		PlayAnimMontage(SpectralAttackMontages[RandomValue], 1.f, "Default");
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
	}
}

void ASpectre::ApplyPossessionParameters(bool bShouldEnable)
{
	
}
