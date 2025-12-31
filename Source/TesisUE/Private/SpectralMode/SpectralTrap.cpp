#include "SpectralMode/SpectralTrap.h"
#include "Player/PlayerMain.h"
#include "Components/BoxComponent.h"
#include "DamageTypes/EnvironmentalDamage.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"

ASpectralTrap::ASpectralTrap()
{
	OnPlayerBeginOverlap.AddDynamic(this, &ASpectralTrap::StartDamage);
	OnPlayerEndOverlap.AddDynamic(this, &ASpectralTrap::FinishDamage);
}

void ASpectralTrap::BeginPlay()
{
	Super::BeginPlay();

	OnPlayerBeginOverlap.AddDynamic(this, &ASpectralTrap::StartDamage);
	OnPlayerEndOverlap.AddDynamic(this, &ASpectralTrap::FinishDamage);
	
	if (TrapSFX)
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), TrapSFX, BoxCollider->GetComponentLocation());
	}
}

void ASpectralTrap::StartDamage()
{
	if (!HitActor)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Not valid HitActor!");
		return;
	}

	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Blue, "Damage Started");
	
	TimeOnTarget = 0.f;
	
	//Add GameplayTag and stun
	
	DealContinuousDamage();
	
	GetWorld()->GetTimerManager().SetTimer(
			ContinuousDamageTimerHandle,
			this,
			&ASpectralTrap::DealContinuousDamage,
			DamageInterval,
			true);
}

void ASpectralTrap::FinishDamage()
{
	if (!HitActor) return;

	TimeOnTarget = 0.f;
	InitialDamage = 0.f;
	
	//Remove GameplayTag 
	GetWorld()->GetTimerManager().ClearTimer(ContinuousDamageTimerHandle);

	HitActor = nullptr;
}

void ASpectralTrap::DealContinuousDamage()
{
	if (!HitActor) return;

	TimeOnTarget += DamageInterval;

	if (DamageCurve && InitialDamage < MaxDamage)
	{
		InitialDamage = MaxDamage * DamageCurve->GetFloatValue(TimeOnTarget);

		if (GEngine) GEngine->AddOnScreenDebugMessage(4, 3.f, FColor::White, "Dealing damage: " + FString::SanitizeFloat(InitialDamage));
	}
	
	UGameplayStatics::ApplyDamage(Cast<AActor>(HitActor.GetObject()), InitialDamage, nullptr, this, UEnvironmentalDamage::StaticClass());
	
	const FDamageEvent DamageEvent(UEnvironmentalDamage::StaticClass());
	HitActor->GetHit(GetOwner(), FVector::ZeroVector, DamageEvent, InitialDamage);
}