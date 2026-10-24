#include "Features/BossSystem/Actors/BossLightShaft.h"

#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Camera/CameraShakeBase.h"
#include "Components/DecalComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "DrawDebugHelpers.h"
#include "Engine/StaticMesh.h"
#include "Entities/Entity.h"
#include "Features/BossSystem/Actors/BossCeilingBreakPoint.h"
#include "Features/BossSystem/Core/BossSystemLibrary.h"
#include "Features/BossSystem/Subsystems/SunlightExposureSubsystem.h"
#include "GenericPool/Core/ActorPoolSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "Engine/World.h"
#include "TimerManager.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "UObject/ConstructorHelpers.h"

namespace BossLightShaft
{
	// Parámetro escalar opcional del BeamMaterial (0-1) que sigue la intensidad de la luz.
	const FName BeamIntensityParam(TEXT("Intensity"));
}

ABossLightShaft::ABossLightShaft()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.bStartWithTickEnabled = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	Root->SetMobility(EComponentMobility::Movable);
	RootComponent = Root;

	Light = CreateDefaultSubobject<USpotLightComponent>(TEXT("Light"));
	Light->SetupAttachment(Root);
	Light->SetMobility(EComponentMobility::Movable);
	Light->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	Light->IntensityUnits = ELightUnits::Candelas;
	Light->Intensity = 0.f;
	Light->SetCastShadows(true);
	// Con Exponential Height Fog + Volumetric Fog en el nivel se ve la columna de luz sin necesidad de mesh.
	Light->VolumetricScatteringIntensity = 4.f;
	// El hueco no ilumina: de la luz se encarga la Directional Light del nivel. Activalo en un Blueprint hijo si hace falta.
	Light->SetVisibility(false);

	BeamMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("BeamMesh"));
	BeamMesh->SetupAttachment(Root);
	BeamMesh->SetMobility(EComponentMobility::Movable);
	BeamMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BeamMesh->SetCastShadow(false);
	BeamMesh->SetVisibility(false);

	static ConstructorHelpers::FObjectFinder<UStaticMesh> CylinderFinder(TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
	if (CylinderFinder.Succeeded())
	{
		BeamMesh->SetStaticMesh(CylinderFinder.Object);
	}

	GroundDecal = CreateDefaultSubobject<UDecalComponent>(TEXT("GroundDecal"));
	GroundDecal->SetupAttachment(Root);
	GroundDecal->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	GroundDecal->SetVisibility(false);

	DustEffect = CreateDefaultSubobject<UNiagaraComponent>(TEXT("DustEffect"));
	DustEffect->SetupAttachment(Root);
	DustEffect->SetAutoActivate(false);

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DustFinder(TEXT("/Game/Effects/ParticleSystems/Dust/NS_Dust.NS_Dust"));
	if (DustFinder.Succeeded())
	{
		DustEffect->SetAsset(DustFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> DebrisFinder(TEXT("/Game/MechanicalDamageFX/FX/NS_ImpactDebris_Large.NS_ImpactDebris_Large"));
	if (DebrisFinder.Succeeded())
	{
		ImpactEffect = DebrisFinder.Object;
	}

	static ConstructorHelpers::FClassFinder<UCameraShakeBase> ShakeFinder(TEXT("/Game/Blueprints/Cameras/CameraShakes/CS_Rock"));
	if (ShakeFinder.Succeeded())
	{
		ImpactCameraShake = ShakeFinder.Class;
	}

	ImpactDamageTypeTag = FGameplayTag::RequestGameplayTag(FName("Damage.Impact"), false);
}

void ABossLightShaft::OpenShaft(AActor* InBoss, const FVector& InTop, const FVector& InFloor, const float InRadius, const FBossDamageParams& InDamageParams,
	ABossCeilingBreakPoint* InBreakPoint)
{
	GetWorldTimerManager().ClearTimer(PhaseTimer);
	GetWorldTimerManager().ClearTimer(DamageTimer);
	ClearExposure();

	Boss = InBoss;
	BreakPoint = InBreakPoint;
	Top = InTop;
	Floor = InFloor;
	Radius = FMath::Max(InRadius, 50.f);
	DamageParams = InDamageParams;

	SetActorLocationAndRotation(Top, FRotator::ZeroRotator);
	LayoutComponents();

	Phase = EShaftPhase::Telegraph;
	PhaseElapsed = 0.f;
	SetLightAlpha(0.f);
	SetActorTickEnabled(true);

	DustEffect->Activate(true);

	if (TelegraphSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, TelegraphSound, Floor);
	}

	OnTelegraphStarted();

	if (TelegraphDuration > 0.f)
	{
		GetWorldTimerManager().SetTimer(PhaseTimer, this, &ABossLightShaft::BeginOpenPhase, TelegraphDuration, false);
	}
	else
	{
		BeginOpenPhase();
	}
}

void ABossLightShaft::BeginOpenPhase()
{
	if (Phase != EShaftPhase::Telegraph) return;

	Phase = EShaftPhase::Open;
	PhaseElapsed = 0.f;
	SetActorTickEnabled(true);

	DustEffect->Deactivate();

	// El techo cae recién ahora: durante el aviso solo se filtra un poco de luz.
	if (ABossCeilingBreakPoint* LinkedPoint = BreakPoint.Get())
	{
		LinkedPoint->Break();
	}

	ApplyImpact();

	GetWorldTimerManager().SetTimer(DamageTimer, this, &ABossLightShaft::DamageTick, DamageTickInterval, true, DamageTickInterval);

	if (Lifetime > 0.f)
	{
		GetWorldTimerManager().SetTimer(PhaseTimer, this, &ABossLightShaft::CloseShaft, Lifetime, false);
	}

	OnShaftOpened();
}

void ABossLightShaft::CloseShaft()
{
	if (Phase == EShaftPhase::Inactive || Phase == EShaftPhase::Closing) return;

	GetWorldTimerManager().ClearTimer(PhaseTimer);
	GetWorldTimerManager().ClearTimer(DamageTimer);
	ClearExposure();

	Phase = EShaftPhase::Closing;
	PhaseElapsed = 0.f;
	SetActorTickEnabled(true);

	DustEffect->Deactivate();
}

void ABossLightShaft::FinishClose()
{
	Phase = EShaftPhase::Inactive;
	SetLightAlpha(0.f);

	OnShaftClosed();

	if (UActorPoolSubsystem* Pool = GetWorld()->GetSubsystem<UActorPoolSubsystem>())
	{
		Pool->ReturnToPool(this);
	}
	else
	{
		Destroy();
	}
}

void ABossLightShaft::Tick(const float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	PhaseElapsed += DeltaSeconds;

	switch (Phase)
	{
	case EShaftPhase::Telegraph:
		{
			// Parpadeo tenue: avisa dónde va a caer el techo.
			const float FadeIn = FMath::Clamp(PhaseElapsed / 0.3f, 0.f, 1.f);
			const float Flicker = 0.65f + 0.35f * FMath::Sin(PhaseElapsed * 30.f);
			SetLightAlpha(TelegraphIntensityFraction * FadeIn * Flicker);
			break;
		}
	case EShaftPhase::Open:
		{
			const float Alpha = FMath::Clamp(PhaseElapsed / LightRampDuration, 0.f, 1.f);
			SetLightAlpha(FMath::Lerp(TelegraphIntensityFraction, 1.f, Alpha));
			if (Alpha >= 1.f && !bDrawDebug)
			{
				SetActorTickEnabled(false);
			}
			break;
		}
	case EShaftPhase::Closing:
		{
			const float Alpha = FMath::Clamp(PhaseElapsed / CloseDuration, 0.f, 1.f);
			SetLightAlpha(1.f - Alpha);
			if (Alpha >= 1.f)
			{
				FinishClose();
				return;
			}
			break;
		}
	default:
		SetActorTickEnabled(false);
		break;
	}

	if (bDrawDebug && Phase != EShaftPhase::Inactive)
	{
		const float HalfHeight = FMath::Max((Top.Z - Floor.Z) * 0.5f, 50.f);
		DrawDebugCylinder(GetWorld(), Floor, Top, Radius, 24, Phase == EShaftPhase::Open ? FColor::Yellow : FColor::Orange, false, 0.f, 0, 2.f);
		DrawDebugString(GetWorld(), Floor + FVector(0.f, 0.f, HalfHeight), TEXT("Boss Light Shaft"), nullptr, FColor::White, 0.f);
	}
}

void ABossLightShaft::ApplyImpact()
{
	if (ImpactEffect)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ImpactEffect, Floor);
	}

	if (ImpactSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Floor);
	}

	if (ImpactCameraShake)
	{
		UGameplayStatics::PlayWorldCameraShake(this, ImpactCameraShake, Floor, 0.f, ImpactCameraShakeRadius);
	}

	if (ImpactDamage <= 0.f) return;

	AActor* BossActor = Boss.Get();

	FBossDamageParams Params = DamageParams;
	Params.Damage = ImpactDamage;
	if (ImpactDamageTypeTag.IsValid())
	{
		Params.DamageTypeTag = ImpactDamageTypeTag;
	}

	TArray<AActor*> Candidates;
	GatherCandidates(Candidates);

	for (AActor* Candidate : Candidates)
	{
		if (!IsPointInsideShaft(Candidate->GetActorLocation())) continue;
		if (!UBossSystemLibrary::IsValidBossTarget(BossActor, Candidate, Params.bOnlyPlayerControlled)) continue;
		if (UBossSystemLibrary::IsImmuneToBossHit(Candidate, Params.ImmunityTags)) continue;

		const FHitResult Hit(Candidate, nullptr, Candidate->GetActorLocation(), FVector::UpVector);
		UBossSystemLibrary::ApplyBossDamage(BossActor, this, Candidate, Params, 1.f, Hit);
	}
}

void ABossLightShaft::DamageTick()
{
	if (Phase != EShaftPhase::Open) return;

	USunlightExposureSubsystem* Exposure = GetWorld()->GetSubsystem<USunlightExposureSubsystem>();
	const AActor* BossActor = Boss.Get();

	TArray<AActor*> Candidates;
	GatherCandidates(Candidates);

	TArray<TWeakObjectPtr<AActor>> NowExposed;
	for (AActor* Candidate : Candidates)
	{
		if (!IsValid(Candidate) || Candidate == BossActor || Candidate->IsHidden()) continue;
		if (const AEntity* Entity = Cast<AEntity>(Candidate); Entity && !Entity->IsAlive()) continue;
		if (!IsPointInsideShaft(Candidate->GetActorLocation())) continue;
		if (bRequireLineOfSight && !HasLineOfSight(Candidate)) continue;

		NowExposed.Add(Candidate);
	}

	if (Exposure)
	{
		for (const TWeakObjectPtr<AActor>& Previous : ExposedActors)
		{
			if (AActor* PreviousActor = Previous.Get(); PreviousActor && !NowExposed.Contains(Previous))
			{
				Exposure->SetExposed(PreviousActor, this, false);
			}
		}
	}

	FBossDamageParams DirectParams = DamageParams;
	DirectParams.Damage = DirectDamagePerSecond * DamageTickInterval;
	DirectParams.DamageTypeTag = FGameplayTag::RequestGameplayTag(FName("Damage.Environmental.Sun"), false);
	DirectParams.CueTag = FGameplayTag();

	for (const TWeakObjectPtr<AActor>& Exposed : NowExposed)
	{
		AActor* Target = Exposed.Get();
		if (!Target) continue;

		const APawn* Pawn = Cast<APawn>(Target);
		const bool bIsPlayer = Pawn && Pawn->IsPlayerControlled();
		const ELightShaftDamageMode Mode = bIsPlayer ? PlayerDamageMode : ELightShaftDamageMode::DirectDamage;

		if (Mode != ELightShaftDamageMode::DirectDamage && Exposure)
		{
			Exposure->SetExposed(Target, this, true);
		}

		if (Mode != ELightShaftDamageMode::SunBurningEvent && DirectParams.Damage > 0.f)
		{
			// Daño ambiental: se lo aplica el propio objetivo, igual que el sol.
			UBossSystemLibrary::ApplyBossDamage(nullptr, this, Target, DirectParams, 1.f, FHitResult());
		}
	}

	ExposedActors = MoveTemp(NowExposed);
}

void ABossLightShaft::GatherCandidates(TArray<AActor*>& OutCandidates) const
{
	OutCandidates.Reset();

	UWorld* World = GetWorld();
	if (!World) return;

	for (FConstPlayerControllerIterator It = World->GetPlayerControllerIterator(); It; ++It)
	{
		if (const APlayerController* PlayerController = It->Get())
		{
			if (APawn* Pawn = PlayerController->GetPawn())
			{
				OutCandidates.AddUnique(Pawn);
			}
		}
	}

	if (!bAffectNonPlayerEntities) return;

	const float HalfHeight = FMath::Max((Top.Z - Floor.Z) * 0.5f, 50.f);
	const FVector Center = (Top + Floor) * 0.5f;

	TArray<FOverlapResult> Overlaps;
	World->OverlapMultiByObjectType(Overlaps, Center, FQuat::Identity, FCollisionObjectQueryParams(ECC_Pawn),
		FCollisionShape::MakeCapsule(Radius, HalfHeight));

	for (const FOverlapResult& Overlap : Overlaps)
	{
		if (AEntity* Entity = Cast<AEntity>(Overlap.GetActor()))
		{
			OutCandidates.AddUnique(Entity);
		}
	}
}

bool ABossLightShaft::IsPointInsideShaft(const FVector& Point) const
{
	if (Phase == EShaftPhase::Inactive) return false;

	return FVector::Dist2D(Point, Floor) <= Radius && Point.Z >= Floor.Z - 100.f && Point.Z <= Top.Z;
}

bool ABossLightShaft::HasLineOfSight(const AActor* Target) const
{
	// Rayo vertical desde el hueco hasta el objetivo: cualquier cosa encima lo cubre del sol.
	const FVector TargetLocation = Target->GetActorLocation();
	const FVector Start(TargetLocation.X, TargetLocation.Y, Top.Z - 30.f);
	const FVector End = TargetLocation + FVector(0.f, 0.f, 40.f);

	if (Start.Z <= End.Z) return true;

	FCollisionQueryParams Params(SCENE_QUERY_STAT(BossShaftLineOfSight), false, this);
	Params.AddIgnoredActor(Target);
	if (const AActor* BossActor = Boss.Get())
	{
		Params.AddIgnoredActor(BossActor);
	}

	// Las armas e items pegados al objetivo no lo cubren.
	TArray<AActor*> AttachedActors;
	Target->GetAttachedActors(AttachedActors, true, true);
	Params.AddIgnoredActors(AttachedActors);

	FHitResult Hit;
	const bool bBlocked = GetWorld()->LineTraceSingleByChannel(Hit, Start, End, LineOfSightChannel, Params);

	if (bDrawDebug)
	{
		DrawDebugLine(GetWorld(), Start, bBlocked ? FVector(Hit.ImpactPoint) : End, bBlocked ? FColor::Blue : FColor::Yellow, false, DamageTickInterval);
	}

	return !bBlocked;
}

void ABossLightShaft::LayoutComponents()
{
	const float Height = FMath::Max(Top.Z - Floor.Z, 100.f);

	const float ConeAngle = FMath::Clamp(FMath::RadiansToDegrees(FMath::Atan(Radius / Height)) * 1.15f, 2.f, 80.f);
	Light->SetOuterConeAngle(ConeAngle);
	Light->SetInnerConeAngle(ConeAngle * 0.75f);
	Light->SetAttenuationRadius(Height * 1.25f + Radius);
	Light->SetLightColor(LightColor);

	if (BeamMaterial)
	{
		// El cilindro básico mide 100 uu con el pivote al centro.
		const UMaterialInstanceDynamic* CurrentInstance = Cast<UMaterialInstanceDynamic>(BeamMesh->GetMaterial(0));
		if (!CurrentInstance || CurrentInstance->Parent != BeamMaterial)
		{
			BeamMesh->CreateDynamicMaterialInstance(0, BeamMaterial);
		}
		BeamMesh->SetRelativeLocation(FVector(0.f, 0.f, -Height * 0.5f));
		BeamMesh->SetRelativeScale3D(FVector(Radius * 2.f / 100.f, Radius * 2.f / 100.f, Height / 100.f));
		BeamMesh->SetVisibility(true);
	}
	else
	{
		BeamMesh->SetVisibility(false);
	}

	if (GroundDecalMaterial)
	{
		GroundDecal->SetDecalMaterial(GroundDecalMaterial);
		GroundDecal->DecalSize = FVector(128.f, Radius, Radius);
		GroundDecal->SetRelativeLocation(FVector(0.f, 0.f, -Height));
		GroundDecal->MarkRenderStateDirty();
		GroundDecal->SetVisibility(true);
	}
	else
	{
		GroundDecal->SetVisibility(false);
	}
}

void ABossLightShaft::SetLightAlpha(const float Alpha)
{
	Light->SetIntensity(LightIntensity * Alpha);

	if (BeamMesh->IsVisible())
	{
		if (UMaterialInstanceDynamic* BeamInstance = Cast<UMaterialInstanceDynamic>(BeamMesh->GetMaterial(0)))
		{
			BeamInstance->SetScalarParameterValue(BossLightShaft::BeamIntensityParam, Alpha);
		}
	}
}

void ABossLightShaft::ClearExposure()
{
	if (const UWorld* World = GetWorld())
	{
		if (USunlightExposureSubsystem* Exposure = World->GetSubsystem<USunlightExposureSubsystem>())
		{
			Exposure->ClearSource(this);
		}
	}

	ExposedActors.Reset();
}

void ABossLightShaft::OnPoolActivate_Implementation()
{
	Phase = EShaftPhase::Inactive;
	PhaseElapsed = 0.f;
}

void ABossLightShaft::OnPoolDeactivate_Implementation()
{
	GetWorldTimerManager().ClearTimer(PhaseTimer);
	GetWorldTimerManager().ClearTimer(DamageTimer);
	ClearExposure();

	Phase = EShaftPhase::Inactive;
	Boss.Reset();
	BreakPoint.Reset();

	Light->SetIntensity(0.f);
	DustEffect->DeactivateImmediate();
	BeamMesh->SetVisibility(false);
	GroundDecal->SetVisibility(false);
}

void ABossLightShaft::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	ClearExposure();
	Super::EndPlay(EndPlayReason);
}
