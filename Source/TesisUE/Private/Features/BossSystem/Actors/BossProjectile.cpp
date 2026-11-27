#include "Features/BossSystem/Actors/BossProjectile.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/SphereComponent.h"
#include "EngineUtils.h"
#include "Entities/Entity.h"
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
	Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_PhysicsBody, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_Destructible, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_GameTraceChannel1, ECR_Block);
	Collision->SetCollisionResponseToChannel(ECC_GameTraceChannel4, ECR_Block);
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

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> TrailFinder(TEXT("/Game/SlashTrail_SoftTofu/Niagara/Dark/NS_SlashTrail_Dark_Loop.NS_SlashTrail_Dark_Loop"));
	if (TrailFinder.Succeeded())
	{
		Effect->SetAsset(TrailFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> ImpactFinder(TEXT("/Game/VFX/Boss/NS_Hit_Solar_Once.NS_Hit_Solar_Once"));
	if (ImpactFinder.Succeeded())
	{
		ImpactEffect = ImpactFinder.Object;
	}
}

void ABossProjectile::BeginPlay()
{
	Super::BeginPlay();

	DefaultPierceTargets = bPierceTargets;
	DefaultCollisionRadius = Collision->GetUnscaledSphereRadius();

	Collision->OnComponentBeginOverlap.AddUniqueDynamic(this, &ABossProjectile::HandleOverlap);
	Movement->OnProjectileStop.AddUniqueDynamic(this, &ABossProjectile::HandleStop);
}

void ABossProjectile::Launch(AActor* InInstigator, const FVector& Direction, const float Speed, const float Lifetime, const FBossDamageParams& InDamageParams)
{
	ClearHomingTarget();

	BossInstigator = InInstigator;
	DamageParams = InDamageParams;
	HandledActors.Reset();
	SetInstigator(Cast<APawn>(InInstigator));

	const FVector LaunchDirection = Direction.GetSafeNormal();
	if (LaunchDirection.IsNearlyZero())
	{
		Expire();
		return;
	}

	SetActorRotation(LaunchDirection.Rotation());

	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ConfigureMoveIgnores(InInstigator);

	Movement->SetUpdatedComponent(Collision);
	Movement->InitialSpeed = Speed;
	Movement->MaxSpeed = Speed;
	Movement->Velocity = LaunchDirection * Speed;
	Movement->Activate(true);
	Movement->UpdateComponentVelocity();

	Effect->Activate(true);

	GetWorldTimerManager().SetTimer(LifetimeTimer, this, &ABossProjectile::Expire, FMath::Max(Lifetime, 0.1f), false);

	bLaunched = true;

	TArray<AActor*> AlreadyOverlapping;
	Collision->GetOverlappingActors(AlreadyOverlapping);
	for (AActor* Other : AlreadyOverlapping)
	{
		HandleOverlap(Collision, Other, nullptr, 0, false, FHitResult());
		if (!bLaunched) break;
	}

	OnLaunched();
}

void ABossProjectile::SetHomingTarget(AActor* TargetActor, const float HomingAcceleration)
{
	ClearHomingTarget();

	if (!bLaunched || !IsValid(TargetActor) || HomingAcceleration <= 0.f) return;

	USceneComponent* TargetComponent = TargetActor->GetRootComponent();
	if (!TargetComponent) return;

	if (AEntity* TargetEntity = Cast<AEntity>(TargetActor))
	{
		if (!TargetEntity->IsAlive()) return;
		HomingEntity = TargetEntity;
		TargetEntity->OnDead.AddUniqueDynamic(this, &ABossProjectile::HandleHomingTargetDead);
	}

	Movement->HomingTargetComponent = TargetComponent;
	Movement->HomingAccelerationMagnitude = HomingAcceleration;
	Movement->bIsHomingProjectile = true;
}

void ABossProjectile::ClearHomingTarget()
{
	if (AEntity* TargetEntity = HomingEntity.Get())
	{
		TargetEntity->OnDead.RemoveDynamic(this, &ABossProjectile::HandleHomingTargetDead);
	}
	HomingEntity.Reset();

	if (Movement)
	{
		Movement->bIsHomingProjectile = false;
		Movement->HomingTargetComponent = nullptr;
		Movement->HomingAccelerationMagnitude = 0.f;
	}
}

void ABossProjectile::SetPierceTargets(const bool bShouldPierce)
{
	bPierceTargets = bShouldPierce;
}

void ABossProjectile::SetCollisionRadius(const float Radius)
{
	if (Collision && Radius > 0.f)
	{
		Collision->SetSphereRadius(Radius, true);
	}
}

void ABossProjectile::HandleOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex, const bool bFromSweep, const FHitResult& SweepResult)
{
	if (!bLaunched || !OtherActor || OtherActor == this) return;

	AActor* BossActor = BossInstigator.Get();
	if (OtherActor == BossActor || HandledActors.Contains(OtherActor)) return;

	if (!UBossSystemLibrary::IsValidBossTarget(BossActor, OtherActor, DamageParams.bOnlyPlayerControlled)) return;

	HandledActors.Add(OtherActor);

	if (UBossSystemLibrary::IsImmuneToBossHit(OtherActor, DamageParams.ImmunityTags)) return;

	const FHitResult Hit = bFromSweep ? SweepResult : FHitResult(OtherActor, OtherComp, GetActorLocation(), -GetActorForwardVector());

	UBossSystemLibrary::ApplyBossDamage(BossActor, this, OtherActor, DamageParams, 1.f, Hit);

	PlayImpactFeedback(GetActorLocation());
	OnImpact(Hit, true);

	if (!bPierceTargets)
	{
		bLaunched = false;
		ClearHomingTarget();
		Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Movement->StopMovementImmediately();
		GetWorldTimerManager().SetTimerForNextTick(this, &ABossProjectile::Expire);
	}
}

void ABossProjectile::HandleStop(const FHitResult& ImpactResult)
{
	if (!bLaunched) return;

	bLaunched = false;
	ClearHomingTarget();
	Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Movement->StopMovementImmediately();

	PlayImpactFeedback(ImpactResult.bBlockingHit ? FVector(ImpactResult.ImpactPoint) : GetActorLocation());
	OnImpact(ImpactResult, false);

	GetWorldTimerManager().SetTimerForNextTick(this, &ABossProjectile::Expire);
}

void ABossProjectile::HandleHomingTargetDead(AEntity* DeadEntity)
{
	ClearHomingTarget();
}

void ABossProjectile::ConfigureMoveIgnores(AActor* InInstigator)
{
	Collision->ClearMoveIgnoreActors();

	if (InInstigator)
	{
		Collision->IgnoreActorWhenMoving(InInstigator, true);

		TArray<AActor*> AttachedActors;
		InInstigator->GetAttachedActors(AttachedActors, true, true);
		for (AActor* Attached : AttachedActors)
		{
			Collision->IgnoreActorWhenMoving(Attached, true);
		}
	}

	if (UWorld* World = GetWorld())
	{
		for (TActorIterator<ABossProjectile> It(World); It; ++It)
		{
			ABossProjectile* OtherProjectile = *It;
			if (OtherProjectile && OtherProjectile != this)
			{
				Collision->IgnoreActorWhenMoving(OtherProjectile, true);
				if (OtherProjectile->Collision)
				{
					OtherProjectile->Collision->IgnoreActorWhenMoving(this, true);
				}
			}
		}
	}
}

void ABossProjectile::Expire()
{
	bLaunched = false;
	ClearHomingTarget();
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
	Collision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
}

void ABossProjectile::OnPoolDeactivate_Implementation()
{
	bLaunched = false;
	ClearHomingTarget();
	GetWorldTimerManager().ClearTimer(LifetimeTimer);

	Collision->ClearMoveIgnoreActors();
	bPierceTargets = DefaultPierceTargets;
	if (DefaultCollisionRadius > 0.f)
	{
		Collision->SetSphereRadius(DefaultCollisionRadius, false);
	}

	Movement->StopMovementImmediately();
	Movement->Deactivate();

	Effect->DeactivateImmediate();
}
