#include "Components/TargetingComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "DataAssets/EntityData.h"
#include "Entities/Entity.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Interfaces/Weapon/WeaponInterface.h"

UTargetingComponent::UTargetingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.01f;
    SoftLockTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("SoftLockTimeline"));
}

void UTargetingComponent::InitializeValues(const FTargetingData& TargetingData)
{
    SoftLockCurve = TargetingData.SoftLockCurve;
}

void UTargetingComponent::BeginPlay()
{
    Super::BeginPlay();
    PrimaryComponentTick.bStartWithTickEnabled = false;
    SetComponentTickEnabled(false);
    PrimaryComponentTick.TickInterval = 0.01f;

    OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter)
    {
        OwnerController = Cast<AController>(OwnerCharacter->GetController());
    }

    if (SoftLockCurve)
    {
        FOnTimelineFloat ProgressSoftLockFunction;
        ProgressSoftLockFunction.BindUFunction(this, FName("UpdateSoftLockOn"));
        SoftLockTimeline->AddInterpFloat(SoftLockCurve, ProgressSoftLockFunction);
    }
}

void UTargetingComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (bIsHardLocking && !CurrentTarget)
    {
        RotateTowardsHardLockTarget(CurrentTarget, DeltaTime);
    }
}

void UTargetingComponent::HandleTargetDeath(AEntity* DeadEntity)
{
    if (DeadEntity == CurrentTarget)
    {
        DeadEntity->OnDead.RemoveDynamic(this, &UTargetingComponent::HandleTargetDeath);
        
        if (!PickHardLockTarget())
        {
            ToggleHardLock();
        }
    }
}

void UTargetingComponent::HandleWeaponChanged(TScriptInterface<IWeaponInterface> NewWeapon)
{
}

void UTargetingComponent::ToggleHardLock()
{
    bIsHardLocking = !bIsHardLocking;
    
    if (bIsHardLocking)
    {
        if (PickHardLockTarget())
        {
            SetComponentTickEnabled(true);
            if (OnHardLockOn.IsBound()) OnHardLockOn.Broadcast();
        }
        else
        {
            bIsHardLocking = false;
        }
    }
    else
    {
        CurrentTarget = nullptr;
        CombatTargets.Empty();
        SetComponentTickEnabled(false);
        if (OnHardLockOff.IsBound()) OnHardLockOff.Broadcast();
    }
}

void UTargetingComponent::ChangeHardLockTarget()
{
    if (!bIsHardLocking || CombatTargets.Num() == 0) return;
    
    CombatTargetIndex = (CombatTargetIndex + 1) % CombatTargets.Num();
    CurrentTarget = CombatTargets[CombatTargetIndex];
}

AActor* UTargetingComponent::PerformSoftLock(const float Distance, const float Radius) const
{
    const FVector Start = GetOwner()->GetActorLocation();
    const FVector End = OwnerCharacter->GetLastMovementInputVector() * Distance + GetOwner()->GetActorLocation();

    return SearchCombatTarget(Start, End, Radius);
}

bool UTargetingComponent::PickHardLockTarget()
{
    CombatTargets = GetCombatTargets(HardLockRadius);

    float MinDistance = TNumericLimits<float>::Max(); 
    AEntity* ClosestCombatTarget = nullptr;
    
    for (const auto CombatTarget : CombatTargets)
    {
        if (CombatTarget && CombatTarget->IsAlive())
        {
            if (const float NewDistance = FVector::Dist(CombatTarget->GetActorLocation(), GetOwner()->GetActorLocation()); NewDistance < MinDistance)
            {
                MinDistance = NewDistance;
                ClosestCombatTarget = CombatTarget;
            }
        }
    }

    if (ClosestCombatTarget) 
    {
        if (AEntity* OldTarget = Cast<AEntity>(CurrentTarget))
        {
            OldTarget->OnDead.RemoveDynamic(this, &UTargetingComponent::HandleTargetDeath);
        }

        CurrentTarget = ClosestCombatTarget;
        
        if (AEntity* NewTarget = Cast<AEntity>(CurrentTarget))
        {
            NewTarget->OnDead.AddUniqueDynamic(this, &UTargetingComponent::HandleTargetDeath);
        }
        
        return true;
    }
    return false;
}

void UTargetingComponent::RotateTowardsHardLockTarget(const AActor* Target, const float DeltaTime) const
{
    if (!OwnerController || !OwnerCharacter || !Target) return;

    const FVector StartLocation = OwnerCharacter->GetPawnViewLocation();
    const FVector TargetLocation = Target->GetActorLocation() + FVector(0.f, 0.f, 70.f);
    
    const FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, TargetLocation);
    const FRotator CurrentControlRotation = OwnerController->GetControlRotation();
    
    const FRotator NewControlRotation = FMath::RInterpTo(CurrentControlRotation, TargetRotation, DeltaTime, 15.f);
    OwnerController->SetControlRotation(NewControlRotation);

    const FRotator NewActorRotation = FRotator(0.f, TargetRotation.Yaw, 0.f);
    OwnerCharacter->SetActorRotation(NewActorRotation);
}

TArray<AEntity*> UTargetingComponent::GetCombatTargets(const float Radius) const
{
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(GetOwner());

    TArray<AActor*> Hits;
    UKismetSystemLibrary::SphereOverlapActors(
        GetWorld(),
        GetOwner()->GetActorLocation(),
        Radius,
        ObjectTypes,
        nullptr,
        ActorsToIgnore,
        Hits
    );

    TArray<AEntity*> FinalCombatTargets;
    for (const auto Target : Hits)
    {
        if (AEntity* Entity = Cast<AEntity>(Target); Entity->IsAlive())
        {
            FinalCombatTargets.Add(Entity);
        }
    }
    return FinalCombatTargets;
}

void UTargetingComponent::RemoveCombatTarget()
{
    if (!bIsHardLocking)
    {
        CurrentTarget = nullptr;
    }
}

AActor* UTargetingComponent::SearchCombatTarget(
    const FVector& Start, const FVector& End, const float SearchRadius) const
{
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(GetOwner());

    FHitResult ResultHit;
    const bool bHit = UKismetSystemLibrary::SphereTraceSingleForObjects(
        GetWorld(),
        Start,
        End,
        SearchRadius,
        ObjectTypes,
        false,
        ActorsToIgnore,
        EDrawDebugTrace::ForDuration,
        ResultHit,
        true
    );

    if (bHit)
    {
        return ResultHit.GetActor();
    }
    return nullptr;
}

void UTargetingComponent::UpdateSoftLockOn(float Alpha)
{
    const FVector Start = GetOwner()->GetActorLocation();

    if (!CurrentTarget) return;
    const FVector End = CurrentTarget->GetActorLocation();

    FRotator NewRotation = FRotator(
        GetOwner()->GetActorRotation().Pitch,
        UKismetMathLibrary::FindLookAtRotation(Start, End).Yaw,
        UKismetMathLibrary::FindLookAtRotation(Start, End).Roll
    );

    NewRotation = FMath::Lerp(GetOwner()->GetActorRotation(), NewRotation, Alpha);

    GetOwner()->SetActorRotation(NewRotation);
}