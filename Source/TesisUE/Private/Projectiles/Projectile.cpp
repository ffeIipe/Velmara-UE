// Fill out your copyright notice in the Description page of Project Settings.


#include "Projectiles/Projectile.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"

AProjectile::AProjectile()
{
	PrimaryActorTick.bCanEverTick = true;

	BoxCollider = CreateDefaultSubobject<UBoxComponent>(TEXT("BoxCollider"));
	BoxCollider->SetupAttachment(GetRootComponent());
	//BoxCollider->SetGenerateOverlapEvents(true);
	//SetActorEnableCollision(true);
	//BoxCollider->SetCollisionObjectType(ECC_PhysicsBody);
	//BoxCollider->SetCollisionResponseToChannel(ECC_Visibility, ECR_Ignore);

	ProjectileMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ProjectileMesh"));
	ProjectileMesh->SetupAttachment(BoxCollider);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->InitialSpeed = Speed;
	ProjectileMovementComponent->MaxSpeed = Speed;
	ProjectileMovementComponent->ProjectileGravityScale = Gravity;
}

void AProjectile::BeginPlay()
{
	Super::BeginPlay();
	
	BoxCollider->OnComponentBeginOverlap.AddDynamic(this, &AProjectile::OnBoxOverlap);
}

void AProjectile::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	OnProjectileImpact(OtherActor, SweepResult);
}

void AProjectile::OnProjectileImpact(AActor* OtherActor, const FHitResult& Hit)
{
	//TODO: Logic of destruction, VFX, SFX, etc.
	Destroy();
}
