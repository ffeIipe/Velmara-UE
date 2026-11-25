#include "Features/SolarBoss/AI/SolarBossActionPlayback.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Character.h"

void FSolarBossActionPlayback::Start(ACharacter* Character, UAnimMontage* InMontage, const float PlayRate, const float InFallbackDuration)
{
	Montage = nullptr;
	Elapsed = 0.f;
	Progress = 0.f;
	FallbackDuration = FMath::Max(InFallbackDuration, 0.05f);

	if (Character && InMontage && Character->PlayAnimMontage(InMontage, PlayRate) > 0.f)
	{
		Montage = InMontage;
	}
}

bool FSolarBossActionPlayback::Advance(const ACharacter* Character, const float DeltaSeconds)
{
	Elapsed += DeltaSeconds;

	if (const UAnimMontage* Playing = Montage.Get())
	{
		const UAnimInstance* AnimInstance = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
		if (AnimInstance && AnimInstance->Montage_IsPlaying(Playing) && Playing->GetPlayLength() > 0.f)
		{
			Progress = AnimInstance->Montage_GetPosition(Playing) / Playing->GetPlayLength();
			return true;
		}

		Progress = 1.f;
		return false;
	}

	Progress = FMath::Min(Elapsed / FallbackDuration, 1.f);
	return Progress < 1.f;
}

void FSolarBossActionPlayback::Stop(const ACharacter* Character) const
{
	UAnimInstance* AnimInstance = Character ? Character->GetMesh()->GetAnimInstance() : nullptr;
	if (AnimInstance && Montage.IsValid() && AnimInstance->Montage_IsPlaying(Montage.Get()))
	{
		AnimInstance->Montage_Stop(0.2f, Montage.Get());
	}
}
