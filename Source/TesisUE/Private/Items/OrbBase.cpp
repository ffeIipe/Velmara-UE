#include "Items/OrbBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameplayEffect.h"
#include "NiagaraComponent.h"
#include "Subsystems/OrbsPoolManager.h"

AOrbBase::AOrbBase()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	Body->SetupAttachment(GetRootComponent());
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCastShadow(false);

	Light = CreateDefaultSubobject<UPointLightComponent>(TEXT("Light"));
	Light->SetupAttachment(GetRootComponent());
	Light->SetCastShadows(false);

	Niagara = CreateDefaultSubobject<UNiagaraComponent>(TEXT("Niagara"));
	Niagara->SetupAttachment(GetRootComponent());
	Niagara->SetAutoActivate(false);
}

void AOrbBase::ActivateOrb_Implementation(const FVector& Location, const FRotator& Rotation)
{
	SetActorLocationAndRotation(Location, Rotation);
	SetActorHiddenInGame(false);

	Age = 0.f;
	Velocity = FVector::ZeroVector;
	Niagara->Activate(true);
}

void AOrbBase::DeactivateOrb_Implementation()
{
	SetActorTickEnabled(false);
	SetActorHiddenInGame(true);
	Niagara->Deactivate();

	TargetActor.Reset();
	OnArrivedNative.Clear();
	OnArrived.Clear();
}

void AOrbBase::LaunchToTarget(AActor* Target)
{
	if (!IsValid(Target))
	{
		ReturnToPool();
		return;
	}

	TargetActor = Target;
	Age = 0.f;
	HoverTime = FMath::FRandRange(HoverTimeMin, FMath::Max(HoverTimeMin, HoverTimeMax));
	ArcSign = FMath::RandBool() ? 1.f : -1.f;

	const FVector Horizontal = FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector();
	Velocity = Horizontal * BurstSpeed + FVector::UpVector * BurstUpSpeed;

	SetActorTickEnabled(true);
}

void AOrbBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	DeltaSeconds = FMath::Min(DeltaSeconds, 1.f / 30.f);

	const AActor* Target = TargetActor.Get();
	Age += DeltaSeconds;

	if (!Target || Age > MaxLifetime)
	{
		ReturnToPool();
		return;
	}

	if (Age < HoverTime)
	{
		Velocity *= FMath::Exp(-BurstDrag * DeltaSeconds);
		Velocity = Velocity.RotateAngleAxis(ArcSign * ArcTurnRate * DeltaSeconds, FVector::UpVector);

		SetActorLocation(GetActorLocation() + Velocity * DeltaSeconds);
		return;
	}

	const FVector ToTarget = Target->GetActorLocation() + TargetOffset - GetActorLocation();
	const float Distance = ToTarget.Size();

	const float Speed = FMath::Min(Velocity.Size() + SeekAcceleration * DeltaSeconds, MaxSpeed);
	const FVector Desired = ToTarget.GetSafeNormal();
	const FVector Current = Velocity.IsNearlyZero() ? Desired : Velocity.GetSafeNormal();

	const float Steer = SteerRate + 2.f * Speed / FMath::Max(Distance, 1.f);
	Velocity = FMath::Lerp(Current, Desired, FMath::Clamp(Steer * DeltaSeconds, 0.f, 1.f)).GetSafeNormal() * Speed;

	const float Step = Speed * DeltaSeconds;
	if (Distance <= ArriveDistance || Distance <= Step)
	{
		SetActorLocation(GetActorLocation() + ToTarget);
		Arrive();
		return;
	}

	SetActorLocation(GetActorLocation() + Velocity * DeltaSeconds);
}

void AOrbBase::Arrive()
{
	AActor* Target = TargetActor.Get();

	ApplyEffect(Target);
	ReceiveArrived(Target);

	FOnOrbArrivedNative NativeCopy = OnArrivedNative;
	OnArrived.Broadcast(Target);
	NativeCopy.Broadcast(Target);

	ReturnToPool();
}

void AOrbBase::ApplyEffect(AActor* Target) const
{
	if (!EffectClass) return;

	UAbilitySystemComponent* ASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Target);
	if (!ASC) return;

	FGameplayEffectContextHandle Context = ASC->MakeEffectContext();
	Context.AddSourceObject(this);

	const FGameplayEffectSpecHandle Spec = ASC->MakeOutgoingSpec(EffectClass, 1.f, Context);
	if (!Spec.IsValid()) return;

	if (MagnitudeTag.IsValid())
	{
		Spec.Data->SetSetByCallerMagnitude(MagnitudeTag, Magnitude);
	}

	ASC->ApplyGameplayEffectSpecToSelf(*Spec.Data.Get());
}

void AOrbBase::ReturnToPool()
{
	if (UOrbsPoolManager* Pool = GetWorld()->GetSubsystem<UOrbsPoolManager>())
	{
		Pool->ReturnOrbToPool(this);
	}
	else
	{
		Destroy();
	}
}
