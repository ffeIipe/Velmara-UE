// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectiles/SpectralProjectile.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Components/TimelineComponent.h"
#include "Player/PlayerMain.h"
#include <Kismet/GameplayStatics.h>
#include "Enemy/Enemy.h"

ASpectralProjectile::ASpectralProjectile()
{
	bEnableDestroyOnCollision = true;
}

void ASpectralProjectile::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(this);

	if (OtherActor == Target && !ActorsToIgnore.Contains(Target))
	{
		UGameplayStatics::ApplyDamage(
			Target,
			Damage,
			GetInstigator()->GetController(),
			this,
			UDamageType::StaticClass()
		);
		ActorsToIgnore.AddUnique(OtherActor);
	}
}

void ASpectralProjectile::BeginPlay()
{
    Super::BeginPlay();
	if(Player)
	{
		Target = Player->GetSpectralTarget();
	}
}

void ASpectralProjectile::OnProjectileImpact(AActor* OtherActor, const FHitResult& Hit)
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

	if (bEnableDestroyOnCollision)
	{
		Destroy();
	}
}


