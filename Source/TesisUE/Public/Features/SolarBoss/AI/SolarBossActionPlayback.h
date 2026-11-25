#pragma once

#include "CoreMinimal.h"

class ACharacter;
class UAnimMontage;

struct FSolarBossActionPlayback
{
	TWeakObjectPtr<UAnimMontage> Montage;
	float Elapsed = 0.f;
	float FallbackDuration = 1.f;
	float Progress = 0.f;

	void Start(ACharacter* Character, UAnimMontage* InMontage, float PlayRate, float InFallbackDuration);
	bool Advance(const ACharacter* Character, float DeltaSeconds);
	void Stop(const ACharacter* Character) const;
};
