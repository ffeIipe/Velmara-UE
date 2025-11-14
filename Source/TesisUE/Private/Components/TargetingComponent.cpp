#include "Components/TargetingComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "DataAssets/EntityData.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Interfaces/CameraProvider.h"
#include "Interfaces/CharacterMovementProvider.h"
#include "Interfaces/CombatTargetInterface.h"
#include "Interfaces/ControllerProvider.h"

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

    ControllerProvider = GetOwner();
    CharacterMovementProvider = GetOwner();
    CameraProvider = GetOwner();

    if (SoftLockCurve)
    {
        FOnTimelineFloat ProgressSoftLockFunction;
        ProgressSoftLockFunction.BindUFunction(this, FName("UpdateSoftLockOn"));
        SoftLockTimeline->AddInterpFloat(SoftLockCurve, ProgressSoftLockFunction);
    }
}

void UTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    GEngine->AddOnScreenDebugMessage(INDEX_NONE, -1.f, FColor::Purple, "UTargetingComponent::TickComponent");

    if (bIsHardLocking)
    {
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, -1.f, FColor::Green, "HardLocking true");
    }
    else
    {
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, -1.f, FColor::Red, "HardLocking false");
    }
    
    ValidateCurrentTarget();
    
    if (bIsHardLocking && CurrentTarget.GetInterface() != nullptr)
    {
        RotateTowardsHardLockTarget(CurrentTarget, DeltaTime);
    }
}

void UTargetingComponent::ToggleHardLock()
{
    bIsHardLocking = !bIsHardLocking;
    
    if (bIsHardLocking)
    {
        if (PickHardLockTarget())
        {
            SetComponentTickEnabled(true);
            if (OnHardLockToggled.IsBound()) OnHardLockToggled.Broadcast();
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
        if (OnHardLockToggled.IsBound()) OnHardLockToggled.Broadcast();
    }
}

void UTargetingComponent::ChangeHardLockTarget()
{
    if (!bIsHardLocking || CombatTargets.Num() == 0) return;
    
    CombatTargetIndex = (CombatTargetIndex + 1) % CombatTargets.Num();
    CurrentTarget = CombatTargets[CombatTargetIndex];
}

void UTargetingComponent::PerformSoftLock()
{
    if (bIsHardLocking) return;

    const FVector Start = GetOwner()->GetActorLocation();
    const FVector End = CharacterMovementProvider->Execute_GetCharacter(GetOwner())->GetLastMovementInputVector() * SoftLockDistance + GetOwner()->GetActorLocation();

    CurrentTarget = SearchCombatTarget(Start, End, SoftLockRadius);
    if (CurrentTarget && CurrentTarget->IsAlive())
    {
        SoftLockTimeline->PlayFromStart();
    }
}

bool UTargetingComponent::PickHardLockTarget()
{
    CombatTargets = GetCombatTargets(HardLockRadius);

    float MinDistance = 0.f;
    TScriptInterface<ICombatTargetInterface> ClosestCombatTarget = nullptr;
    
    for (const TScriptInterface CombatTarget : CombatTargets)
    {
        if (CombatTarget && CombatTarget->IsAlive())
        {
            const float NewDistance = (CombatTarget->GetTargetActorLocation() - GetOwner()->GetActorLocation()).Length();

            if (MinDistance == 0.f)
            {
                MinDistance = NewDistance;
                ClosestCombatTarget = CombatTarget;
            }
        
            if (MinDistance > NewDistance)
            {
                MinDistance = NewDistance;
                ClosestCombatTarget = CombatTarget;
            }
        }
        
        if (CombatTargets.Num() > 0)
        {
            CombatTargetIndex = 0;
            CurrentTarget = ClosestCombatTarget;
            return true;
        }
    }
       
    return false;
}

void UTargetingComponent::ValidateCurrentTarget()
{
    if (bIsHardLocking && CurrentTarget && !CurrentTarget->IsAlive())
    {
        if (!PickHardLockTarget())
        {
            ToggleHardLock();
        }
    }
}

void UTargetingComponent::RotateTowardsHardLockTarget(const TScriptInterface<ICombatTargetInterface>& Target, const float DeltaTime) const
{
    if (!ControllerProvider || !CameraProvider || !Target) return;

    AController* OwnerController = ControllerProvider->GetEntityController();
    if (!OwnerController) return;
    
    const FVector StartLocation = CameraProvider->GetCameraLocation();
    const FVector TargetLocation = Target->GetTargetActorLocation() + FVector(0.f, 0.f, 70.f);
    
    FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, TargetLocation);
    
    TargetRotation.Pitch = FMath::Clamp(TargetRotation.Pitch, -45.0f, 45.0f);
    
    const FRotator CurrentControlRotation = OwnerController->GetControlRotation();
    const FRotator NewControlRotation = UKismetMathLibrary::RInterpTo(CurrentControlRotation, TargetRotation, DeltaTime, 0.f);

    OwnerController->SetControlRotation(NewControlRotation);

    FRotator OwnerActorRotation = GetOwner()->GetActorRotation();
    FRotator NewActorYawRotation = FRotator(OwnerActorRotation.Pitch, TargetRotation.Yaw, OwnerActorRotation.Roll);

    GetOwner()->SetActorRotation(NewActorYawRotation);

    DrawDebugBox(GetWorld(), TargetLocation, FVector(32.f, 32.f, 32.f), FColor::Red);
}

TArray<TScriptInterface<ICombatTargetInterface>> UTargetingComponent::GetCombatTargets(const float Radius) const
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

    TArray<TScriptInterface<ICombatTargetInterface>> FinalCombatTargets;
    for (AActor* Target : Hits)
    {
        if (TScriptInterface<ICombatTargetInterface> CombatTargetFound = Target; CombatTargetFound && CombatTargetFound->IsAlive())
        {
            FinalCombatTargets.Add(CombatTargetFound);
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

TScriptInterface<ICombatTargetInterface> UTargetingComponent::SearchCombatTarget(const FVector& Start, const FVector& End, const float SearchRadius) const
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
        EDrawDebugTrace::None,
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
    const FVector End = CurrentTarget->GetTargetActorLocation();

    FRotator NewRotation = FRotator(
        GetOwner()->GetActorRotation().Pitch,
        UKismetMathLibrary::FindLookAtRotation(Start, End).Yaw,
        UKismetMathLibrary::FindLookAtRotation(Start, End).Roll
    );

    NewRotation = FMath::Lerp(GetOwner()->GetActorRotation(), NewRotation, Alpha);

    GetOwner()->SetActorRotation(NewRotation);
}