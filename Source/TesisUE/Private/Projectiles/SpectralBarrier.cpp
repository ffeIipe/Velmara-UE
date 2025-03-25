// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectiles/SpectralBarrier.h"
#include "Player/PlayerMain.h"
#include <Kismet/GameplayStatics.h>

ASpectralBarrier::ASpectralBarrier()
{
	bEnableDestroyOnCollision = false;
}

void ASpectralBarrier::BeginPlay()
{
	Super::BeginPlay();

	if (Player)
	{
		EnemyTargets = Player->GetEnemyTargets();
	}
}

void ASpectralBarrier::OnProjectileImpact(AActor* OtherActor, const FHitResult& Hit)
{
	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			HitSound,
			Hit.ImpactPoint
		);
	}
	if (HitParticles)
	{
		UGameplayStatics::SpawnEmitterAtLocation(
			GetWorld(),
			HitParticles,
			Hit.ImpactPoint
		);
	}
}
