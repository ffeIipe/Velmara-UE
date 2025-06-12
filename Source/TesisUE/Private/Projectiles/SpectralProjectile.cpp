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

		Target->Execute_GetHit(OtherActor, OtherActor->GetActorLocation(), UDamageType::StaticClass(), Damage);

		ActorsToIgnore.AddUnique(OtherActor);
	}
}

