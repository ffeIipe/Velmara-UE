#include "Features/BossSystem/Components/BossCeilingBreakComponent.h"

#include "EngineUtils.h"
#include "NavigationSystem.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Camera/CameraShakeBase.h"
#include "Features/BossSystem/Actors/BossCeilingBreakPoint.h"
#include "Features/BossSystem/Actors/BossLightShaft.h"
#include "Features/BossSystem/Components/BossCombatComponent.h"
#include "Features/BossSystem/Components/BossSummonComponent.h"
#include "Features/BossSystem/Core/BossEnemy.h"
#include "Features/BossSystem/Core/BossSystemLibrary.h"
#include "GenericPool/Core/ActorPoolSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace BossCeiling
{
	constexpr int32 RandomPointAttempts = 12;
	constexpr float MinShaftHeight = 150.f;
}

UBossCeilingBreakComponent::UBossCeilingBreakComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	LightShaftClass = ABossLightShaft::StaticClass();

	FBossCeilingStage Stage1To2;
	Stage1To2.HealthThreshold = 0.66f;
	Stage1To2.ShaftCount = 3;
	Stage1To2.bOpenShaftOnTarget = true;
	Stage1To2.bSummonMinions = true;
	Stage1To2.MinionCountOverride = 3;
	Stage1To2.bFullArenaCollapse = false;
	Stage1To2.bActivateSolarShield = false;
	Stage1To2.SolarShieldAmountOverride = 0.f;
	Stages.Add(Stage1To2);

	FBossCeilingStage Stage2To3;
	Stage2To3.HealthThreshold = 0.33f;
	Stage2To3.ShaftCount = 0;
	Stage2To3.bOpenShaftOnTarget = false;
	Stage2To3.bSummonMinions = true;
	Stage2To3.MinionCountOverride = 4;
	Stage2To3.bFullArenaCollapse = true;
	Stage2To3.bActivateSolarShield = true;
	Stage2To3.SolarShieldAmountOverride = 0.f;
	Stages.Add(Stage2To3);

	static ConstructorHelpers::FClassFinder<UCameraShakeBase> ShakeFinder(TEXT("/Game/Blueprints/Cameras/CameraShakes/CS_Rock"));
	if (ShakeFinder.Succeeded())
	{
		BreakCameraShake = ShakeFinder.Class;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> BreakFinder(TEXT("/Game/MechanicalDamageFX/FX/NS_ImpactDebris_Medium.NS_ImpactDebris_Medium"));
	if (BreakFinder.Succeeded())
	{
		BreakEffect = BreakFinder.Object;
	}
}

void UBossCeilingBreakComponent::BeginPlay()
{
	Super::BeginPlay();

	SpawnOrigin = GetOwner()->GetActorLocation();

	Stages.Sort([](const FBossCeilingStage& A, const FBossCeilingStage& B) { return A.HealthThreshold > B.HealthThreshold; });

	if (BreakPoints.Num() == 0 && bAutoCollectBreakPoints)
	{
		for (TActorIterator<ABossCeilingBreakPoint> It(GetWorld()); It; ++It)
		{
			BreakPoints.Add(*It);
		}
	}

	if (LightShaftClass && ShaftPoolPrewarm > 0)
	{
		if (UActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>())
		{
			Pool->InitializePool(LightShaftClass, ShaftPoolPrewarm - Pool->GetInactiveCount(LightShaftClass));
		}
	}
}

void UBossCeilingBreakComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		for (FTimerHandle& Handle : ShaftTimers)
		{
			TimerManager.ClearTimer(Handle);
		}
		for (FTimerHandle& Handle : CollapseTimers)
		{
			TimerManager.ClearTimer(Handle);
		}
	}

	ShaftTimers.Reset();
	CollapseTimers.Reset();

	Super::EndPlay(EndPlayReason);
}

void UBossCeilingBreakComponent::EvaluateHealth(const float HealthPercent)
{
	if (bBossDead) return;

	while (Stages.IsValidIndex(NextStageToQueue) && HealthPercent <= Stages[NextStageToQueue].HealthThreshold)
	{
		PendingStages.Add(NextStageToQueue++);
	}
}

bool UBossCeilingBreakComponent::ExecutePendingStage(AActor* Target)
{
	if (!HasPendingStage()) return false;

	const int32 StageIndex = PendingStages[0];
	PendingStages.RemoveAt(0);

	bExecutingStage = true;
	bStageEventFired = false;
	ExecutingStageIndex = StageIndex;

	const TWeakObjectPtr<AActor> WeakTarget = Target;

	UBossCombatComponent* Combat = GetOwner()->FindComponentByClass<UBossCombatComponent>();
	if (!Combat)
	{
		HandleBreakEvent(StageIndex, WeakTarget);
		HandleBreakFinished(false);
		return true;
	}

	const bool bStarted = Combat->PlayActionMontage(BreakMontage, BreakPlayRate, BreakEventTime,
		FSimpleDelegate::CreateUObject(this, &UBossCeilingBreakComponent::HandleBreakEvent, StageIndex, WeakTarget),
		FBossActionFinished::CreateUObject(this, &UBossCeilingBreakComponent::HandleBreakFinished), Target, RecoveryAfterBreak);

	if (!bStarted)
	{
		bExecutingStage = false;
		PendingStages.Insert(StageIndex, 0);
	}

	return bStarted;
}

void UBossCeilingBreakComponent::HandleBreakEvent(const int32 StageIndex, const TWeakObjectPtr<AActor> Target)
{
	if (bBossDead) return;

	bStageEventFired = true;
	BreakCeilingNow(StageIndex, Target.Get());
}

void UBossCeilingBreakComponent::HandleBreakFinished(const bool bInterrupted)
{
	bExecutingStage = false;

	if (!bStageEventFired && !bBossDead && Stages.IsValidIndex(ExecutingStageIndex))
	{
		PendingStages.Insert(ExecutingStageIndex, 0);
	}

	ExecutingStageIndex = INDEX_NONE;
}

void UBossCeilingBreakComponent::BreakCeilingNow(const int32 StageIndex, AActor* Target)
{
	if (!Stages.IsValidIndex(StageIndex)) return;

	AActor* Owner = GetOwner();
	if (!Owner) return;

	const FBossCeilingStage Stage = Stages[StageIndex];
	const FVector BossLocation = Owner->GetActorLocation();

	if (BreakCameraShake)
	{
		UGameplayStatics::PlayWorldCameraShake(this, BreakCameraShake, BossLocation, 0.f, BreakCameraShakeRadius);
	}

	if (BreakSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, BreakSound, BossLocation);
	}

	if (BreakEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, BreakEffect, BossLocation);
	}

	ApplyShockwave();

	if (Stage.bFullArenaCollapse)
	{
		ExecuteFullArenaCollapse();
	}
	else
	{
		TArray<FShaftRequest> Requests;
		BuildShaftRequests(StageIndex, Target, Requests);

		for (int32 Index = 0; Index < Requests.Num(); ++Index)
		{
			const float Delay = ShaftStagger * Index;
			if (Delay <= 0.f)
			{
				OpenShaft(Requests[Index]);
				continue;
			}

			FTimerHandle Handle;
			GetWorld()->GetTimerManager().SetTimer(Handle,
				FTimerDelegate::CreateUObject(this, &UBossCeilingBreakComponent::OpenShaft, Requests[Index]), Delay, false);
			ShaftTimers.Add(Handle);
		}
	}

	CompletedStages = FMath::Max(CompletedStages, StageIndex + 1);

	if (UBossCombatComponent* Combat = Owner->FindComponentByClass<UBossCombatComponent>())
	{
		Combat->SetStage(CompletedStages);
	}

	UBossSummonComponent* Summon = Owner->FindComponentByClass<UBossSummonComponent>();
	if (Summon)
	{
		Summon->SetCurrentStage(CompletedStages);
	}

	if (Stage.bSummonMinions && Summon)
	{
		Summon->SpawnMinionsNow(Target, Stage.MinionCountOverride);
	}

	if (Stage.bActivateSolarShield)
	{
		if (ABossEnemy* Boss = Cast<ABossEnemy>(Owner))
		{
			Boss->ActivateSolarShield(Stage.SolarShieldAmountOverride);
		}
	}

	OnStageChanged.Broadcast(CompletedStages);
}

void UBossCeilingBreakComponent::ExecuteFullArenaCollapse()
{
	AActor* Owner = GetOwner();
	UWorld* World = GetWorld();

	for (AActor* CeilingPiece : ExtraCeilingPiecesToHideOnFullCollapse)
	{
		if (IsValid(CeilingPiece))
		{
			CeilingPiece->SetActorHiddenInGame(true);
			CeilingPiece->SetActorEnableCollision(false);
		}
	}

	if (bClosePreviousShaftsOnFullCollapse)
	{
		if (World)
		{
			FTimerManager& TimerManager = World->GetTimerManager();
			for (FTimerHandle& Handle : ShaftTimers)
			{
				TimerManager.ClearTimer(Handle);
			}
		}
		ShaftTimers.Reset();
		CloseAllShafts();
	}

	int32 CollapseIndex = 0;
	for (ABossCeilingBreakPoint* Point : BreakPoints)
	{
		if (!IsValid(Point) || Point->IsBroken()) continue;

		const float Delay = FullCollapseBreakPointStagger * CollapseIndex;
		++CollapseIndex;

		if (Delay <= 0.f || !World)
		{
			Point->Break();
			continue;
		}

		FTimerHandle Handle;
		const TWeakObjectPtr<ABossCeilingBreakPoint> WeakPoint = Point;
		World->GetTimerManager().SetTimer(Handle,
			FTimerDelegate::CreateUObject(this, &UBossCeilingBreakComponent::BreakPointDeferred, WeakPoint), Delay, false);
		CollapseTimers.Add(Handle);
	}

	const FVector Center = ResolveArenaCenter();
	float FloorZ = Center.Z;

	if (World)
	{
		FCollisionQueryParams Params(SCENE_QUERY_STAT(BossFullCollapseTrace), false, Owner);
		for (const ABossCeilingBreakPoint* Point : BreakPoints)
		{
			if (IsValid(Point))
			{
				Params.AddIgnoredActor(Point);
			}
		}

		FHitResult FloorHit;
		const FVector TraceStart(Center.X, Center.Y, Center.Z + 200.f);
		const FVector TraceEnd(Center.X, Center.Y, Center.Z - MaxCeilingTraceHeight);
		if (World->LineTraceSingleByChannel(FloorHit, TraceStart, TraceEnd, ECC_WorldStatic, Params))
		{
			FloorZ = FloorHit.ImpactPoint.Z;
		}
	}

	const FVector Floor(Center.X, Center.Y, FloorZ);
	const FVector Top(Center.X, Center.Y, FloorZ + OpenSkyShaftHeight);
	const float Radius = FullCollapseShaftRadius > 0.f ? FullCollapseShaftRadius : ArenaRadius;

	if (LightShaftClass && World)
	{
		AActor* Spawned = nullptr;
		if (UActorPoolSubsystem* Pool = World->GetSubsystem<UActorPoolSubsystem>())
		{
			Spawned = Pool->GetActorFromPool(LightShaftClass, FTransform(Top));
		}

		ABossLightShaft* Shaft = Cast<ABossLightShaft>(Spawned);
		if (!Shaft)
		{
			FActorSpawnParameters SpawnParams;
			SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
			Shaft = World->SpawnActor<ABossLightShaft>(LightShaftClass, Top, FRotator::ZeroRotator, SpawnParams);
		}

		if (Shaft)
		{
			Shaft->OpenFullArenaShaft(Top, Floor, Radius, Owner, GetShaftDamageParams());
			ActiveShafts.RemoveAll([](const TWeakObjectPtr<ABossLightShaft>& Existing) { return !Existing.IsValid() || Existing->IsHidden(); });
			ActiveShafts.Add(Shaft);
		}
	}

	OnFullArenaCollapsed.Broadcast();
}

void UBossCeilingBreakComponent::BreakPointDeferred(const TWeakObjectPtr<ABossCeilingBreakPoint> Point)
{
	if (ABossCeilingBreakPoint* BreakPoint = Point.Get())
	{
		if (!BreakPoint->IsBroken())
		{
			BreakPoint->Break();
		}
	}
}

void UBossCeilingBreakComponent::ApplyShockwave() const
{
	if (ShockwaveDamage <= 0.f || ShockwaveRadius <= 0.f) return;

	AActor* Owner = GetOwner();
	const FVector Center = Owner->GetActorLocation();

	FBossDamageParams Params = GetShaftDamageParams();
	Params.Damage = ShockwaveDamage;
	Params.DamageTypeTag = FGameplayTag::RequestGameplayTag(FName("Damage.Impact"), false);

	TArray<FOverlapResult> Overlaps;
	GetWorld()->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeSphere(ShockwaveRadius), FCollisionQueryParams(SCENE_QUERY_STAT(BossShockwave), false, Owner));

	TArray<AActor*> Damaged;
	for (const FOverlapResult& Overlap : Overlaps)
	{
		AActor* Target = Overlap.GetActor();
		if (!Target || Damaged.Contains(Target)) continue;
		if (!UBossSystemLibrary::IsValidBossTarget(Owner, Target, Params.bOnlyPlayerControlled)) continue;
		if (UBossSystemLibrary::IsImmuneToBossHit(Target, Params.ImmunityTags)) continue;

		Damaged.Add(Target);

		const FVector TargetLocation = Target->GetActorLocation();
		const FHitResult Hit(Target, Overlap.GetComponent(), TargetLocation, (TargetLocation - Center).GetSafeNormal());
		UBossSystemLibrary::ApplyBossDamage(Owner, Owner, Target, Params, 1.f, Hit);
	}
}

void UBossCeilingBreakComponent::OpenShaft(const FShaftRequest Request)
{
	if (!LightShaftClass) return;

	AActor* Spawned = nullptr;
	if (UActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>())
	{
		Spawned = Pool->GetActorFromPool(LightShaftClass, FTransform(Request.Top));
	}

	ABossLightShaft* Shaft = Cast<ABossLightShaft>(Spawned);
	if (!Shaft) return;

	Shaft->OpenShaft(GetOwner(), Request.Top, Request.Floor, Request.Radius, GetShaftDamageParams(), Request.Point.Get());

	ActiveShafts.RemoveAll([](const TWeakObjectPtr<ABossLightShaft>& Existing) { return !Existing.IsValid() || Existing->IsHidden(); });
	ActiveShafts.Add(Shaft);
}

void UBossCeilingBreakComponent::BuildShaftRequests(const int32 StageIndex, const AActor* Target, TArray<FShaftRequest>& OutRequests)
{
	const FBossCeilingStage& Stage = Stages[StageIndex];
	int32 Remaining = Stage.ShaftCount;

	if (Stage.bOpenShaftOnTarget && Target && Remaining > 0)
	{
		FShaftRequest Request;
		if (ResolveShaftFromGround(Target->GetActorLocation(), Target, Request) && IsFarFromExisting(Request.Floor, OutRequests))
		{
			OutRequests.Add(Request);
			--Remaining;
		}
	}

	for (int32 Pass = 0; Pass < 2 && Remaining > 0; ++Pass)
	{
		TArray<ABossCeilingBreakPoint*> Candidates;
		for (ABossCeilingBreakPoint* Point : BreakPoints)
		{
			if (!IsValid(Point) || !Point->CanBreakAtStage(StageIndex)) continue;

			const bool bExactStage = Point->StageIndex == StageIndex;
			if ((Pass == 0) == bExactStage)
			{
				Candidates.Add(Point);
			}
		}

		for (int32 Index = Candidates.Num() - 1; Index > 0; --Index)
		{
			Candidates.Swap(Index, FMath::RandRange(0, Index));
		}

		for (ABossCeilingBreakPoint* Point : Candidates)
		{
			if (Remaining <= 0) break;

			FShaftRequest Request;
			if (!ResolveShaftFromCeiling(Point->GetActorLocation(), Request)) continue;
			if (!IsFarFromExisting(Request.Floor, OutRequests)) continue;

			Request.Point = Point;
			if (Point->ShaftRadiusOverride > 0.f)
			{
				Request.Radius = Point->ShaftRadiusOverride;
			}

			OutRequests.Add(Request);
			--Remaining;
		}
	}

	for (int32 Attempt = 0; Remaining > 0 && Attempt < Stage.ShaftCount * 4; ++Attempt)
	{
		FVector GroundPoint;
		if (!FindRandomGroundPoint(OutRequests, GroundPoint)) continue;

		FShaftRequest Request;
		if (ResolveShaftFromGround(GroundPoint, nullptr, Request))
		{
			OutRequests.Add(Request);
			--Remaining;
		}
	}
}

bool UBossCeilingBreakComponent::ResolveShaftFromGround(const FVector& GroundPoint, const AActor* IgnoredActor, FShaftRequest& OutRequest) const
{
	UWorld* World = GetWorld();

	FCollisionQueryParams Params(SCENE_QUERY_STAT(BossCeilingTrace), false, GetOwner());
	if (IgnoredActor)
	{
		Params.AddIgnoredActor(IgnoredActor);

		TArray<AActor*> AttachedActors;
		IgnoredActor->GetAttachedActors(AttachedActors, true, true);
		Params.AddIgnoredActors(AttachedActors);
	}

	FVector Floor = GroundPoint;
	FHitResult FloorHit;
	if (World->LineTraceSingleByChannel(FloorHit, GroundPoint + FVector(0.f, 0.f, 50.f), GroundPoint - FVector(0.f, 0.f, 1000.f), CeilingTraceChannel, Params))
	{
		Floor = FloorHit.ImpactPoint;
	}

	FVector Top = Floor + FVector(0.f, 0.f, OpenSkyShaftHeight);
	FHitResult CeilingHit;
	if (World->LineTraceSingleByChannel(CeilingHit, Floor + FVector(0.f, 0.f, 120.f), Floor + FVector(0.f, 0.f, MaxCeilingTraceHeight), CeilingTraceChannel, Params))
	{
		Top = FVector(CeilingHit.ImpactPoint) - FVector(0.f, 0.f, 5.f);
	}

	if (Top.Z - Floor.Z < BossCeiling::MinShaftHeight) return false;

	OutRequest.Top = Top;
	OutRequest.Floor = Floor;
	OutRequest.Radius = ShaftRadius;
	return true;
}

bool UBossCeilingBreakComponent::ResolveShaftFromCeiling(const FVector& CeilingPoint, FShaftRequest& OutRequest) const
{
	const FCollisionQueryParams Params(SCENE_QUERY_STAT(BossCeilingTrace), false, GetOwner());

	const FVector Start = CeilingPoint - FVector(0.f, 0.f, 20.f);
	FVector Floor = CeilingPoint - FVector(0.f, 0.f, OpenSkyShaftHeight);

	FHitResult FloorHit;
	if (GetWorld()->LineTraceSingleByChannel(FloorHit, Start, Start - FVector(0.f, 0.f, MaxCeilingTraceHeight), CeilingTraceChannel, Params))
	{
		Floor = FloorHit.ImpactPoint;
	}

	if (CeilingPoint.Z - Floor.Z < BossCeiling::MinShaftHeight) return false;

	OutRequest.Top = CeilingPoint;
	OutRequest.Floor = Floor;
	OutRequest.Radius = ShaftRadius;
	return true;
}

bool UBossCeilingBreakComponent::FindRandomGroundPoint(const TArray<FShaftRequest>& Existing, FVector& OutPoint) const
{
	const FVector Origin = GetArenaOrigin();
	const FVector BossLocation = GetOwner()->GetActorLocation();
	const UNavigationSystemV1* NavSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(GetWorld());

	for (int32 Attempt = 0; Attempt < BossCeiling::RandomPointAttempts; ++Attempt)
	{
		FVector Candidate;

		if (NavSystem)
		{
			FNavLocation NavLocation;
			if (!NavSystem->GetRandomReachablePointInRadius(Origin, ArenaRadius, NavLocation)) continue;
			Candidate = NavLocation.Location;
		}
		else
		{
			Candidate = Origin + FRotator(0.f, FMath::FRandRange(0.f, 360.f), 0.f).Vector() * FMath::FRandRange(0.f, ArenaRadius);
		}

		if (FVector::Dist2D(Candidate, BossLocation) < MinDistanceFromBoss) continue;
		if (!IsFarFromExisting(Candidate, Existing)) continue;

		OutPoint = Candidate;
		return true;
	}

	return false;
}

bool UBossCeilingBreakComponent::IsFarFromExisting(const FVector& Point, const TArray<FShaftRequest>& Existing) const
{
	for (const FShaftRequest& Request : Existing)
	{
		if (FVector::Dist2D(Request.Floor, Point) < MinShaftSeparation) return false;
	}

	for (const TWeakObjectPtr<ABossLightShaft>& Shaft : ActiveShafts)
	{
		if (Shaft.IsValid() && !Shaft->IsHidden() && FVector::Dist2D(Shaft->GetFloorLocation(), Point) < MinShaftSeparation) return false;
	}

	return true;
}

FBossDamageParams UBossCeilingBreakComponent::GetShaftDamageParams() const
{
	if (const AActor* Owner = GetOwner())
	{
		if (const UBossCombatComponent* Combat = Owner->FindComponentByClass<UBossCombatComponent>())
		{
			return Combat->DamageParams;
		}
	}

	FBossDamageParams Params;
	Params.SetByCallerTag = FGameplayTag::RequestGameplayTag(FName("Damage"), false);
	return Params;
}

FVector UBossCeilingBreakComponent::ResolveArenaCenter() const
{
	if (IsValid(ArenaCenter))
	{
		return ArenaCenter->GetActorLocation();
	}

	FVector2D SumXY = FVector2D::ZeroVector;
	int32 ValidCount = 0;

	for (const ABossCeilingBreakPoint* Point : BreakPoints)
	{
		if (!IsValid(Point)) continue;

		const FVector Location = Point->GetActorLocation();
		SumXY.X += Location.X;
		SumXY.Y += Location.Y;
		++ValidCount;
	}

	if (ValidCount > 0)
	{
		return FVector(SumXY.X / ValidCount, SumXY.Y / ValidCount, SpawnOrigin.Z);
	}

	return SpawnOrigin;
}

FVector UBossCeilingBreakComponent::GetArenaOrigin() const
{
	return IsValid(ArenaCenter) ? ArenaCenter->GetActorLocation() : SpawnOrigin;
}

void UBossCeilingBreakComponent::CloseAllShafts()
{
	for (const TWeakObjectPtr<ABossLightShaft>& Shaft : ActiveShafts)
	{
		if (Shaft.IsValid())
		{
			Shaft->CloseShaft();
		}
	}
	ActiveShafts.Reset();
}

void UBossCeilingBreakComponent::OnBossDied()
{
	bBossDead = true;
	PendingStages.Reset();

	if (UWorld* World = GetWorld())
	{
		FTimerManager& TimerManager = World->GetTimerManager();
		for (FTimerHandle& Handle : CollapseTimers)
		{
			TimerManager.ClearTimer(Handle);
		}
		CollapseTimers.Reset();

		if (bCloseShaftsOnBossDeath)
		{
			for (FTimerHandle& Handle : ShaftTimers)
			{
				TimerManager.ClearTimer(Handle);
			}
			ShaftTimers.Reset();

			CloseAllShafts();
		}
	}
	else
	{
		CollapseTimers.Reset();
		if (bCloseShaftsOnBossDeath)
		{
			ShaftTimers.Reset();
			CloseAllShafts();
		}
	}
}
