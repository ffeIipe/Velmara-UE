#include "Components/CombatComponent.h"

#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "DataAssets/EntityData.h"
#include "Entities/Entity.h"
#include "GameFramework/Character.h"

#include "Interfaces/Weapon/WeaponInterface.h"

void UCombatComponent::HandleWeaponChanged(TScriptInterface<IWeaponInterface> NewWeapon)
{
	if (NewWeapon)
	{
		NewWeapon->OnWeaponUsed.AddUniqueDynamic(this, &UCombatComponent::StartAttackBufferEvent);
	}
}

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BufferAttackTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BufferAttackTimeline"));
	LaunchCharacterTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("LaunchCharacterTimeline"));
	BufferBackwardsTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BufferBackwardsTimeline"));
}

void UCombatComponent::InitializeValues(const FCombatData& CombatData)
{
	UE_LOG(LogTemp, Warning, TEXT("Combat Component values assigned by CombatData"));

	BufferAttackDistance = CombatData.BufferAttackDistance;
	LaunchCharacterUpCurve = CombatData.LaunchUpCurve;
	BufferBackwardsCurve = CombatData.BufferBackwardsCurve;
	BufferCurve = CombatData.BufferCurve;
}


void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetComponentTickEnabled(false);
	
	OwnerCharacter = Cast<ACharacter>(GetOwner());
	
	if (BufferCurve)
	{
		FOnTimelineFloat ProgressAttackFunction;
		ProgressAttackFunction.BindUFunction(this, FName("UpdateAttackBuffer"));
		BufferAttackTimeline->AddInterpFloat(BufferCurve, ProgressAttackFunction);
	}

	if (LaunchCharacterUpCurve)
	{
		FOnTimelineFloat ProgressLaunchUpFunction;
		ProgressLaunchUpFunction.BindUFunction(this, FName("UpdateLaunchCharacterUp"));
		LaunchCharacterTimeline->AddInterpFloat(LaunchCharacterUpCurve, ProgressLaunchUpFunction);

		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Magenta, GetOwner()->GetName() + " using curve: " + LaunchCharacterUpCurve->GetName());
	}
		
	if (BufferBackwardsCurve)
	{
		FOnTimelineFloat ProgressBufferBackwardsFunction;
		ProgressBufferBackwardsFunction.BindUFunction(this, FName("UpdateBufferBackwards"));
		BufferBackwardsTimeline->AddInterpFloat(BufferBackwardsCurve, ProgressBufferBackwardsFunction);
	}
}

void UCombatComponent::StartAttackBufferEvent()
{
	StopAttackBufferEvent();
	if (BufferAttackTimeline)
	{
		BufferAttackTimeline->PlayFromStart();
	}
}

void UCombatComponent::StopAttackBufferEvent()
{
	if (BufferAttackTimeline)
	{
		BufferAttackTimeline->Stop();
	}
}

void UCombatComponent::StartBufferBackwards()
{
	if (BufferBackwardsTimeline)
	{
		StopAttackBufferEvent();
		BufferBackwardsTimeline->PlayFromStart();
	}
}

void UCombatComponent::StopBufferBackwards()
{
	if (BufferBackwardsTimeline)
	{
		BufferBackwardsTimeline->Stop();
	}
}

void UCombatComponent::UpdateBufferBackwards(const float Alpha)
{
	UpdateBuffer(Alpha, -BufferAttackDistance);
}

void UCombatComponent::UpdateAttackBuffer(const float Alpha) const
{
	UpdateBuffer(Alpha, BufferAttackDistance * BufferMultiplier);
}

void UCombatComponent::UpdateBuffer(const float Alpha, const float BufferDistance) const
{
	const FVector CurrentLocation = GetOwner()->GetActorLocation();
	const FVector ForwardVector = GetOwner()->GetActorForwardVector();

	const FVector TargetLocation = FMath::Lerp(CurrentLocation, CurrentLocation + (ForwardVector * BufferDistance), Alpha);

	GetOwner()->SetActorLocation(TargetLocation, false);
}

void UCombatComponent::StartLaunchingUp()
{
	CurrentLocationLaunch = GetOwner()->GetActorLocation();
	UpVectorLaunch = GetOwner()->GetActorUpVector();

	LaunchCharacterTimeline->PlayFromStart();
}

bool UCombatComponent::CheckDistance(const AEntity* TargetToCheck, const float DistanceToCheck) const
{
	if (!TargetToCheck) return false;
	
	const float Distance = FVector::Distance(TargetToCheck->GetActorLocation(), GetOwner()->GetActorLocation());
	
	if (Distance < DistanceToCheck)
		return true;
	
	return false;
}
