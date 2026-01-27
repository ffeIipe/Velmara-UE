#include "Components/TargetingComponent.h"

#include "AbilitySystemComponent.h"
#include "Curves/CurveFloat.h"
#include "Entities/Entity.h"
#include "GameFramework/Character.h"
#include "GAS/VelmaraGameplayTags.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"

UTargetingComponent::UTargetingComponent()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.01f;

    CurrentTarget = nullptr;
    CombatTargets.Empty();
}

void UTargetingComponent::BeginPlay()
{
    Super::BeginPlay();
    
    PrimaryComponentTick.bStartWithTickEnabled = false;
    PrimaryComponentTick.TickInterval = 0.01f;
    SetComponentTickEnabled(false);

    OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter)
    {
        OwnerController = Cast<AController>(OwnerCharacter->GetController());
    }
}

void UTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    RotateTowardsTarget(CurrentTarget);
}

void UTargetingComponent::HandleTargetDeath(AEntity* DeadEntity)
{
    if (DeadEntity == CurrentTarget)
    {
        DeadEntity->OnDead.RemoveDynamic(this, &UTargetingComponent::HandleTargetDeath);

        ChangeHardLockTarget();
    }
}

void UTargetingComponent::EnableLock()
{
    CombatTargets = GetTargets(1500.f);
    CurrentTarget = SelectNearestTarget(CombatTargets);

    if (CurrentTarget)
    {
        bIsLocking = true;
        SetComponentTickEnabled(true);
    }
}

void UTargetingComponent::DisableLock()
{
    bIsLocking = false;
    SetComponentTickEnabled(false);
    
    CurrentTarget = nullptr;
    CombatTargets.Empty();
}

void UTargetingComponent::ChangeHardLockTarget()
{
    if (!bIsLocking || CombatTargets.Num() == 0) return;
    
    CombatTargetIndex = (CombatTargetIndex + 1) % CombatTargets.Num();
    CurrentTarget = CombatTargets[CombatTargetIndex];
}

AActor* UTargetingComponent::SelectNearestTarget(TArray<AActor*> Targets)
{
    if (Targets.Num() == 0) return nullptr;
    
    float MinDistance = TNumericLimits<float>::Max(); 
    AActor* ClosestCombatTarget = nullptr;
    
    for (const auto CombatTarget : Targets)
    {
        if (CombatTarget)
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
        
        return ClosestCombatTarget;
    }

    return nullptr;
}

void UTargetingComponent::RotateTowardsTarget(AActor* Target)
{
    if (!OwnerController || !OwnerCharacter || !Target)
    {
        if (bIsLocking)
        {
            DisableLock();
        }
        
        return;
    }

    const FVector StartLocation = OwnerCharacter->GetPawnViewLocation();
    const FVector TargetLocation = CurrentTarget->GetActorLocation() + FVector(0.f, 0.f, 70.f);
    
    const FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, TargetLocation);
    const FRotator CurrentControlRotation = OwnerController->GetControlRotation();
    
    const FRotator NewControlRotation = FMath::RInterpTo(CurrentControlRotation, TargetRotation, GetWorld()->DeltaTimeSeconds, 15.f);
    OwnerController->SetControlRotation(NewControlRotation);

    const FRotator NewActorRotation = FRotator(0.f, TargetRotation.Yaw, 0.f);
    OwnerCharacter->SetActorRotation(NewActorRotation);
}

TArray<AActor*> UTargetingComponent::GetTargets(const float Radius) const
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
        TSubclassOf<AEntity>(),
        ActorsToIgnore,
        Hits
    );

    TArray<AActor*> Actors;
    for (const auto Target : Hits)
    {
        if (AEntity* TargetEntity = Cast<AEntity>(Target))
        {
            if (TargetEntity->IsAlive())
            {
                Actors.Add(TargetEntity);

                TargetEntity->OnDead.AddUniqueDynamic(this, &UTargetingComponent::HandleTargetDeath);
            }
            else if (GEngine)
            {
                //GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, TargetEntity->GetName() + " is dead!");
            }
        }
    }
    return Actors;
}

void UTargetingComponent::RemoveCombatTarget()
{
    if (!bIsLocking)
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