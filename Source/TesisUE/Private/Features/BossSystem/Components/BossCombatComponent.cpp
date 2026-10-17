#include "Features/BossSystem/Components/BossCombatComponent.h"

#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Components/CapsuleComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Features/BossSystem/Actors/BossProjectile.h"
#include "Features/BossSystem/Animation/AnimNotify_BossActionEvent.h"
#include "Features/BossSystem/Animation/AnimNotifyState_BossHitWindow.h"
#include "Features/BossSystem/Core/BossSystemLibrary.h"
#include "GameFramework/Character.h"
#include "GenericPool/Core/ActorPoolSubsystem.h"
#include "Kismet/GameplayStatics.h"

DEFINE_LOG_CATEGORY_STATIC(LogBossCombat, Log, All);

namespace BossCombat
{
	constexpr uint8 WindowPending = 0;
	constexpr uint8 WindowActive = 1;
	constexpr uint8 WindowDone = 2;

	// Duración de una acción cuyo montaje no se pudo reproducir (para que igual ocurra su evento).
	constexpr float NoMontageDuration = 0.6f;

	// El root motion escalado se queda corto por esta distancia para no meterse dentro del objetivo.
	constexpr float RootMotionStopShort = 120.f;
}

UBossCombatComponent::UBossCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false;
	// Después de la animación y el movimiento, para barrer el arma en su posición final del frame.
	PrimaryComponentTick.TickGroup = TG_PostPhysics;

	TargetObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	DamageParams.SetByCallerTag = FGameplayTag::RequestGameplayTag(FName("Damage"), false);
	DamageParams.DamageTypeTag = FGameplayTag::RequestGameplayTag(FName("Damage.Directional"), false);
	DamageParams.CueTag = FGameplayTag::RequestGameplayTag(FName("GameplayCue.Damage.Default"), false);

	if (const FGameplayTag DodgeTag = FGameplayTag::RequestGameplayTag(FName("State.Dodging"), false); DodgeTag.IsValid())
	{
		DamageParams.ImmunityTags.AddTag(DodgeTag);
	}
}

void UBossCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	OwnerCharacter = Cast<ACharacter>(GetOwner());
	AttackReadyTimes.Init(0.0, Attacks.Num());

	if (ProjectileClass && ProjectilePoolPrewarm > 0)
	{
		if (UActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>())
		{
			Pool->InitializePool(ProjectileClass, ProjectilePoolPrewarm - Pool->GetInactiveCount(ProjectileClass));
		}
	}
}

void UBossCombatComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	CurrentAction = FBossActionRuntime();

	if (IsValid(ActiveTelegraphDecal))
	{
		ActiveTelegraphDecal->DestroyComponent();
	}
	ActiveTelegraphDecal = nullptr;

	Super::EndPlay(EndPlayReason);
}

void UBossCombatComponent::SetWeaponComponent(UPrimitiveComponent* InWeaponComponent)
{
	WeaponComponent = InWeaponComponent;
}

int32 UBossCombatComponent::SelectAttack(const AActor* Target, const int32 Stage) const
{
	const AActor* Owner = GetOwner();
	if (!Owner || !Target) return INDEX_NONE;

	const double Now = GetWorld()->GetTimeSeconds();

	const FVector ToTarget = Target->GetActorLocation() - Owner->GetActorLocation();
	const float Distance = ToTarget.Size2D();
	const float Cos = FVector::DotProduct(Owner->GetActorForwardVector().GetSafeNormal2D(), ToTarget.GetSafeNormal2D());
	const float Angle = FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(Cos, -1.f, 1.f)));

	TArray<int32, TInlineAllocator<16>> Candidates;
	float TotalWeight = 0.f;

	for (int32 Index = 0; Index < Attacks.Num(); ++Index)
	{
		const FBossAttackDefinition& Attack = Attacks[Index];

		if (!Attack.Montage || Attack.Weight <= 0.f) continue;
		if (Stage < Attack.MinStage) continue;
		if (Distance < Attack.MinDistance || Distance > Attack.MaxDistance) continue;
		if (Angle < Attack.MinAngle || Angle > Attack.MaxAngle) continue;
		if (AttackReadyTimes.IsValidIndex(Index) && Now < AttackReadyTimes[Index]) continue;

		Candidates.Add(Index);
		TotalWeight += Attack.Weight;
	}

	if (Candidates.Num() == 0) return INDEX_NONE;

	float Pick = FMath::FRandRange(0.f, TotalWeight);
	for (const int32 Index : Candidates)
	{
		Pick -= Attacks[Index].Weight;
		if (Pick <= 0.f) return Index;
	}

	return Candidates.Last();
}

bool UBossCombatComponent::StartAttack(const int32 AttackIndex, AActor* Target)
{
	if (!Attacks.IsValidIndex(AttackIndex) || !Attacks[AttackIndex].Montage) return false;

	if (AttackReadyTimes.Num() != Attacks.Num())
	{
		AttackReadyTimes.SetNumZeroed(Attacks.Num());
	}

	const FBossAttackDefinition& Attack = Attacks[AttackIndex];
	AttackReadyTimes[AttackIndex] = GetWorld()->GetTimeSeconds() + Attack.Cooldown;

	const float PlayRate = Attack.PlayRate * (1.f + PlayRateBonusPerStage * CurrentStage);

	// Un ataque sin animación sería un golpe invisible: si el montaje no corre, no se ataca.
	if (!BeginAction(Attack.Montage, PlayRate, Target, false)) return false;

	CurrentAction.AttackIndex = AttackIndex;
	CurrentAction.TrackingEndTime = Attack.TrackingEndTime;
	CurrentAction.TrackingSpeed = Attack.TrackingSpeed;
	CurrentAction.RecoveryAfter = Attack.RecoveryTime * FMath::Max(0.1f, 1.f - RecoveryReductionPerStage * CurrentStage);
	CurrentAction.TimedWindowStates.Init(BossCombat::WindowPending, Attack.HitWindows.Num());

	if (Attack.bScaleRootMotionToTarget && Target && OwnerCharacter)
	{
		const float Distance = FVector::Dist2D(OwnerCharacter->GetActorLocation(), Target->GetActorLocation()) - BossCombat::RootMotionStopShort;
		OwnerCharacter->SetAnimRootMotionTranslationScale(FMath::Clamp(Distance / Attack.RootMotionNominalDistance, 0.25f, 2.5f));
	}

	float FirstHitProgress = 1.f;
	for (const FBossHitWindow& Window : Attack.HitWindows)
	{
		FirstHitProgress = FMath::Min(FirstHitProgress, Window.StartTime);
	}
	const float TimeUntilFirstHit = FirstHitProgress * Attack.Montage->GetPlayLength() / PlayRate;

	if (Attack.bShowGroundTelegraph && GroundTelegraphMaterial && OwnerCharacter)
	{
		const float HalfHeight = OwnerCharacter->GetCapsuleComponent()->GetScaledCapsuleHalfHeight();
		const FVector DecalOffset = Attack.TelegraphOffset - FVector(0.f, 0.f, HalfHeight);

		ActiveTelegraphDecal = UGameplayStatics::SpawnDecalAttached(GroundTelegraphMaterial,
			FVector(150.f, Attack.TelegraphRadius, Attack.TelegraphRadius), OwnerCharacter->GetRootComponent(), NAME_None,
			DecalOffset, FRotator(-90.f, 0.f, 0.f), EAttachLocation::KeepRelativeOffset, FMath::Max(TimeUntilFirstHit, 0.2f));
	}

	if (Attack.TelegraphSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, Attack.TelegraphSound, GetOwner()->GetActorLocation());
	}

	OnAttackStarted.Broadcast(Attack.AttackName, TimeUntilFirstHit);
	return true;
}

bool UBossCombatComponent::PlayActionMontage(UAnimMontage* Montage, const float PlayRate, const float EventTime, FSimpleDelegate OnEvent,
	FBossActionFinished OnFinished, AActor* TrackTarget, const float RecoveryAfter)
{
	if (!BeginAction(Montage, PlayRate, TrackTarget, true)) return false;

	CurrentAction.EventTime = EventTime;
	CurrentAction.OnEvent = MoveTemp(OnEvent);
	CurrentAction.OnFinished = MoveTemp(OnFinished);
	CurrentAction.TrackingEndTime = EventTime >= 0.f ? EventTime : 0.3f;
	CurrentAction.TrackingSpeed = TrackTarget ? 180.f : 0.f;
	CurrentAction.RecoveryAfter = RecoveryAfter;

	return true;
}

bool UBossCombatComponent::BeginAction(UAnimMontage* Montage, const float PlayRate, AActor* Target, const bool bAllowWithoutMontage)
{
	CancelCurrentAction();

	CurrentAction = FBossActionRuntime();
	CurrentAction.Id = NextActionId++;
	CurrentAction.Target = Target;
	CurrentAction.StartTime = GetWorld()->GetTimeSeconds();

	UAnimInstance* AnimInstance = GetAnimInstance();
	if (Montage && AnimInstance && AnimInstance->Montage_Play(Montage, PlayRate) > 0.f)
	{
		CurrentAction.Montage = Montage;

		FOnMontageEnded EndDelegate = FOnMontageEnded::CreateUObject(this, &UBossCombatComponent::HandleMontageEnded, CurrentAction.Id);
		AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);

		for (const FAnimNotifyEvent& NotifyEvent : Montage->Notifies)
		{
			if (NotifyEvent.NotifyStateClass && NotifyEvent.NotifyStateClass->IsA<UAnimNotifyState_BossHitWindow>())
			{
				CurrentAction.bUseNotifyWindows = true;
			}
			if (NotifyEvent.Notify && NotifyEvent.Notify->IsA<UAnimNotify_BossActionEvent>())
			{
				CurrentAction.bUseNotifyEvent = true;
			}
		}
	}
	else
	{
		if (Montage)
		{
			UE_LOG(LogBossCombat, Warning, TEXT("%s: no se pudo reproducir %s (revisá que el skeleton coincida y que el AnimBP tenga el slot DefaultSlot)."),
				*GetNameSafe(GetOwner()), *GetNameSafe(Montage));
		}

		if (!bAllowWithoutMontage) return false;

		CurrentAction.FallbackDuration = BossCombat::NoMontageDuration / FMath::Max(PlayRate, 0.1f);
	}

	CurrentAction.bActive = true;
	PrevWeaponSamples.Reset();
	bHasPrevWeaponSamples = false;
	SetComponentTickEnabled(true);

	return true;
}

void UBossCombatComponent::CancelCurrentAction()
{
	if (!CurrentAction.bActive) return;

	if (UAnimMontage* Montage = CurrentAction.Montage.Get())
	{
		if (UAnimInstance* AnimInstance = GetAnimInstance(); AnimInstance && AnimInstance->Montage_IsActive(Montage))
		{
			AnimInstance->Montage_Stop(0.2f, Montage);
		}
	}

	FinishAction(true);
}

void UBossCombatComponent::HandleMontageEnded(UAnimMontage* Montage, const bool bInterrupted, const int32 ActionId)
{
	// Montajes de acciones anteriores (canceladas o reemplazadas) llegan tarde y se ignoran.
	if (!CurrentAction.bActive || CurrentAction.Id != ActionId) return;

	FinishAction(bInterrupted);
}

void UBossCombatComponent::FinishAction(const bool bInterrupted)
{
	if (!CurrentAction.bActive) return;

	if (!bInterrupted && CurrentAction.EventTime >= 0.f && !CurrentAction.bEventFired)
	{
		FireActionEvent();
		if (!CurrentAction.bActive) return;
	}

	const bool bWasAttack = CurrentAction.AttackIndex != INDEX_NONE;
	const FName AttackName = GetCurrentAttackName();
	const FBossActionFinished OnFinished = CurrentAction.OnFinished;

	LastRecoveryTime = CurrentAction.RecoveryAfter;

	CurrentAction.bActive = false;
	CurrentAction.Windows.Reset();
	CurrentAction.OnEvent.Unbind();
	CurrentAction.OnFinished.Unbind();
	bHasPrevWeaponSamples = false;
	SetComponentTickEnabled(false);

	if (OwnerCharacter)
	{
		OwnerCharacter->SetAnimRootMotionTranslationScale(1.f);
	}

	if (IsValid(ActiveTelegraphDecal))
	{
		ActiveTelegraphDecal->DestroyComponent();
	}
	ActiveTelegraphDecal = nullptr;

	// Los callbacks pueden arrancar otra acción, por eso van al final.
	if (bWasAttack)
	{
		OnAttackEnded.Broadcast(AttackName, bInterrupted);
	}
	OnFinished.ExecuteIfBound(bInterrupted);
	OnActionFinished.Broadcast(bInterrupted);
}

void UBossCombatComponent::FireActionEvent()
{
	if (!CurrentAction.bActive || CurrentAction.bEventFired) return;

	CurrentAction.bEventFired = true;

	const FSimpleDelegate Event = CurrentAction.OnEvent;
	Event.ExecuteIfBound();
}

void UBossCombatComponent::TriggerActionEventFromNotify()
{
	FireActionEvent();
}

void UBossCombatComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (!CurrentAction.bActive)
	{
		SetComponentTickEnabled(false);
		return;
	}

	const int32 ActionId = CurrentAction.Id;
	const float Progress = GetActionProgress();

	if (const AActor* Target = CurrentAction.Target.Get(); Target && CurrentAction.TrackingSpeed > 0.f && Progress <= CurrentAction.TrackingEndTime)
	{
		RotateOwnerTowards(Target->GetActorLocation(), DeltaTime, CurrentAction.TrackingSpeed);
	}

	if (CurrentAction.EventTime >= 0.f && !CurrentAction.bUseNotifyEvent && Progress >= CurrentAction.EventTime)
	{
		FireActionEvent();
		if (!CurrentAction.bActive || CurrentAction.Id != ActionId) return;
	}

	if (CurrentAction.AttackIndex != INDEX_NONE && !CurrentAction.bUseNotifyWindows)
	{
		UpdateTimedWindows(Progress);
	}

	ProcessActiveWindows(DeltaTime);
	if (!CurrentAction.bActive || CurrentAction.Id != ActionId) return;

	if (UAnimMontage* Montage = CurrentAction.Montage.Get())
	{
		// Si el montaje se cortó sin avisar (ej. cambio de AnimInstance), la acción no puede quedar colgada.
		if (const UAnimInstance* AnimInstance = GetAnimInstance(); !AnimInstance || !AnimInstance->Montage_IsActive(Montage))
		{
			FinishAction(false);
		}
	}
	else if (Progress >= 1.f)
	{
		FinishAction(false);
	}
}

float UBossCombatComponent::GetActionProgress() const
{
	if (const UAnimMontage* Montage = CurrentAction.Montage.Get())
	{
		const UAnimInstance* AnimInstance = GetAnimInstance();
		const float Length = Montage->GetPlayLength();

		if (AnimInstance && Length > 0.f && AnimInstance->Montage_IsActive(Montage))
		{
			return FMath::Clamp(AnimInstance->Montage_GetPosition(Montage) / Length, 0.f, 1.f);
		}
		return 1.f;
	}

	if (CurrentAction.FallbackDuration <= 0.f) return 1.f;

	const double Elapsed = GetWorld()->GetTimeSeconds() - CurrentAction.StartTime;
	return FMath::Clamp(static_cast<float>(Elapsed) / CurrentAction.FallbackDuration, 0.f, 1.f);
}

void UBossCombatComponent::UpdateTimedWindows(const float Progress)
{
	if (!Attacks.IsValidIndex(CurrentAction.AttackIndex)) return;

	const TArray<FBossHitWindow>& Definitions = Attacks[CurrentAction.AttackIndex].HitWindows;
	const int32 Count = FMath::Min(Definitions.Num(), CurrentAction.TimedWindowStates.Num());

	for (int32 Index = 0; Index < Count; ++Index)
	{
		const FBossHitWindow& Definition = Definitions[Index];
		uint8& State = CurrentAction.TimedWindowStates[Index];

		// Primero se cierran las que venían activas: una ventana que abre este frame se procesa al menos una vez.
		if (State == BossCombat::WindowActive && Progress > Definition.EndTime)
		{
			CurrentAction.Windows.RemoveAll([Index](const FActiveHitWindow& Window) { return Window.TimedIndex == Index; });
			State = BossCombat::WindowDone;
		}

		if (State == BossCombat::WindowPending && Progress >= Definition.StartTime)
		{
			if (Definition.Shape == EBossHitShape::Projectile)
			{
				FireProjectiles(Definition);
				State = BossCombat::WindowDone;
				continue;
			}

			FActiveHitWindow& Window = CurrentAction.Windows.AddDefaulted_GetRef();
			Window.Params = Definition;
			Window.TimedIndex = Index;
			State = BossCombat::WindowActive;
		}
	}
}

void UBossCombatComponent::BeginNotifyWindow(const UObject* NotifyKey, const FBossHitWindow& Window)
{
	if (!CurrentAction.bActive) return;

	if (Window.Shape == EBossHitShape::Projectile)
	{
		FireProjectiles(Window);
		return;
	}

	FActiveHitWindow& Active = CurrentAction.Windows.AddDefaulted_GetRef();
	Active.Params = Window;
	Active.NotifyKey = NotifyKey;
}

void UBossCombatComponent::EndNotifyWindow(const UObject* NotifyKey)
{
	CurrentAction.Windows.RemoveAll([NotifyKey](const FActiveHitWindow& Window) { return Window.NotifyKey == NotifyKey; });
}

void UBossCombatComponent::ProcessActiveWindows(const float DeltaTime)
{
	if (CurrentAction.Windows.Num() == 0)
	{
		bHasPrevWeaponSamples = false;
		return;
	}

	UWorld* World = GetWorld();
	AActor* Owner = GetOwner();
	if (!World || !Owner) return;

	FCollisionObjectQueryParams ObjectParams;
	for (const TEnumAsByte<EObjectTypeQuery>& ObjectType : TargetObjectTypes)
	{
		ObjectParams.AddObjectTypesToQuery(UEngineTypes::ConvertToCollisionChannel(ObjectType));
	}
	if (!ObjectParams.IsValid())
	{
		ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
	}

	const FCollisionQueryParams QueryParams(SCENE_QUERY_STAT(BossAttack), false, Owner);

	TArray<FHitResult> WeaponHits;
	const bool bHasWeaponWindow = CurrentAction.Windows.ContainsByPredicate([](const FActiveHitWindow& Window)
	{
		return Window.Params.Shape == EBossHitShape::Weapon;
	});

	if (bHasWeaponWindow)
	{
		TArray<FVector> Samples;
		BuildWeaponSamples(Samples);

		if (Samples.Num() > 0 && bHasPrevWeaponSamples && PrevWeaponSamples.Num() == Samples.Num() && DeltaTime > KINDA_SMALL_NUMBER)
		{
			float MaxSpeed = 0.f;
			for (int32 Index = 0; Index < Samples.Num(); ++Index)
			{
				MaxSpeed = FMath::Max(MaxSpeed, FVector::Dist(PrevWeaponSamples[Index], Samples[Index]) / DeltaTime);
			}

			if (MaxSpeed >= MinWeaponSweepSpeed)
			{
				const FCollisionShape Shape = FCollisionShape::MakeSphere(WeaponTraceRadius);

				for (int32 Index = 0; Index < Samples.Num(); ++Index)
				{
					TArray<FHitResult> Hits;
					World->SweepMultiByObjectType(Hits, PrevWeaponSamples[Index], Samples[Index], FQuat::Identity, ObjectParams, Shape, QueryParams);
					WeaponHits.Append(Hits);

					if (bDrawDebug)
					{
						DrawDebugLine(World, PrevWeaponSamples[Index], Samples[Index], Hits.Num() > 0 ? FColor::Red : FColor::Orange, false, 1.f, 0, 2.f);
						DrawDebugSphere(World, Samples[Index], WeaponTraceRadius, 8, FColor::Orange, false, 1.f);
					}
				}
			}
		}

		PrevWeaponSamples = MoveTemp(Samples);
		bHasPrevWeaponSamples = PrevWeaponSamples.Num() > 0;
	}
	else
	{
		bHasPrevWeaponSamples = false;
	}

	// Aplicar daño ejecuta el ReceiveDamage del objetivo, que podría cancelar esta acción: se valida después de cada golpe.
	const int32 ActionId = CurrentAction.Id;
	auto IsStillValid = [this, ActionId](const int32 WindowIndex)
	{
		return CurrentAction.bActive && CurrentAction.Id == ActionId && CurrentAction.Windows.IsValidIndex(WindowIndex);
	};

	for (int32 WindowIndex = 0; WindowIndex < CurrentAction.Windows.Num(); ++WindowIndex)
	{
		const FBossHitWindow Params = CurrentAction.Windows[WindowIndex].Params;

		if (Params.Shape == EBossHitShape::Weapon)
		{
			for (const FHitResult& Hit : WeaponHits)
			{
				TryHitTarget(CurrentAction.Windows[WindowIndex], Hit.GetActor(), Hit);
				if (!IsStillValid(WindowIndex)) return;
			}
		}
		else if (Params.Shape == EBossHitShape::Sphere)
		{
			const FVector Center = Owner->GetActorTransform().TransformPosition(Params.SphereOffset);

			TArray<FOverlapResult> Overlaps;
			World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, ObjectParams, FCollisionShape::MakeSphere(Params.SphereRadius), QueryParams);

			if (bDrawDebug)
			{
				DrawDebugSphere(World, Center, Params.SphereRadius, 16, FColor::Red, false, 0.f);
			}

			for (const FOverlapResult& Overlap : Overlaps)
			{
				AActor* Target = Overlap.GetActor();
				if (!Target) continue;

				const FVector TargetLocation = Target->GetActorLocation();
				const FHitResult Hit(Target, Overlap.GetComponent(), TargetLocation, (TargetLocation - Center).GetSafeNormal());

				TryHitTarget(CurrentAction.Windows[WindowIndex], Target, Hit);
				if (!IsStillValid(WindowIndex)) return;
			}
		}
	}
}

void UBossCombatComponent::TryHitTarget(FActiveHitWindow& Window, AActor* Target, const FHitResult& Hit)
{
	AActor* Owner = GetOwner();
	if (!Target || !UBossSystemLibrary::IsValidBossTarget(Owner, Target, DamageParams.bOnlyPlayerControlled)) return;

	const double Now = GetWorld()->GetTimeSeconds();
	if (const double* LastHitTime = Window.LastHitTimes.Find(Target))
	{
		if (!Window.Params.bAllowMultipleHits || Now - *LastHitTime < Window.Params.ReHitDelay) return;
	}
	Window.LastHitTimes.Add(Target, Now);

	const FName AttackName = GetCurrentAttackName();

	// Un golpe esquivado no vuelve a buscar al objetivo cuando termina el dodge.
	if (UBossSystemLibrary::IsImmuneToBossHit(Target, DamageParams.ImmunityTags))
	{
		OnAttackEvaded.Broadcast(Target, AttackName);
		return;
	}

	FBossDamageParams Params = DamageParams;
	Params.Damage = GetCurrentBaseDamage() * GetStageDamageMultiplier();

	if (Window.Params.DamageTypeTag.IsValid())
	{
		Params.DamageTypeTag = Window.Params.DamageTypeTag;
	}
	else if (Attacks.IsValidIndex(CurrentAction.AttackIndex) && Attacks[CurrentAction.AttackIndex].DamageTypeTag.IsValid())
	{
		Params.DamageTypeTag = Attacks[CurrentAction.AttackIndex].DamageTypeTag;
	}

	if (UBossSystemLibrary::ApplyBossDamage(Owner, Owner, Target, Params, Window.Params.DamageMultiplier, Hit))
	{
		OnTargetHit.Broadcast(Target, AttackName);
	}
}

void UBossCombatComponent::FireProjectiles(const FBossHitWindow& Window)
{
	if (!ProjectileClass)
	{
		if (!bWarnedMissingProjectileClass)
		{
			UE_LOG(LogBossCombat, Warning, TEXT("%s: un ataque dispara proyectiles pero ProjectileClass está vacío."), *GetNameSafe(GetOwner()));
			bWarnedMissingProjectileClass = true;
		}
		return;
	}

	UActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>();
	AActor* Owner = GetOwner();
	if (!Pool || !Owner) return;

	const FVector SpawnLocation = Owner->GetActorTransform().TransformPosition(ProjectileSpawnOffset);

	FVector AimDirection = Owner->GetActorForwardVector();
	if (const AActor* Target = CurrentAction.Target.Get())
	{
		AimDirection = Target->GetActorLocation() - SpawnLocation;
	}
	if (bFlattenProjectileAim)
	{
		AimDirection.Z = 0.f;
	}
	AimDirection = AimDirection.GetSafeNormal();
	if (AimDirection.IsNearlyZero())
	{
		AimDirection = Owner->GetActorForwardVector();
	}

	FBossDamageParams Params = DamageParams;
	Params.Damage = GetCurrentBaseDamage() * GetStageDamageMultiplier() * Window.DamageMultiplier;
	if (Window.DamageTypeTag.IsValid())
	{
		Params.DamageTypeTag = Window.DamageTypeTag;
	}
	else if (Attacks.IsValidIndex(CurrentAction.AttackIndex) && Attacks[CurrentAction.AttackIndex].DamageTypeTag.IsValid())
	{
		Params.DamageTypeTag = Attacks[CurrentAction.AttackIndex].DamageTypeTag;
	}

	const int32 Count = FMath::Max(1, Window.ProjectileCount);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Alpha = Count > 1 ? static_cast<float>(Index) / (Count - 1) : 0.5f;
		const float YawOffset = FMath::Lerp(-Window.ProjectileSpreadAngle * 0.5f, Window.ProjectileSpreadAngle * 0.5f, Alpha);
		const FVector Direction = AimDirection.RotateAngleAxis(YawOffset, FVector::UpVector);

		AActor* Spawned = Pool->GetActorFromPool(ProjectileClass, FTransform(Direction.Rotation(), SpawnLocation));
		if (ABossProjectile* Projectile = Cast<ABossProjectile>(Spawned))
		{
			Projectile->Launch(Owner, Direction, ProjectileSpeed, ProjectileLifetime, Params);
		}
	}
}

bool UBossCombatComponent::GetWeaponSegment(FVector& OutStart, FVector& OutEnd) const
{
	if (const UStaticMeshComponent* WeaponMesh = Cast<UStaticMeshComponent>(WeaponComponent.Get()); WeaponMesh && WeaponMesh->IsVisible())
	{
		if (WeaponMesh->DoesSocketExist(WeaponBaseSocket) && WeaponMesh->DoesSocketExist(WeaponTipSocket))
		{
			OutStart = WeaponMesh->GetSocketLocation(WeaponBaseSocket);
			OutEnd = WeaponMesh->GetSocketLocation(WeaponTipSocket);
			return true;
		}

		if (const UStaticMesh* Mesh = WeaponMesh->GetStaticMesh())
		{
			// Sin sockets: el eje más largo del bounding box local es el largo del arma.
			const FBox Box = Mesh->GetBoundingBox();
			const FVector Center = Box.GetCenter();
			const FVector Extent = Box.GetExtent();

			FVector Axis = FVector::XAxisVector;
			float HalfLength = Extent.X;
			if (Extent.Y > HalfLength) { Axis = FVector::YAxisVector; HalfLength = Extent.Y; }
			if (Extent.Z > HalfLength) { Axis = FVector::ZAxisVector; HalfLength = Extent.Z; }

			const FTransform& Transform = WeaponMesh->GetComponentTransform();
			OutStart = Transform.TransformPosition(Center - Axis * HalfLength);
			OutEnd = Transform.TransformPosition(Center + Axis * HalfLength);
			return true;
		}
	}

	if (OwnerCharacter && OwnerCharacter->GetMesh() && OwnerCharacter->GetMesh()->DoesSocketExist(FallbackWeaponSocket))
	{
		OutStart = OutEnd = OwnerCharacter->GetMesh()->GetSocketLocation(FallbackWeaponSocket);
		return true;
	}

	return false;
}

void UBossCombatComponent::BuildWeaponSamples(TArray<FVector>& OutSamples) const
{
	OutSamples.Reset();

	FVector Start, End;
	if (!GetWeaponSegment(Start, End)) return;

	const int32 Count = FMath::Max(1, WeaponTraceSamples);
	for (int32 Index = 0; Index < Count; ++Index)
	{
		const float Alpha = Count > 1 ? static_cast<float>(Index) / (Count - 1) : 1.f;
		OutSamples.Add(FMath::Lerp(Start, End, Alpha));
	}
}

float UBossCombatComponent::GetCurrentBaseDamage() const
{
	return Attacks.IsValidIndex(CurrentAction.AttackIndex) ? Attacks[CurrentAction.AttackIndex].BaseDamage : DefaultNotifyDamage;
}

FName UBossCombatComponent::GetCurrentAttackName() const
{
	return Attacks.IsValidIndex(CurrentAction.AttackIndex) ? Attacks[CurrentAction.AttackIndex].AttackName : NAME_None;
}

float UBossCombatComponent::GetMaxAttackDistance() const
{
	float MaxDistance = 0.f;
	for (const FBossAttackDefinition& Attack : Attacks)
	{
		if (Attack.Montage)
		{
			MaxDistance = FMath::Max(MaxDistance, Attack.MaxDistance);
		}
	}
	return MaxDistance;
}

void UBossCombatComponent::RotateOwnerTowards(const FVector& Location, const float DeltaTime, const float DegreesPerSecond) const
{
	AActor* Owner = GetOwner();
	if (!Owner) return;

	const FVector ToTarget = (Location - Owner->GetActorLocation()).GetSafeNormal2D();
	if (ToTarget.IsNearlyZero()) return;

	const FRotator Current = Owner->GetActorRotation();
	const FRotator Desired(0.f, ToTarget.Rotation().Yaw, 0.f);
	const FRotator NewRotation = FMath::RInterpConstantTo(Current, Desired, DeltaTime, DegreesPerSecond);

	Owner->SetActorRotation(FRotator(0.f, NewRotation.Yaw, 0.f));
}

UAnimInstance* UBossCombatComponent::GetAnimInstance() const
{
	return OwnerCharacter && OwnerCharacter->GetMesh() ? OwnerCharacter->GetMesh()->GetAnimInstance() : nullptr;
}
