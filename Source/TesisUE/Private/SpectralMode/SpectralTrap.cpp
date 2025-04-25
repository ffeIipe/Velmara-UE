// Fill out your copyright notice in the Description page of Project Settings.


#include "SpectralMode/SpectralTrap.h"
#include "Player/PlayerMain.h"
#include "Kismet/GameplayStatics.h"

void ASpectralTrap::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (Player)
	{
		OverlappingPlayer = Player;
		ApplyTrapDamage();
		//OverlappingPlayer->PlayAnimMontage(OverlappingPlayer->HitReactMontage, 1.f, FName("KnockDown"));
	}
}

void ASpectralTrap::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnSphereEndOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);

	if (Player && Player == OverlappingPlayer)
	{
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
		UDamageType::StaticClass() //TODO: change the type of damage, bc it overrides the AnimMontage applied
	);
}
