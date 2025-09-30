#include "SpectralMode/SpectralTrap.h"
#include "Player/PlayerMain.h"
#include "Components/BoxComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "DamageTypes/SpectralTrapDamageType.h"


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
	
	HitActor->AddStunBehavior();
	
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
	CurrentDamage = 0.0f;
	
	HitActor->RemoveStunBehavior();
	GetWorld()->GetTimerManager().ClearTimer(ContinuousDamageTimerHandle);

	HitActor = nullptr;
}

void ASpectralTrap::DealContinuousDamage()
{
	if (!HitActor) return;

	TimeOnTarget += DamageInterval;

	if (DamageCurve && CurrentDamage < MaxDamage)
	{
		CurrentDamage = MaxDamage * DamageCurve->GetFloatValue(TimeOnTarget);

		if (GEngine) GEngine->AddOnScreenDebugMessage(4, 3.f, FColor::White, "Dealing damage: " + FString::SanitizeFloat(CurrentDamage));
	}
	
	UGameplayStatics::ApplyDamage(Cast<AActor>(HitActor.GetObject()), CurrentDamage, nullptr, this, USpectralTrapDamageType::StaticClass());
	
	const FDamageEvent DamageEvent(USpectralTrapDamageType::StaticClass());
	HitActor->GetHit(GetOwner(), FVector::ZeroVector, DamageEvent, CurrentDamage);
}