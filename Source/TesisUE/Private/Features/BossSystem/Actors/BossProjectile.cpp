#include "Features/BossSystem/Actors/BossProjectile.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/SphereComponent.h"
#include "Features/BossSystem/Core/BossSystemLibrary.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "GenericPool/Core/ActorPoolSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

ABossProjectile::ABossProjectile()
{
	PrimaryActorTick.bCanEverTick = false;

	Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
	Collision->InitSphereRadius(55.f);
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	Collision->SetCollisionObjectType(ECC_WorldDynamic);
	Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
	Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
	Collision->SetGenerateOverlapEvents(true);
	Collision->CanCharacterStepUpOn = ECB_No;
	RootComponent = Collision;

	Effect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Effect"));
	Effect->SetupAttachment(Collision);
	Effect->SetAutoActivate(false);

	Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
	Movement->SetUpdatedComponent(Collision);
	Movement->ProjectileGravityScale = 0.f;
	Movement->InitialSpeed = 0.f;
	Movement->bRotationFollowsVelocity = true;
	Movement->bAutoActivate = false;

	// Valores por defecto con assets del proyecto; se pueden cambiar en un Blueprint hijo.
	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> TrailFinder(TEXT("/Game/SlashTrail_SoftTofu/Niagara/Dark/NS_SlashTrail_Dark_Loop.NS_SlashTrail_Dark_Loop"));
	if (TrailFinder.Succeeded())
	{
		Effect->SetAsset(TrailFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ImpactFinder(TEXT("/Game/SlashTrail_SoftTofu/Niagara/Dark/NS_Hit_Dark_Once.NS_Hit_Dark_Once"));
	if (ImpactFinder.Succeeded())
	{
		ImpactEffect = ImpactFinder.Object;
	}
}

void ABossProjectile::BeginPlay()
{
	Super::BeginPlay();

	Collision->OnComponentBeginOverlap.AddUniqueDynamic(this, &ABossProjectile::HandleOverlap);
	Movement->OnProjectileStop.AddUniqueDynamic(this, &ABossProjectile::HandleStop);
}

void ABossProjectile::Launch(AActor* InInstigator, const FVector& Direction, const float Speed, const float Lifetime, const FBossDamageParams& InDamageParams)
{
	BossInstigator = InInstigator;
	DamageParams = InDamageParams;
	HandledActors.Reset();
	SetInstigator(Cast<APawn>(InInstigator));

	const FVector LaunchDirection = Direction.GetSafeNormal();
	SetActorRotation(LaunchDirection.Rotation());

	// Al detenerse, el ProjectileMovement suelta su UpdatedComponent: se reasigna en cada lanzamiento.
	Movement->SetUpdatedComponent(Collision);
	Movement->InitialSpeed = Speed;
	Movement->MaxSpeed = Speed;
	Movement->Velocity = LaunchDirection * Speed;
	Movement->Activate(true);
	Movement->UpdateComponentVelocity();

	Effect->Activate(true);

	GetWorldTimerManager().SetTimer(LifetimeTimer, this, &ABossProjectile::Expire, FMath::Max(Lifetime, 0.1f), false);

	bLaunched = true;

	// Lo que ya estaba dentro de la esfera al salir del pool no dispara un BeginOverlap nuevo.
	TArray<AActor*> AlreadyOverlapping;
	Collision->GetOverlappingActors(AlreadyOverlapping);
	for (AActor* Other : AlreadyOverlapping)
	{
		HandleOverlap(Collision, Other, nullptr, 0, false, FHitResult());
		if (!bLaunched) break;
	}

	OnLaunched();
}

void ABossProjectile::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bLaunched || !OtherActor || OtherActor == this) return;

	AActor* BossActor = BossInstigator.Get();
	if (OtherActor == BossActor || HandledActors.Contains(OtherActor)) return;

	if (!UBossSystemLibrary::IsValidBossTarget(BossActor, OtherActor, DamageParams.bOnlyPlayerControlled)) return;

	HandledActors.Add(OtherActor);

	// Con el dodge el proyectil lo atraviesa sin dañarlo.
	if (UBossSystemLibrary::IsImmuneToBossHit(OtherActor, DamageParams.ImmunityTags)) return;

	const FHitResult Hit = bFromSweep ? SweepResult : FHitResult(OtherActor, OtherComp, GetActorLocation(), -GetActorForwardVector());

	UBossSystemLibrary::ApplyBossDamage(BossActor, this, OtherActor, DamageParams, 1.f, Hit);

	PlayImpactFeedback(GetActorLocation());
	OnImpact(Hit, true);

	if (!bPierceTargets)
	{
		// Se devuelve al pool en el próximo tick: no conviene apagar la colisión dentro del propio evento de overlap.
		bLaunched = false;
		Movement->StopMovementImmediately();
		GetWorldTimerManager().SetTimerForNextTick(this, &ABossProjectile::Expire);
	}
}

void ABossProjectile::HandleStop(const FHitResult& ImpactResult)
{
	if (!bLaunched) return;

	bLaunched = false;

	PlayImpactFeedback(ImpactResult.bBlockingHit ? FVector(ImpactResult.ImpactPoint) : GetActorLocation());
	OnImpact(ImpactResult, false);

	GetWorldTimerManager().SetTimerForNextTick(this, &ABossProjectile::Expire);
}

void ABossProjectile::Expire()
{
	bLaunched = false;
	GetWorldTimerManager().ClearTimer(LifetimeTimer);

	if (UActorPoolSubsystem* Pool = GetWorld() ? GetWorld()->GetSubsystem<UActorPoolSubsystem>() : nullptr)
	{
		Pool->ReturnToPool(this);
	}
	else
	{
		Destroy();
	}
}

void ABossProjectile::PlayImpactFeedback(const FVector& Location) const
{
	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, Location, GetActorRotation());
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Location);
	}
}

void ABossProjectile::OnPoolActivate_Implementation()
{
	HandledActors.Reset();
}

void ABossProjectile::OnPoolDeactivate_Implementation()
{
	bLaunched = false;
	GetWorldTimerManager().ClearTimer(LifetimeTimer);

	Movement->StopMovementImmediately();
	Movement->Deactivate();

	// Inmediato: si no, el trail se estira desde el último impacto hasta el próximo spawn.
	Effect->DeactivateImmediate();
}
