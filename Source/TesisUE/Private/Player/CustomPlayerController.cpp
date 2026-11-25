#include "Player/CustomPlayerController.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "NavigationSystem.h"
#include "TimerManager.h"
#include "Animation/AnimMontage.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "DataAssets/EntityData.h"
#include "DataAssets/InputData.h"
#include "Engine/World.h"
#include "Entities/Entity.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Features/BossSystem/Subsystems/SunlightExposureSubsystem.h"
#include "Features/TotemSystem/Core/TotemChargeComponent.h"
#include "Subsystems/BloodMarkSubsystem.h"
#include "GAS/VelmaraAbilityInputID.h"
#include "GAS/VelmaraAttributeSet.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GenericTeamAgentInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundMix.h"

#include "HUD/PlayerMainHUD.h"
#include "CommonInputSubsystem.h"
#include "SceneEvents/VelmaraGameInstance.h"
#include "UI/Layout/VelmaraPrimaryGameLayout.h"
#include "UI/Widgets/Menus/VelmaraPauseMenuWidget.h"

namespace
{
    bool IsEntityDead(const AEntity* Entity)
    {
        const UAbilitySystemComponent* ASC = Entity ? Entity->GetAbilitySystemComponent() : nullptr;
        const FGameplayAttribute HealthAttribute = UVelmaraAttributeSet::GetHealthAttribute();
        return ASC && ASC->HasAttributeSetForAttribute(HealthAttribute) && ASC->GetNumericAttribute(HealthAttribute) <= 0.f;
    }

    float PlayReactionMontage(ACharacter* Character, UAnimMontage* Montage)
    {
        const float Length = Character->PlayAnimMontage(Montage);
        return Length > 0.f && Montage->RateScale > 0.f ? Length / Montage->RateScale : 0.f;
    }
}


ACustomPlayerController::ACustomPlayerController()
{
    CurrentPauseMenuInstance = nullptr;
    CurrentOptionsMenuInstance_Pause = nullptr;
    bIsGamePausedExplicitly = false;

    PauseSoundMix = nullptr;
    PlayerMainHUD = nullptr;
    OriginalBody = nullptr;
    
    TeamId = FGenericTeamId(2);
}

bool ACustomPlayerController::PerformPossession(APawn* NewPawn)
{
    if (!NewPawn || NewPawn == GetPawn()) return false;

    if (!CanPossessActor(NewPawn)) return false;

    APawn* PreviousPawn = GetPawn();
    if (!OriginalBody)
    {
        OriginalBody = PreviousPawn;
    }

    const bool bReturningToOriginalBody = (NewPawn == OriginalBody);
    const bool bWasPossessingHost = (PreviousPawn && PreviousPawn != OriginalBody) || bOriginalBodyProtected;
    APawn* ReleasedHost = (PreviousPawn && PreviousPawn != OriginalBody) ? PreviousPawn : nullptr;

    AController* NewPawnPreviousController = NewPawn->GetController();
    const AEntity* NewEntity = Cast<AEntity>(NewPawn);
    const FGenericTeamId NewPawnTeam = NewEntity ? NewEntity->GetTeamId() : FGenericTeamId::NoTeam;

    if (bReturningToOriginalBody && bWasPossessingHost)
    {
        RestoreOriginalBody(ReleasedHost);
    }

    GetWorldTimerManager().ClearTimer(HostPainTimer);

    UnPossess();

    Possess(NewPawn);

    if (UAbilitySystemComponent* NewASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(NewPawn))
    {
        NewASC->InitAbilityActorInfo(this, NewPawn);
    }

    if (ReleasedHost)
    {
        ReleaseHost(ReleasedHost);
    }

    if (!bReturningToOriginalBody)
    {
        ClearPossessionHover();
        if (UBloodMarkSubsystem* BloodMarks = GetWorld() ? GetWorld()->GetSubsystem<UBloodMarkSubsystem>() : nullptr)
        {
            BloodMarks->ClearMarkForActor(NewPawn);
        }

        GetWorldTimerManager().ClearTimer(EmergeVerifyTimer);
        ProtectOriginalBody();
        HostAIController = NewPawnPreviousController;
        HostTeamId = NewPawnTeam;
        bEndingPossession = false;
        LastKnownHostLocation = NewPawn->GetActorLocation();
        bHasLastKnownHostLocation = true;
        GetWorldTimerManager().SetTimer(PossessionTickTimer, this, &ACustomPlayerController::TickPossession, PossessionTickInterval, true);
        OnPossessionStarted.Broadcast(NewPawn);
        PlayHostPainReaction(NewPawn);
    }
    else
    {
        GetWorldTimerManager().ClearTimer(PossessionTickTimer);
        bEndingPossession = false;
        bHasLastKnownHostLocation = false;
        if (bWasPossessingHost)
        {
            OnPossessionEnded.Broadcast(ReleasedHost, PostPossessionSunGraceDuration);
        }
    }

    return true;
}

void ACustomPlayerController::ReleaseHost(APawn* Host)
{
    if (UAbilitySystemComponent* HostASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(Host))
    {
        HostASC->InitAbilityActorInfo(Host, Host);
    }

    AEntity* HostEntity = Cast<AEntity>(Host);
    if (!HostEntity) return;

    HostEntity->TeamId = HostTeamId;

    if (AController* OldController = HostAIController.Get())
    {
        OldController->Destroy();
    }
    HostAIController = nullptr;

    if (IsEntityDead(HostEntity))
    {
        HostEntity->SpawnDefaultController();
        return;
    }

    const float RecoveryDuration = HostEntity->EntityData ? PlayReactionMontage(HostEntity, HostEntity->EntityData->ReleasedMontage) : 0.f;

    TWeakObjectPtr<AEntity> WeakHost = HostEntity;
    GetWorldTimerManager().SetTimer(HostEntity->PossessionRecoveryTimer, [WeakHost]()
    {
        AEntity* Entity = WeakHost.Get();
        if (Entity && !IsEntityDead(Entity) && !Entity->GetController())
        {
            Entity->SpawnDefaultController();
        }
    }, RecoveryDuration > 0.f ? RecoveryDuration : FMath::Max(UnpossessStunDuration, 0.01f), false);
}

void ACustomPlayerController::PlayHostPainReaction(APawn* Host)
{
    AEntity* HostEntity = Cast<AEntity>(Host);
    if (!HostEntity || !HostEntity->EntityData) return;

    HostEntity->GetCharacterMovement()->StopMovementImmediately();

    const float PainDuration = PlayReactionMontage(HostEntity, HostEntity->EntityData->PossessedMontage);
    if (PainDuration <= 0.f) return;

    SetIgnoreMoveInput(true);
    GetWorldTimerManager().SetTimer(HostPainTimer, this, &ACustomPlayerController::EndHostPainReaction, PainDuration, false);
}

void ACustomPlayerController::EndHostPainReaction()
{
    SetIgnoreMoveInput(false);
}

void ACustomPlayerController::TickPossession()
{
    if (!IsPossessing()) return;

    if (!GetPawn())
    {
        if (OriginalBody) PerformPossession(OriginalBody);
        return;
    }

    LastKnownHostLocation = GetPawn()->GetActorLocation();
    bHasLastKnownHostLocation = true;

    if (bEndingPossession)
    {
        PerformPossession(OriginalBody);
        return;
    }

    const AEntity* Host = Cast<AEntity>(GetPawn());
    UAbilitySystemComponent* BodyASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OriginalBody);
    bool bShouldEnd = IsEntityDead(Host);

    if (BodyASC && Host)
    {
        const FGameplayAttribute EnergyAttribute = UVelmaraAttributeSet::GetEnergyAttribute();
        const float NewEnergy = FMath::Max(0.f, BodyASC->GetNumericAttribute(EnergyAttribute) - PossessionEnergyDrainPerSecond * PossessionTickInterval);
        BodyASC->SetNumericAttributeBase(EnergyAttribute, NewEnergy);
        bShouldEnd |= NewEnergy <= 0.f;
    }

    if (bShouldEnd)
    {
        bEndingPossession = true;
        UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(GetPawn(), FGameplayTag::RequestGameplayTag(TEXT("Event.Posses.Return")), FGameplayEventData());
    }
}

bool ACustomPlayerController::IsPossessing() const
{
    return OriginalBody != nullptr && OriginalBody != GetPawn();
}

bool ACustomPlayerController::IsOriginalBodyProtected() const
{
    return bOriginalBodyProtected && IsValid(OriginalBody);
}

bool ACustomPlayerController::IsPossessedHost(const AActor* Candidate) const
{
    return IsPossessing() && Candidate != nullptr && Candidate == GetPawn() && Candidate != OriginalBody;
}

void ACustomPlayerController::RestoreOriginalBodyEnergy(const float Amount)
{
    if (Amount <= 0.f || !OriginalBody) return;

    UAbilitySystemComponent* BodyASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OriginalBody);
    if (!BodyASC) return;

    const FGameplayAttribute EnergyAttribute = UVelmaraAttributeSet::GetEnergyAttribute();
    const FGameplayAttribute MaxEnergyAttribute = UVelmaraAttributeSet::GetMaxEnergyAttribute();
    if (!BodyASC->HasAttributeSetForAttribute(EnergyAttribute)) return;

    const float CurrentEnergy = BodyASC->GetNumericAttribute(EnergyAttribute);
    float NewEnergy = FMath::Max(0.f, CurrentEnergy + Amount);

    if (BodyASC->HasAttributeSetForAttribute(MaxEnergyAttribute))
    {
        const float MaxEnergy = BodyASC->GetNumericAttribute(MaxEnergyAttribute);
        if (MaxEnergy > 0.f)
        {
            NewEnergy = FMath::Clamp(NewEnergy, 0.f, MaxEnergy);
        }
    }

    BodyASC->SetNumericAttributeBase(EnergyAttribute, NewEnergy);

    if (IsPossessing())
    {
        APawn* CurrentHost = GetPawn();
        if (CurrentHost && CurrentHost != OriginalBody)
        {
            if (UAbilitySystemComponent* HostASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(CurrentHost))
            {
                if (HostASC != BodyASC && HostASC->HasAttributeSetForAttribute(EnergyAttribute))
                {
                    float HostEnergy = NewEnergy;
                    if (HostASC->HasAttributeSetForAttribute(MaxEnergyAttribute))
                    {
                        const float HostMaxEnergy = HostASC->GetNumericAttribute(MaxEnergyAttribute);
                        if (HostMaxEnergy > 0.f)
                        {
                            HostEnergy = FMath::Clamp(HostEnergy, 0.f, HostMaxEnergy);
                        }
                    }
                    HostASC->SetNumericAttributeBase(EnergyAttribute, HostEnergy);
                }
            }
        }
    }
}

void ACustomPlayerController::ProtectOriginalBody()
{
    if (bOriginalBodyProtected || !OriginalBody) return;

    OriginalBody->SetActorHiddenInGame(true);
    OriginalBody->SetActorEnableCollision(false);
    OriginalBody->SetCanBeDamaged(false);

    if (ACharacter* OriginalCharacter = Cast<ACharacter>(OriginalBody))
    {
        if (UCharacterMovementComponent* Movement = OriginalCharacter->GetCharacterMovement())
        {
            Movement->StopMovementImmediately();
            Movement->SetMovementMode(MOVE_None);
        }
    }

    bOriginalBodyProtected = true;

    if (UWorld* World = GetWorld())
    {
        if (USunlightExposureSubsystem* SunSubsystem = World->GetSubsystem<USunlightExposureSubsystem>())
        {
            SunSubsystem->SetSunProtection(OriginalBody, true);
        }
    }
}

void ACustomPlayerController::RestoreOriginalBody(APawn* ReferenceHost)
{
    if (!OriginalBody) return;

    FVector SafeLocation = OriginalBody->GetActorLocation();
    FRotator SafeRotation = FRotator(0.f, GetControlRotation().Yaw, 0.f);
    FindSafeEmergeLocation(ReferenceHost, SafeLocation, SafeRotation);

    if (UWorld* World = GetWorld())
    {
        if (USunlightExposureSubsystem* SunSubsystem = World->GetSubsystem<USunlightExposureSubsystem>())
        {
            SunSubsystem->SetSunProtection(OriginalBody, false);
            SunSubsystem->GrantSunGrace(OriginalBody, PostPossessionSunGraceDuration);
        }
    }

    OriginalBody->SetActorLocationAndRotation(SafeLocation, SafeRotation, false, nullptr, ETeleportType::TeleportPhysics);
    OriginalBody->SetActorEnableCollision(true);
    OriginalBody->SetActorHiddenInGame(false);
    OriginalBody->SetCanBeDamaged(true);

    if (ACharacter* OriginalCharacter = Cast<ACharacter>(OriginalBody))
    {
        if (UCharacterMovementComponent* Movement = OriginalCharacter->GetCharacterMovement())
        {
            Movement->SetMovementMode(MOVE_Walking);
        }
    }

    bOriginalBodyProtected = false;
    OriginalBody->TeleportTo(SafeLocation, SafeRotation, false, true);

    LastReleasedHost = ReferenceHost;
    GetWorldTimerManager().ClearTimer(EmergeVerifyTimer);
    EmergeVerifyTimer = GetWorldTimerManager().SetTimerForNextTick(this, &ACustomPlayerController::VerifyOriginalBodyEmergeLocation);
}

bool ACustomPlayerController::FindSafeEmergeLocation(const APawn* ReferenceHost, FVector& OutLocation, FRotator& OutRotation) const
{
    if (!OriginalBody) return false;

    UWorld* World = GetWorld();
    const FRotator ControlRot = GetControlRotation();
    OutRotation = FRotator(0.f, ControlRot.Yaw, 0.f);

    float CapsuleHalfHeight = 88.f;
    float CapsuleRadius = 34.f;
    ECollisionChannel CapsuleChannel = ECC_Pawn;
    if (const ACharacter* OriginalCharacter = Cast<ACharacter>(OriginalBody))
    {
        if (const UCapsuleComponent* Capsule = OriginalCharacter->GetCapsuleComponent())
        {
            CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
            CapsuleRadius = Capsule->GetScaledCapsuleRadius();
            CapsuleChannel = Capsule->GetCollisionObjectType();
        }
    }

    FVector HostLocation = OriginalBody->GetActorLocation();
    if (ReferenceHost)
    {
        HostLocation = ReferenceHost->GetActorLocation();
    }
    else if (bHasLastKnownHostLocation)
    {
        HostLocation = LastKnownHostLocation;
    }

    float HostHalfHeight = CapsuleHalfHeight;
    float HostRadius = CapsuleRadius;
    if (const ACharacter* HostCharacter = Cast<ACharacter>(ReferenceHost))
    {
        if (const UCapsuleComponent* HostCapsule = HostCharacter->GetCapsuleComponent())
        {
            HostHalfHeight = HostCapsule->GetScaledCapsuleHalfHeight();
            HostRadius = HostCapsule->GetScaledCapsuleRadius();
        }
    }

    const AEntity* HostEntity = Cast<AEntity>(ReferenceHost);
    const bool bHostDead = !ReferenceHost || (HostEntity && (!HostEntity->IsAlive() || IsEntityDead(HostEntity)));
    const FVector FallbackLocation(HostLocation.X, HostLocation.Y, HostLocation.Z - HostHalfHeight + CapsuleHalfHeight);

    if (!World)
    {
        OutLocation = FallbackLocation;
        return true;
    }

    UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World);
    const FVector QueryExtent(EmergeSearchRadius, EmergeSearchRadius, 300.f);

    auto AdjustNavPointToCapsuleCenter = [&](const FVector& NavPoint) -> FVector
    {
        float FloorZ = NavPoint.Z;
        FHitResult FloorHit;
        FCollisionQueryParams FloorParams(SCENE_QUERY_STAT(FindSafeEmergeFloor), false, OriginalBody);
        if (ReferenceHost)
        {
            FloorParams.AddIgnoredActor(ReferenceHost);
        }
        const FVector TraceStart = NavPoint + FVector(0.f, 0.f, CapsuleHalfHeight);
        const FVector TraceEnd = NavPoint - FVector(0.f, 0.f, 50.f);
        if (World->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_WorldStatic, FloorParams))
        {
            if (FloorHit.ImpactNormal.Z > 0.5f)
            {
                FloorZ = FMath::Max(FloorZ, FloorHit.ImpactPoint.Z);
            }
        }
        return FVector(NavPoint.X, NavPoint.Y, FloorZ + CapsuleHalfHeight + 2.f);
    };

    auto ValidateCandidate = [&](FVector& InOutCandidate) -> bool
    {
        if (!OriginalBody->GetActorEnableCollision())
        {
            FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(FindSafeEmergeOverlap), false, OriginalBody);
            if (bHostDead && ReferenceHost)
            {
                QueryParams.AddIgnoredActor(ReferenceHost);
            }
            const float TestRadius = FMath::Max(CapsuleRadius - 2.f, 10.f);
            const float TestHalfHeight = FMath::Max(CapsuleHalfHeight - 10.f, 10.f);
            const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(TestRadius, TestHalfHeight);
            const FVector TestCenter = InOutCandidate + FVector(0.f, 0.f, 5.f);
            if (World->OverlapBlockingTestByChannel(TestCenter, FQuat::Identity, CapsuleChannel, CapsuleShape, QueryParams))
            {
                return false;
            }
        }

        if (!World->FindTeleportSpot(OriginalBody, InOutCandidate, OutRotation))
        {
            return false;
        }

        if (!bHostDead && ReferenceHost)
        {
            const float MinHorizontalSep = CapsuleRadius + HostRadius + 5.f;
            const float Dist2DSq = FVector::DistSquared2D(InOutCandidate, HostLocation);
            const float DistZ = FMath::Abs(InOutCandidate.Z - HostLocation.Z);
            if (Dist2DSq < FMath::Square(MinHorizontalSep) && DistZ < (CapsuleHalfHeight + HostHalfHeight))
            {
                return false;
            }
        }

        return true;
    };

    static constexpr float RingYawOffsets[] = { 180.f, 135.f, -135.f, 90.f, -90.f, 45.f, -45.f, 0.f };
    TArray<FVector> CandidatePoints;
    CandidatePoints.Reserve(9);

    for (const float YawOffset : RingYawOffsets)
    {
        const FRotator DirRot(0.f, ControlRot.Yaw + YawOffset, 0.f);
        const FVector Offset = DirRot.Vector() * EmergeOffsetDistance;
        CandidatePoints.Add(HostLocation + Offset);
    }

    if (bHostDead)
    {
        CandidatePoints.Add(HostLocation);
    }

    for (const FVector& RawCandidate : CandidatePoints)
    {
        FVector CandidateLoc;
        if (NavSys)
        {
            FNavLocation ProjectedNav;
            if (!NavSys->ProjectPointToNavigation(RawCandidate, ProjectedNav, QueryExtent))
            {
                continue;
            }
            CandidateLoc = AdjustNavPointToCapsuleCenter(ProjectedNav.Location);
        }
        else
        {
            CandidateLoc = FVector(RawCandidate.X, RawCandidate.Y, FallbackLocation.Z);
        }

        if (ValidateCandidate(CandidateLoc))
        {
            OutLocation = CandidateLoc;
            return true;
        }
    }

    if (NavSys)
    {
        for (int32 Attempt = 0; Attempt < 6; ++Attempt)
        {
            FNavLocation RandomNav;
            if (NavSys->GetRandomReachablePointInRadius(HostLocation, EmergeSearchRadius, RandomNav))
            {
                FVector CandidateLoc = AdjustNavPointToCapsuleCenter(RandomNav.Location);
                if (ValidateCandidate(CandidateLoc))
                {
                    OutLocation = CandidateLoc;
                    return true;
                }
            }
        }

        FNavLocation RandomNav;
        if (NavSys->GetRandomReachablePointInRadius(HostLocation, EmergeSearchRadius, RandomNav))
        {
            OutLocation = AdjustNavPointToCapsuleCenter(RandomNav.Location);
            return true;
        }
    }

    OutLocation = FallbackLocation;
    return true;
}

void ACustomPlayerController::VerifyOriginalBodyEmergeLocation()
{
    APawn* ReleasedHost = LastReleasedHost.Get();
    LastReleasedHost = nullptr;

    if (!OriginalBody || IsPossessing() || GetPawn() != OriginalBody) return;

    UWorld* World = GetWorld();
    if (!World) return;

    const FVector CurrentLocation = OriginalBody->GetActorLocation();
    float CapsuleHalfHeight = 88.f;
    float CapsuleRadius = 34.f;
    if (const ACharacter* OriginalCharacter = Cast<ACharacter>(OriginalBody))
    {
        if (const UCapsuleComponent* Capsule = OriginalCharacter->GetCapsuleComponent())
        {
            CapsuleHalfHeight = Capsule->GetScaledCapsuleHalfHeight();
            CapsuleRadius = Capsule->GetScaledCapsuleRadius();
        }
    }

    bool bOnNavMesh = true;
    if (UNavigationSystemV1* NavSys = UNavigationSystemV1::GetCurrent(World))
    {
        FNavLocation NavCheck;
        const FVector CheckExtent(CapsuleRadius + 20.f, CapsuleRadius + 20.f, CapsuleHalfHeight + 150.f);
        bOnNavMesh = NavSys->ProjectPointToNavigation(CurrentLocation, NavCheck, CheckExtent);
    }

    bool bOverlappingLiveHost = false;
    if (ReleasedHost && ReleasedHost != OriginalBody)
    {
        const AEntity* HostEntity = Cast<AEntity>(ReleasedHost);
        const bool bHostAlive = HostEntity ? (HostEntity->IsAlive() && !IsEntityDead(HostEntity)) : true;
        if (bHostAlive)
        {
            float HostRadius = 34.f;
            float HostHalfHeight = 88.f;
            if (const ACharacter* HostCharacter = Cast<ACharacter>(ReleasedHost))
            {
                if (const UCapsuleComponent* HostCapsule = HostCharacter->GetCapsuleComponent())
                {
                    HostRadius = HostCapsule->GetScaledCapsuleRadius();
                    HostHalfHeight = HostCapsule->GetScaledCapsuleHalfHeight();
                }
            }

            const float MinSep = CapsuleRadius + HostRadius;
            const float Dist2DSq = FVector::DistSquared2D(CurrentLocation, ReleasedHost->GetActorLocation());
            const float DistZ = FMath::Abs(CurrentLocation.Z - ReleasedHost->GetActorLocation().Z);
            bOverlappingLiveHost = (Dist2DSq < FMath::Square(MinSep)) && (DistZ < (CapsuleHalfHeight + HostHalfHeight));
        }
    }

    FCollisionQueryParams StaticOverlapParams(SCENE_QUERY_STAT(VerifyOriginalBodyStaticOverlap), false, OriginalBody);
    TArray<AActor*> AttachedActors;
    OriginalBody->GetAttachedActors(AttachedActors, true, true);
    StaticOverlapParams.AddIgnoredActors(AttachedActors);
    if (ReleasedHost)
    {
        StaticOverlapParams.AddIgnoredActor(ReleasedHost);
    }
    const float TestRadius = FMath::Max(CapsuleRadius - 2.f, 10.f);
    const float TestHalfHeight = FMath::Max(CapsuleHalfHeight - 10.f, TestRadius);
    const FCollisionShape CapsuleShape = FCollisionShape::MakeCapsule(TestRadius, TestHalfHeight);
    const FVector TestCenter = CurrentLocation + FVector(0.f, 0.f, 5.f);
    const bool bOverlappingWorldStatic = World->OverlapBlockingTestByChannel(TestCenter, FQuat::Identity, ECC_WorldStatic, CapsuleShape, StaticOverlapParams);

    if (!bOnNavMesh || bOverlappingLiveHost || bOverlappingWorldStatic)
    {
        FVector SafeLocation = CurrentLocation;
        FRotator SafeRotation = OriginalBody->GetActorRotation();
        if (FindSafeEmergeLocation(ReleasedHost, SafeLocation, SafeRotation))
        {
            OriginalBody->TeleportTo(SafeLocation, SafeRotation, false, true);
        }
    }
}

void ACustomPlayerController::TickShadowEnergyRegen()
{
    if (!OriginalBody && !IsPossessing())
    {
        OriginalBody = GetPawn();
    }

    if (IsPossessing() || !OriginalBody) return;

    const AEntity* OriginalEntity = Cast<AEntity>(OriginalBody);
    if (!OriginalEntity || !OriginalEntity->IsAlive()) return;

    UWorld* World = GetWorld();
    if (!World) return;

    float RegenRate = bEnablePassiveEnergyRegen ? PassiveEnergyRegenPerSecond : 0.f;

    if (bEnableShadowEnergyRegen)
    {
        const USunlightExposureSubsystem* SunSubsystem = World->GetSubsystem<USunlightExposureSubsystem>();
        if (SunSubsystem && SunSubsystem->IsInShadowDuringSunHazard(OriginalBody))
        {
            RegenRate = FMath::Max(RegenRate, ShadowEnergyRegenPerSecond);
        }
    }

    if (RegenRate > 0.f)
    {
        const float EffectiveInterval = FMath::Max(ShadowEnergyRegenInterval, 0.05f);
        RestoreOriginalBodyEnergy(RegenRate * EffectiveInterval);
    }
}

bool ACustomPlayerController::CanPossessActor(const AActor* Candidate) const
{
    if (!Candidate || Candidate == GetPawn()) return false;

    if (Candidate == OriginalBody) return true;

    if (!UTotemChargeComponent::CanActorBePossessed(Candidate)) return false;

    if (const AEntity* CandidateEntity = Cast<AEntity>(Candidate))
    {
        if (IsEntityDead(CandidateEntity) || CandidateEntity->IsPlayerControlled()) return false;
    }

    const AActor* BodyActor = OriginalBody ? static_cast<const AActor*>(OriginalBody) : static_cast<const AActor*>(GetPawn());
    if (const IAbilitySystemInterface* BodyASI = Cast<IAbilitySystemInterface>(BodyActor))
    {
        if (const UAbilitySystemComponent* BodyASC = BodyASI->GetAbilitySystemComponent())
        {
            const FGameplayAttribute EnergyAttribute = UVelmaraAttributeSet::GetEnergyAttribute();
            if (BodyASC->HasAttributeSetForAttribute(EnergyAttribute) && BodyASC->GetNumericAttribute(EnergyAttribute) < MinEnergyToStartPossession)
            {
                return false;
            }
        }
    }

    if (bRequireBloodMarkForEnemyPossession && Cast<AEntity>(Candidate) != nullptr && !Candidate->FindComponentByClass<UTotemChargeComponent>())
    {
        bool bMarked = false;
        if (const UWorld* World = GetWorld())
        {
            if (const UBloodMarkSubsystem* BloodMarks = World->GetSubsystem<UBloodMarkSubsystem>())
            {
                bMarked = BloodMarks->IsActorBloodMarked(Candidate);
            }
        }
        if (!bMarked) return false;
    }

    return true;
}

void ACustomPlayerController::TickPossessionHover()
{
    UWorld* World = GetWorld();
    APawn* ControlledPawn = GetPawn();
    if (!World || !ControlledPawn || !IsLocalController())
    {
        ClearPossessionHover();
        return;
    }

    FVector ViewLocation;
    FRotator ViewRotation;
    GetPlayerViewPoint(ViewLocation, ViewRotation);
    const FVector ViewForward = ViewRotation.Vector();

    const FVector Start = ControlledPawn->GetActorLocation() + ViewForward;
    const FVector End = ViewLocation + ViewForward * PossessionHoverTraceLength;

    FCollisionQueryParams Params(SCENE_QUERY_STAT(PossessionHover), false);
    Params.AddIgnoredActor(ControlledPawn);
    if (OriginalBody)
    {
        Params.AddIgnoredActor(OriginalBody);
    }

    FHitResult Hit;
    const bool bHit = World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_GameTraceChannel6, FCollisionShape::MakeSphere(FMath::Max(PossessionHoverTraceRadius, 1.f)), Params);

    AActor* NewHovered = nullptr;
    if (bHit)
    {
        AActor* HitActor = Hit.GetActor();
        if (HitActor && CanPossessActor(HitActor) && HitActor->FindFunction(TEXT("OnHoveredPossession")))
        {
            NewHovered = HitActor;
        }
    }

    AActor* OldHovered = CurrentHoveredPossessable.Get();
    if (NewHovered != OldHovered)
    {
        if (OldHovered)
        {
            NotifyPossessionHover(OldHovered, false);
        }
        CurrentHoveredPossessable = NewHovered;
        if (NewHovered)
        {
            NotifyPossessionHover(NewHovered, true);
        }
    }
}

void ACustomPlayerController::ClearPossessionHover()
{
    if (AActor* Old = CurrentHoveredPossessable.Get())
    {
        NotifyPossessionHover(Old, false);
    }
    CurrentHoveredPossessable.Reset();
}

void ACustomPlayerController::NotifyPossessionHover(AActor* Target, const bool bIsHovered)
{
    if (!IsValid(Target)) return;

    if (UFunction* HoverFunction = Target->FindFunction(TEXT("OnHoveredPossession")))
    {
        if (HoverFunction->NumParms == 1)
        {
            if (FBoolProperty* BoolParam = CastField<FBoolProperty>(HoverFunction->PropertyLink))
            {
                uint8* Params = static_cast<uint8*>(FMemory_Alloca(HoverFunction->ParmsSize));
                FMemory::Memzero(Params, HoverFunction->ParmsSize);
                BoolParam->SetPropertyValue_InContainer(Params, bIsHovered);
                Target->ProcessEvent(HoverFunction, Params);
            }
        }
    }

    OnPossessionHoverChanged.Broadcast(Target, bIsHovered);
}
void ACustomPlayerController::BeginPlay()
{
    Super::BeginPlay();

    OriginalBody = GetPawn();
    
    PlayerMainHUD = Cast<APlayerMainHUD>(GetHUD());

    if (const UVelmaraGameInstance* VelmaraGI = Cast<UVelmaraGameInstance>(GetGameInstance()))
    {
        bForceFeedbackEnabled = VelmaraGI->IsVibrationEnabled();
    }

    if (IsLocalController())
    {
        UVelmaraPrimaryGameLayout::GetOrCreate(this);

        const float HoverInterval = FMath::Max(PossessionHoverTickInterval, 0.05f);
        GetWorldTimerManager().SetTimer(PossessionHoverTimer, this, &ACustomPlayerController::TickPossessionHover, HoverInterval, true);
    }

    if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(GetLocalPlayer()))
    {
        if (DefaultMappingContext)
        {
            Subsystem->AddMappingContext(DefaultMappingContext, 0);
        }
    }

    const float RegenInterval = FMath::Max(ShadowEnergyRegenInterval, 0.05f);
    GetWorldTimerManager().SetTimer(ShadowEnergyRegenTimer, this, &ACustomPlayerController::TickShadowEnergyRegen, RegenInterval, true);
}

void ACustomPlayerController::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    ClearPossessionHover();

    if (UWorld* World = GetWorld())
    {
        FTimerManager& TimerManager = World->GetTimerManager();
        TimerManager.ClearTimer(PossessionTickTimer);
        TimerManager.ClearTimer(HostPainTimer);
        TimerManager.ClearTimer(ShadowEnergyRegenTimer);
        TimerManager.ClearTimer(EmergeVerifyTimer);
        TimerManager.ClearTimer(PossessionHoverTimer);

        if (bOriginalBodyProtected && OriginalBody)
        {
            if (USunlightExposureSubsystem* SunSubsystem = World->GetSubsystem<USunlightExposureSubsystem>())
            {
                SunSubsystem->SetSunProtection(OriginalBody, false);
            }
            bOriginalBodyProtected = false;
        }
    }

    Super::EndPlay(EndPlayReason);
}

void ACustomPlayerController::SetupInputComponent()
{
    Super::SetupInputComponent();

    if (InputComponent)
    {
        FInputActionBinding& PauseBinding = InputComponent->BindAction("PauseGame", IE_Pressed, this, &ACustomPlayerController::TogglePauseMenu);
        PauseBinding.bExecuteWhenPaused = true;
    }

    if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent))
    {

        if (InputData->Inputs.CrouchAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.CrouchAction, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Crouch);
            EnhancedInputComponent->BindAction(InputData->Inputs.CrouchAction, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Crouch);
        }
        
        if (InputData->Inputs.JumpAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.JumpAction, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Jump);
            EnhancedInputComponent->BindAction(InputData->Inputs.JumpAction, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Jump);
        }
        
        if (InputData->Inputs.DodgeAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.DodgeAction, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Dodge);
            EnhancedInputComponent->BindAction(InputData->Inputs.DodgeAction, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Dodge);
        }
        
        if (InputData->Inputs.InteractAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.InteractAction, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Interact);
            EnhancedInputComponent->BindAction(InputData->Inputs.InteractAction, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Interact);
        }

        if (InputData->Inputs.InventoryAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.InventoryAction, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Interact);
            EnhancedInputComponent->BindAction(InputData->Inputs.InventoryAction, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Interact);
        }

        if (InputData->Inputs.ToggleWeaponAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.ToggleWeaponAction, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Interact);
            EnhancedInputComponent->BindAction(InputData->Inputs.ToggleWeaponAction, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Interact);
        }
        
        if (InputData->Inputs.AttackAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.AttackAction, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Attack);
            EnhancedInputComponent->BindAction(InputData->Inputs.AttackAction, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Attack);
        }

        if (InputData->Inputs.InputAction_Skill1)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill1, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Skill1);
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill1, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Skill1);
        }

        if (InputData->Inputs.InputAction_Skill2)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill2, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Skill2);
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill2, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Skill2);
        }

        if (InputData->Inputs.InputAction_Skill3)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill3, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Skill3);
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill3, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Skill3);
        }

        if (InputData->Inputs.InputAction_Skill4)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill4, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Skill4);
            EnhancedInputComponent->BindAction(InputData->Inputs.InputAction_Skill4, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Skill4);
        }

        if (InputData->Inputs.PossessAction)
        {
            EnhancedInputComponent->BindAction(InputData->Inputs.PossessAction, ETriggerEvent::Started, this, &ACustomPlayerController::AbilityInputTagPressed, EVelmaraAbilityInputID::Possess);
            EnhancedInputComponent->BindAction(InputData->Inputs.PossessAction, ETriggerEvent::Completed, this, &ACustomPlayerController::AbilityInputTagReleased, EVelmaraAbilityInputID::Possess);
        }
    }
}

void ACustomPlayerController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
    if (TeamId != NewTeamID)
    {
        TeamId = NewTeamID;
    }
}

FGenericTeamId ACustomPlayerController::GetGenericTeamId() const
{
    return TeamId;
}

ETeamAttitude::Type ACustomPlayerController::GetTeamAttitudeTowards(const AActor& Other) const
{
    const IGenericTeamAgentInterface* OtherTeamAgent = Cast<const IGenericTeamAgentInterface>(&Other);
    if (!OtherTeamAgent) return ETeamAttitude::Neutral;

    const FGenericTeamId OtherTeamId = OtherTeamAgent->GetGenericTeamId();

    if (OtherTeamId == 255) return ETeamAttitude::Neutral;
    if (OtherTeamId == TeamId) return ETeamAttitude::Friendly;
    
    return ETeamAttitude::Hostile;
}

void ACustomPlayerController::AbilityInputTagPressed(EVelmaraAbilityInputID InputId)
{
    if (InputId != EVelmaraAbilityInputID::Possess && GetWorldTimerManager().IsTimerActive(HostPainTimer)) return;

    if (APawn* ControlledPawn = GetPawn())
    {
        if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(ControlledPawn))
        {
            if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
            {
                if (ASC)
                {
                    ASC->AbilityLocalInputPressed(static_cast<int32>(InputId));
                }
            }
        }
    }
}

void ACustomPlayerController::AbilityInputTagReleased(EVelmaraAbilityInputID InputId)
{
    if (APawn* ControlledPawn = GetPawn())
    {
        if (IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(ControlledPawn))
        {
            if (UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent())
            {
                if (ASC)
                {
                    ASC->AbilityLocalInputReleased(static_cast<int32>(InputId));
                }
            }
        }
    }
}

void ACustomPlayerController::SetPauseAudioMixState(bool bPaused)
{
    bIsGamePausedExplicitly = bPaused;
    if (PauseSoundMix && GetWorld())
    {
        if (bPaused)
        {
            UGameplayStatics::PushSoundMixModifier(GetWorld(), PauseSoundMix);
        }
        else
        {
            UGameplayStatics::PopSoundMixModifier(GetWorld(), PauseSoundMix);
        }
    }
}

void ACustomPlayerController::AddYawInput(float Val)
{
    float Scale = 1.0f;
    if (const UVelmaraGameInstance* VelmaraGI = Cast<UVelmaraGameInstance>(GetGameInstance()))
    {
        const UCommonInputSubsystem* CommonInput = UCommonInputSubsystem::Get(GetLocalPlayer());
        const bool bGamepad = CommonInput && CommonInput->GetCurrentInputType() == ECommonInputType::Gamepad;
        Scale = bGamepad ? VelmaraGI->GetGamepadSensitivity() : VelmaraGI->GetMouseSensitivity();
    }
    Super::AddYawInput(Val * Scale);
}

void ACustomPlayerController::AddPitchInput(float Val)
{
    float Scale = 1.0f;
    if (const UVelmaraGameInstance* VelmaraGI = Cast<UVelmaraGameInstance>(GetGameInstance()))
    {
        const UCommonInputSubsystem* CommonInput = UCommonInputSubsystem::Get(GetLocalPlayer());
        const bool bGamepad = CommonInput && CommonInput->GetCurrentInputType() == ECommonInputType::Gamepad;
        Scale = bGamepad ? VelmaraGI->GetGamepadSensitivity() : VelmaraGI->GetMouseSensitivity();
        if (VelmaraGI->IsInvertY())
        {
            Scale = -Scale;
        }
    }
    Super::AddPitchInput(Val * Scale);
}

void ACustomPlayerController::TogglePauseMenu()
{
    if (PlayerMainHUD)
    {
        PlayerMainHUD->TogglePauseMenu();
        return;
    }

    if (UVelmaraPrimaryGameLayout* Layout = UVelmaraPrimaryGameLayout::GetOrCreate(this))
    {
        if (Layout->HasActiveModal())
        {
            return;
        }
        if (Layout->HasActiveMenu())
        {
            if (UCommonActivatableWidgetStack* MenuStack = Layout->GetMenuStack())
            {
                if (UCommonActivatableWidget* ActiveMenu = MenuStack->GetActiveWidget())
                {
                    ActiveMenu->DeactivateWidget();
                    return;
                }
            }
        }
        Layout->PushMenuWidget<UVelmaraPauseMenuWidget>(UVelmaraPauseMenuWidget::StaticClass());
    }
}
