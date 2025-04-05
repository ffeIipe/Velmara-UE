// Fill out your copyright notice in the Description page of Project Settings.


#include "SpectralMode/SpectralTrap.h"
#include "Player/PlayerMain.h"
#include "Kismet/GameplayStatics.h"

void ASpectralTrap::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	{
		APlayerMain* Player = Cast<APlayerMain>(OtherActor);
		if (Player)
		{
			OverlappingPlayer = Player;
			GetWorldTimerManager().SetTimer(DamageTimerHandle, this, &ASpectralTrap::ApplyTrapDamage, DamageInterval, true);
		}
	}
}

void ASpectralTrap::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerMain* Player = Cast<APlayerMain>(OtherActor);
	if (Player && Player == OverlappingPlayer)
	{
		GetWorldTimerManager().ClearTimer(DamageTimerHandle);
		OverlappingPlayer = nullptr;
	}
}
void ASpectralTrap::ApplyTrapDamage()
{
	if (OverlappingPlayer && !OverlappingPlayer->IsPendingKill())
	{
		AController* InstigatorController = GetInstigator() ? GetInstigator()->GetController() : nullptr;

		UGameplayStatics::ApplyDamage(
			OverlappingPlayer,
			DamagePerTick,
			InstigatorController,
			this,
			UDamageType::StaticClass()
		);
	}
	else GetWorldTimerManager().ClearTimer(DamageTimerHandle);
}
