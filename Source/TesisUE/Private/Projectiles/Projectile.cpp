// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectiles/Projectile.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Player/PlayerMain.h"
#include "Enemy/Enemy.h"
#include <Kismet/GameplayStatics.h>

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	BoxCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollider"));
	BoxCollider->SetupAttachment(GetRootComponent());
	

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(BoxCollider);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	Player = Cast<APlayerMain>(GetWorld()->GetFirstPlayerController()->GetPawn());

	BoxCollider->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnBoxOverlap);
}

void AProjectile::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	//FString String = OtherActor->GetDebugName(OtherActor) + " " + OtherComp->GetName();
	//GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Black, String);

	if (Cast<AEnemy>(OtherActor))
	{
		OnProjectileImpact(OtherActor, SweepResult);
	}
	else return;
}

void AProjectile::OnProjectileImpact(AActor* OtherActor, const FHitResult& Hit)
{
	//TODO: Logic of destruction, VFX, SFX, etc.

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

	Destroy();
}
