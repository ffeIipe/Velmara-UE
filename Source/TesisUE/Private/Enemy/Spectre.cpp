// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/Spectre.h"
#include "Components/AttributeComponent.h"
#include "Components/CapsuleComponent.h"
#include <Kismet/GameplayStatics.h>

void ASpectre::GetHit_Implementation(const FVector& ImpactPoint)
{
	if (Attributes && Attributes->IsAlive())
	{
		//TODO: here put the logic when the spectre receives a hit
	}
	else
	{
		Die();
	}

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			HitSound,
			ImpactPoint);
	}
	if (HitParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			HitParticles,
			ImpactPoint
		);
	}
}

void ASpectre::Die()
{
	Destroy();
}
