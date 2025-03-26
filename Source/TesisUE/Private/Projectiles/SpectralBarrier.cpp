// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectiles/SpectralBarrier.h"
#include <Kismet/GameplayStatics.h>
#include "Components/BoxComponent.h"
#include "Enemy/Enemy.h"

ASpectralBarrier::ASpectralBarrier()
{
	bEnableDestroyOnCollision = false;
}

void ASpectralBarrier::BeginPlay()
{
	Super::BeginPlay();

	BoxCollider->OnComponentBeginOverlap.AddDynamic(this, &ASpectralBarrier::OnOverlap);
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

void ASpectralBarrier::OnOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	AEnemy* EnemyTarget = Cast<AEnemy>(OtherActor);
	if (EnemyTarget)
	{
		EnemyTarget->Disarm();
	}
}
