#include "Components/TargetingComponent.h"
#include "Curves/CurveFloat.h"
#include "Entities/Entity.h"
#include "Features/TotemSystem/Core/TotemChargeComponent.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetMathLibrary.h"
#include "Kismet/KismetSystemLibrary.h"
#include "DrawDebugHelpers.h"

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
    SetComponentTickEnabled(false);

    OwnerCharacter = Cast<ACharacter>(GetOwner());
    if (OwnerCharacter)
    {
        OwnerController = OwnerCharacter->GetController();
        OwnerCharacter->ReceiveControllerChangedDelegate.AddDynamic(this, &UTargetingComponent::HandleOwnerControllerChanged);
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("[UTargetingComponent] BeginPlay: Owner no es un ACharacter."));
    }
}

void UTargetingComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    if (OwnerCharacter)
    {
        OwnerCharacter->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UTargetingComponent::HandleOwnerControllerChanged);
    }

    Super::EndPlay(EndPlayReason);
}

void UTargetingComponent::RefreshOwnerController()
{
    if (!OwnerCharacter)
    {
        OwnerCharacter = Cast<ACharacter>(GetOwner());
    }
    OwnerController = OwnerCharacter ? OwnerCharacter->GetController() : nullptr;
}

void UTargetingComponent::HandleOwnerControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
    OwnerController = NewController;
    if (bIsLocking && (!NewController || !NewController->IsPlayerController()))
    {
        DisableLock();
    }
}

void UTargetingComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    
    if (!CurrentTarget || !OwnerCharacter)
    {
        DisableLock();
        return;
    }

    const float DistSq = FVector::DistSquared(GetOwner()->GetActorLocation(), CurrentTarget->GetActorLocation());
    
    if (DistSq > FMath::Square(TargetingLostRadius))
    {
        UE_LOG(LogTemp, Warning, TEXT("[UTargetingComponent] Target perdido por distancia. Distancia actual: %f, Radio máximo: %f"), FMath::Sqrt(DistSq), TargetingLostRadius);
        DisableLock();
    }
    else
    {
        RotateTowardsTarget(CurrentTarget);
    }
}

void UTargetingComponent::HandleTargetDeath(AEntity* DeadEntity)
{
    if (DeadEntity == CurrentTarget)
    {
        UE_LOG(LogTemp, Log, TEXT("[UTargetingComponent] El target actual (%s) ha muerto. Cambiando target."), *DeadEntity->GetName());
        DeadEntity->OnDead.RemoveDynamic(this, &UTargetingComponent::HandleTargetDeath);

        if (OnTargetedEntityDead.IsBound()) OnTargetedEntityDead.Broadcast(DeadEntity);

        ChangeHardLockTarget();
    }
}

void UTargetingComponent::EnableLock(const float LockRange)
{
    UE_LOG(LogTemp, Log, TEXT("[UTargetingComponent] EnableLock llamado con rango: %f"), LockRange);
    RefreshOwnerController();
    ActiveLockRange = LockRange;
    CombatTargets = GetTargets(ActiveLockRange);
    CurrentTarget = SelectNearestTarget(CombatTargets);

    if (CurrentTarget)
    {
        UE_LOG(LogTemp, Log, TEXT("[UTargetingComponent] Target fijado: %s"), *CurrentTarget->GetName());
        bIsLocking = true;
        SetComponentTickEnabled(true);

        if (UWorld* World = GetWorld())
        {
            World->GetTimerManager().SetTimer(
                TargetRefreshTimerHandle, 
                this, 
                &UTargetingComponent::RefreshTargets, 
                0.5f, 
                true
            );
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("[UTargetingComponent] EnableLock: No se encontró ningún target válido en el rango."));
    }
}

void UTargetingComponent::DisableLock()
{
    UE_LOG(LogTemp, Log, TEXT("[UTargetingComponent] DisableLock llamado."));
    bIsLocking = false;
    SetComponentTickEnabled(false);
    
    if (UWorld* World = GetWorld())
    {
        World->GetTimerManager().ClearTimer(TargetRefreshTimerHandle);
    }
    
    if (AEntity* OldTarget = Cast<AEntity>(CurrentTarget))
    {
        OldTarget->OnDead.RemoveDynamic(this, &UTargetingComponent::HandleTargetDeath);
    }

    CurrentTarget = nullptr;
    CombatTargets.Empty();
}

void UTargetingComponent::RefreshTargets()
{
    if (!bIsLocking) return;

    CombatTargets = GetTargets(ActiveLockRange);

    if (CombatTargets.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("[UTargetingComponent] RefreshTargets: Ya no hay targets en combate. Desactivando Lock."));
        DisableLock();
        return;
    }

    if (CurrentTarget && !CombatTargets.Contains(CurrentTarget))
    {
        UE_LOG(LogTemp, Log, TEXT("[UTargetingComponent] RefreshTargets: El target actual ya no es válido/está fuera de rango. Seleccionando uno nuevo."));
        CurrentTarget = SelectNearestTarget(CombatTargets);
    }
}

void UTargetingComponent::ChangeHardLockTarget()
{
    if (!bIsLocking || CombatTargets.Num() == 0) return;
    
    int32 Attempts = 0;
    AActor* NewCandidate = nullptr;

    while (Attempts < CombatTargets.Num())
    {
        CombatTargetIndex = (CombatTargetIndex + 1) % CombatTargets.Num();

        if (AActor* Candidate = CombatTargets[CombatTargetIndex]; Candidate && Candidate != CurrentTarget)
        {
            NewCandidate = Candidate;
            break;
        }
        Attempts++;
    }

    if (NewCandidate)
    {
        if (AEntity* OldTarget = Cast<AEntity>(CurrentTarget))
        {
            OldTarget->OnDead.RemoveDynamic(this, &UTargetingComponent::HandleTargetDeath);
        }

        CurrentTarget = NewCandidate;
        UE_LOG(LogTemp, Log, TEXT("[UTargetingComponent] ChangeHardLockTarget: Cambiado a %s"), *CurrentTarget->GetName());

        if (AEntity* NewTarget = Cast<AEntity>(CurrentTarget))
        {
            NewTarget->OnDead.AddUniqueDynamic(this, &UTargetingComponent::HandleTargetDeath);
        }

        if (OnTargetedEntityChanged.IsBound()) OnTargetedEntityChanged.Broadcast(Cast<AEntity>(NewCandidate));
    }
}

AActor* UTargetingComponent::SelectNearestTarget(TArray<AActor*> Targets)
{
    if (Targets.Num() == 0) return nullptr;
    
    float MinDistance = TNumericLimits<float>::Max(); 
    AActor* ClosestCombatTarget = nullptr;
    
    const FVector OwnerLocation = GetOwner()->GetActorLocation();

    for (const auto CombatTarget : Targets)
    {
        if (CombatTarget)
        {
            if (const float DistSq = FVector::DistSquared(CombatTarget->GetActorLocation(), OwnerLocation); DistSq < MinDistance)
            {
                MinDistance = DistSq;
                ClosestCombatTarget = CombatTarget;
            }
        }
    }

    if (ClosestCombatTarget && ClosestCombatTarget != CurrentTarget) 
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
    }

    return ClosestCombatTarget;
}

void UTargetingComponent::RotateTowardsTarget(AActor* Target)
{
    RefreshOwnerController();

    if (!OwnerController || !OwnerCharacter || !Target)
    {
        if (bIsLocking)
        {
            DisableLock();
        }
        return;
    }
    
    const FVector StartLocation = OwnerCharacter->GetPawnViewLocation();
    const FVector TargetLocation = Target->GetActorLocation() + FVector(0.f, 0.f, 70.f);
    
    const FRotator TargetRotation = UKismetMathLibrary::FindLookAtRotation(StartLocation, TargetLocation);
    const FRotator CurrentControlRotation = OwnerController->GetControlRotation();
    const FRotator NewControlRotation = FMath::RInterpTo(CurrentControlRotation, TargetRotation, GetWorld()->GetDeltaSeconds(), 15.f);
    
    OwnerController->SetControlRotation(NewControlRotation);
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
        AEntity::StaticClass(), 
        ActorsToIgnore,
        Hits
    );

    UE_LOG(LogTemp, Warning, TEXT("[UTargetingComponent] GetTargets: Esfera ejecutada. Actores interceptados (brutos): %d"), Hits.Num());

    TArray<AActor*> ValidTargets;
    
    const AEntity* OwnerEntity = Cast<AEntity>(GetOwner());
    if (!OwnerEntity) 
    {
        UE_LOG(LogTemp, Error, TEXT("[UTargetingComponent] GetTargets: El Owner no es un AEntity válido."));
        return ValidTargets; 
    }

    for (const auto HitActor : Hits)
    {
        if (AEntity* TargetEntity = Cast<AEntity>(HitActor))
        {
            const bool bIsAlive = TargetEntity->IsAlive();
            const bool bIsHostile = OwnerEntity->IsHostile(TargetEntity);

            UE_LOG(LogTemp, Warning, TEXT("[UTargetingComponent] Evaluando a %s | ¿Está vivo?: %s | ¿Es hostil?: %s"), 
                *HitActor->GetName(), 
                bIsAlive ? TEXT("True") : TEXT("False"), 
                bIsHostile ? TEXT("True") : TEXT("False"));

            if (bIsAlive && bIsHostile && !TargetEntity->IsHidden() && !TargetEntity->FindComponentByClass<UTotemChargeComponent>())
            {
                ValidTargets.Add(TargetEntity);
            }
        }
        else
        {
            UE_LOG(LogTemp, Warning, TEXT("[UTargetingComponent] El actor detectado %s no casteó correctamente a AEntity."), *HitActor->GetName());
        }
    }
    
    UE_LOG(LogTemp, Warning, TEXT("[UTargetingComponent] GetTargets: Total de targets válidos retornados: %d"), ValidTargets.Num());
    return ValidTargets;
}

AActor* UTargetingComponent::FindSoftLockTarget(const float Radius) const
{
    const APawn* OwnerPawn = Cast<APawn>(GetOwner());
    if (!OwnerPawn) return nullptr;

    const FVector Origin = OwnerPawn->GetActorLocation();
    const FVector AimDirection = FRotator(0.f, OwnerPawn->GetControlRotation().Yaw, 0.f).Vector();

    AActor* BestTarget = nullptr;
    float BestAngle = SoftLockHalfAngle;
    float BestDistSq = TNumericLimits<float>::Max();

    for (AActor* Candidate : GetTargets(Radius))
    {
        FVector ToCandidate = Candidate->GetActorLocation() - Origin;
        const float DistSq = ToCandidate.SizeSquared();
        ToCandidate.Z = 0.f;
        if (!ToCandidate.Normalize()) continue;

        const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(AimDirection, ToCandidate), -1.f, 1.f)));
        if (Angle > SoftLockHalfAngle) continue;

        const bool bClearlyBetterAngle = !BestTarget || Angle < BestAngle - SoftLockAngleTieTolerance;
        const bool bTiedAngleButCloser = BestTarget && FMath::Abs(Angle - BestAngle) <= SoftLockAngleTieTolerance && DistSq < BestDistSq;
        if (bClearlyBetterAngle || bTiedAngleButCloser)
        {
            BestTarget = Candidate;
            BestAngle = Angle;
            BestDistSq = DistSq;
        }
    }

    if (bDebugSoftLock)
    {
        const float HalfAngleRad = FMath::DegreesToRadians(SoftLockHalfAngle);
        const FColor ConeColor = BestTarget ? FColor::Green : FColor::Red;
        DrawDebugCone(GetWorld(), Origin, AimDirection, Radius, HalfAngleRad, KINDA_SMALL_NUMBER, 24, ConeColor, false, 2.f, 0, 2.f);
        if (BestTarget)
        {
            DrawDebugLine(GetWorld(), Origin, BestTarget->GetActorLocation(), FColor::Green, false, 2.f, 0, 3.f);
            DrawDebugSphere(GetWorld(), BestTarget->GetActorLocation(), 40.f, 12, FColor::Green, false, 2.f);
        }
    }

    return BestTarget;
}

void UTargetingComponent::RemoveCombatTarget()
{
    if (!bIsLocking)
    {
        CurrentTarget = nullptr;
    }
}

AActor* UTargetingComponent::SearchCombatTarget(const FVector& Start, const FVector& End, const float SearchRadius) const
{
    TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
    ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

    TArray<AActor*> ActorsToIgnore;
    ActorsToIgnore.Add(GetOwner());

    TArray<FHitResult> ResultHits;
    UKismetSystemLibrary::SphereTraceMultiForObjects(
        GetWorld(),
        Start,
        End,
        SearchRadius,
        ObjectTypes,
        false,
        ActorsToIgnore,
        EDrawDebugTrace::ForDuration,
        ResultHits,
        true
    );

    if (AActor* HitActor = GetFirstNonTotemHitActor(ResultHits))
    {
        UE_LOG(LogTemp, Log, TEXT("[UTargetingComponent] SearchCombatTarget: Impacto con %s"), *HitActor->GetName());
        return HitActor;
    }

    UE_LOG(LogTemp, Warning, TEXT("[UTargetingComponent] SearchCombatTarget: No se impactó nada."));
    return nullptr;
}

AActor* UTargetingComponent::GetFirstNonTotemHitActor(const TArray<FHitResult>& Hits)
{
    for (const FHitResult& Hit : Hits)
    {
        AActor* HitActor = Hit.GetActor();
        if (HitActor && !HitActor->FindComponentByClass<UTotemChargeComponent>())
        {
            return HitActor;
        }
    }
    return nullptr;
}