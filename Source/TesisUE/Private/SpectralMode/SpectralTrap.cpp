// Fill out your copyright notice in the Description page of Project Settings.


#include "SpectralMode/SpectralTrap.h"
#include "Player/PlayerMain.h"
#include "Kismet/GameplayStatics.h"

void ASpectralTrap::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	APlayerMain* Player = Cast<APlayerMain>(OtherActor);
	if (Player)
	{
		OverlappingPlayer = Player;
		OverlappingPlayer->PlayAnimMontage(OverlappingPlayer->HitReactMontage, 1.f, FName("KnockDown"));
		//ApplyTrapDamage();
	}
}

void ASpectralTrap::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor,
	UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	APlayerMain* Player = Cast<APlayerMain>(OtherActor);
	if (Player && Player == OverlappingPlayer)
	{
		//GetWorldTimerManager().ClearTimer(DamageTimerHandle);
		OverlappingPlayer = nullptr;
	}
}
void ASpectralTrap::ApplyTrapDamage()
{
	AController* InstigatorController = GetInstigator() ? GetInstigator()->GetController() : nullptr;

	UGameplayStatics::ApplyDamage(
		OverlappingPlayer,
		Damage,
		InstigatorController,
		this,
		UDamageType::StaticClass()
	);
}
