#include "Features/BossSystem/Core/BossEnemy.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "GameplayEffect.h"
#include "GenericTeamAgentInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraSystem.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Features/BossSystem/Actors/BossProjectile.h"
#include "Features/BossSystem/Components/BossCeilingBreakComponent.h"
#include "Features/BossSystem/Components/BossCombatComponent.h"
#include "Features/BossSystem/Components/BossSummonComponent.h"
#include "Features/BossSystem/Core/BossAIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/VelmaraAttributeSet.h"
#include "GAS/VelmaraGameplayTags.h"
#include "HUD/PaladinBossHealthBar.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Player/CustomPlayerController.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "UObject/ConstructorHelpers.h"

namespace BossDefaults
{
	const TCHAR* const MontageFolder = TEXT("/Game/BossAnimationsPack/Animations/AttackMontages/");

	UAnimMontage* FindMontage(const TCHAR* MontageName)
	{
		const FString Path = FString::Printf(TEXT("%s%s.%s"), MontageFolder, MontageName, MontageName);
		ConstructorHelpers::FObjectFinder<UAnimMontage> Finder(*Path);
		return Finder.Succeeded() ? Finder.Object.Get() : nullptr;
	}

	FBossHitWindow Weapon(const float Start, const float End, const bool bMultiHit = false, const float ReHitDelay = 0.4f)
	{
		FBossHitWindow Window;
		Window.Shape = EBossHitShape::Weapon;
		Window.StartTime = Start;
		Window.EndTime = End;
		Window.bAllowMultipleHits = bMultiHit;
		Window.ReHitDelay = ReHitDelay;
		return Window;
	}

	FBossHitWindow Sphere(const float Start, const float End, const FVector& Offset, const float Radius)
	{
		FBossHitWindow Window;
		Window.Shape = EBossHitShape::Sphere;
		Window.StartTime = Start;
		Window.EndTime = End;
		Window.SphereOffset = Offset;
		Window.SphereRadius = Radius;
		return Window;
	}

	FBossHitWindow Projectile(const float Time, const int32 Count = 1, const float Spread = 0.f)
	{
		FBossHitWindow Window;
		Window.Shape = EBossHitShape::Projectile;
		Window.StartTime = Time;
		Window.EndTime = Time;
		Window.ProjectileCount = Count;
		Window.ProjectileSpreadAngle = Spread;
		return Window;
	}

	FBossAttackDefinition Attack(const FName Name, const TCHAR* MontageName, const EBossAttackRange Range, const float MinDistance,
		const float MaxDistance, const float Damage, const float Cooldown, std::initializer_list<FBossHitWindow> Windows)
	{
		FBossAttackDefinition Definition;
		Definition.AttackName = Name;
		Definition.Montage = FindMontage(MontageName);
		Definition.Range = Range;
		Definition.MinDistance = MinDistance;
		Definition.MaxDistance = MaxDistance;
		Definition.BaseDamage = Damage;
		Definition.Cooldown = Cooldown;
		Definition.HitWindows = Windows;
		return Definition;
	}
}

ABossEnemy::ABossEnemy()
{
	PrimaryActorTick.bCanEverTick = false;

	bKnockbackResistant = true;

	CombatComponent = CreateDefaultSubobject<UBossCombatComponent>(TEXT("BossCombat"));
	SummonComponent = CreateDefaultSubobject<UBossSummonComponent>(TEXT("BossSummon"));
	CeilingBreakComponent = CreateDefaultSubobject<UBossCeilingBreakComponent>(TEXT("BossCeilingBreak"));

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), TEXT("hand_r"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
	WeaponMesh->CanCharacterStepUpOn = ECB_No;

	SolarShieldAuraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("SolarShieldAura"));
	SolarShieldAuraComponent->SetupAttachment(GetMesh());
	SolarShieldAuraComponent->bAutoActivate = false;

	AIControllerClass = ABossAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(48.f, 96.f);
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel3, ECR_Block);
	GetMesh()->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -96.f), FRotator(0.f, -90.f, 0.f));

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 360.f, 0.f);

	SetupDefaultAssets();
}

void ABossEnemy::SetupDefaultAssets()
{
	using namespace BossDefaults;

	static ConstructorHelpers::FObjectFinder<USkeletalMesh> MeshFinder(TEXT("/Game/BossAnimationsPack/Demo/Mannequin/Character/Mesh/SK_Mannequin.SK_Mannequin"));
	if (MeshFinder.Succeeded())
	{
		GetMesh()->SetSkeletalMeshAsset(MeshFinder.Object);
	}

	static ConstructorHelpers::FClassFinder<UAnimInstance> AnimFinder(TEXT("/Game/BossAnimationsPack/Animations/BossAnimationBluepint"));
	if (AnimFinder.Succeeded())
	{
		GetMesh()->AnimClass = AnimFinder.Class;
	}

	static ConstructorHelpers::FObjectFinder<UStaticMesh> StickFinder(TEXT("/Game/BossAnimationsPack/Stick/Stick.Stick"));
	if (StickFinder.Succeeded())
	{
		WeaponMesh->SetStaticMesh(StickFinder.Object);
	}

	static ConstructorHelpers::FObjectFinder<UAnimSequenceBase> DeathFinder(TEXT("/Game/BossAnimationsPack/Animations/Death/BossDeath.BossDeath"));
	if (DeathFinder.Succeeded())
	{
		DeathAnimation = DeathFinder.Object;
	}

	static ConstructorHelpers::FClassFinder<UPaladinBossHealthBar> HealthBarFinder(TEXT("/Game/Blueprints/HUD/WBP_PaladinBossHealthBar"));
	if (HealthBarFinder.Succeeded())
	{
		HealthBarClass = HealthBarFinder.Class;
	}

	static ConstructorHelpers::FClassFinder<UGameplayEffect> DamageEffectFinder(TEXT("/Game/Blueprints/GAS/Effects/GE_Damage"));
	if (DamageEffectFinder.Succeeded())
	{
		CombatComponent->DamageParams.DamageEffectClass = DamageEffectFinder.Class;
	}

	static ConstructorHelpers::FObjectFinder<UNiagaraSystem> SolarAuraFinder(TEXT("/Game/VFX/Aura/NS_BossAura.NS_BossAura"));
	if (SolarAuraFinder.Succeeded())
	{
		SolarShieldAuraSystem = SolarAuraFinder.Object;
	}

	static ConstructorHelpers::FObjectFinder<UMaterialInterface> SolarOverlayFinder(TEXT("/Game/VFX/Aura/MI_BossAuraShield.MI_BossAuraShield"));
	if (SolarOverlayFinder.Succeeded())
	{
		SolarShieldOverlayMaterial = SolarOverlayFinder.Object;
	}

	CombatComponent->ProjectileClass = ABossProjectile::StaticClass();

	const FGameplayTag ImpactTag = FGameplayTag::RequestGameplayTag(FName("Damage.Impact"), false);
	const FGameplayTag PunctureTag = FGameplayTag::RequestGameplayTag(FName("Damage.Puncture"), false);
	const FGameplayTag SlashTag = FGameplayTag::RequestGameplayTag(FName("Damage.Slash"), false);

	TArray<FBossAttackDefinition>& Attacks = CombatComponent->Attacks;
	Attacks.Reset();

	{
		FBossAttackDefinition Definition = Attack("SingleAttack", TEXT("BossSingleAttack_Montage"), EBossAttackRange::Short, 0.f, 300.f, 14.f, 2.5f,
			{ Weapon(0.2f, 0.65f) });
		Definition.MinStage = 0;
		Definition.Weight = 1.2f;
		Definition.RecoveryTime = 0.75f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("DoubleAttack", TEXT("BossDoubleAttack2_Montage"), EBossAttackRange::Short, 0.f, 300.f, 12.f, 4.f,
			{ Weapon(0.15f, 0.8f, true, 0.4f) });
		Definition.MinStage = 0;
		Definition.TrackingEndTime = 0.2f;
		Definition.RecoveryTime = 0.8f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("BackDoubleAttack", TEXT("BossBackDoubleAttack_Montage"), EBossAttackRange::Short, 0.f, 300.f, 12.f, 5.f,
			{ Weapon(0.15f, 0.8f, true, 0.4f) });
		Definition.MinStage = 0;
		Definition.Weight = 0.8f;
		Definition.RecoveryTime = 0.8f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("SpinAttack", TEXT("BossSpinAttack_Montage"), EBossAttackRange::Short, 0.f, 350.f, 16.f, 7.f,
			{ Weapon(0.15f, 0.85f, true, 0.5f) });
		Definition.MinStage = 0;
		Definition.MaxAngle = 180.f;
		Definition.TrackingEndTime = 0.1f;
		Definition.bShowGroundTelegraph = true;
		Definition.TelegraphRadius = 330.f;
		Definition.TelegraphOffset = FVector::ZeroVector;
		Definition.RecoveryTime = 0.95f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("TurningAttack", TEXT("BossTurningAttack1_Montage"), EBossAttackRange::Short, 0.f, 340.f, 15.f, 4.f,
			{ Weapon(0.15f, 0.75f) });
		Definition.MinStage = 0;
		Definition.MinAngle = 100.f;
		Definition.MaxAngle = 180.f;
		Definition.TrackingEndTime = 0.f;
		Definition.Weight = 2.f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("TurningSingleAttack", TEXT("BossTurningAttackSingle_Montage"), EBossAttackRange::Short, 0.f, 340.f, 14.f, 3.5f,
			{ Weapon(0.15f, 0.65f) });
		Definition.MinStage = 0;
		Definition.MinAngle = 100.f;
		Definition.MaxAngle = 180.f;
		Definition.TrackingEndTime = 0.f;
		Definition.Weight = 2.f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("SpearStab", TEXT("BossSpearStab_Montage"), EBossAttackRange::Medium, 250.f, 600.f, 17.f, 4.5f,
			{ Weapon(0.25f, 0.6f) });
		Definition.MinStage = 0;
		Definition.DamageTypeTag = PunctureTag;
		Definition.MaxAngle = 45.f;
		Definition.TrackingEndTime = 0.3f;
		Definition.RecoveryTime = 0.9f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("DodgeAttack", TEXT("BossDodgeAttack_Montage"), EBossAttackRange::Medium, 150.f, 550.f, 15.f, 6.f,
			{ Weapon(0.3f, 0.85f) });
		Definition.MinStage = 0;
		Definition.TrackingEndTime = 0.35f;
		Definition.Weight = 0.7f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("DodgeLeftAttack", TEXT("BossDogeLAttack_Montage"), EBossAttackRange::Medium, 150.f, 550.f, 15.f, 6.f,
			{ Weapon(0.3f, 0.85f) });
		Definition.MinStage = 0;
		Definition.TrackingEndTime = 0.35f;
		Definition.Weight = 0.6f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("DodgeRightAttack", TEXT("BossDogeRAttack_Montage"), EBossAttackRange::Medium, 150.f, 550.f, 15.f, 6.f,
			{ Weapon(0.3f, 0.85f) });
		Definition.MinStage = 0;
		Definition.TrackingEndTime = 0.35f;
		Definition.Weight = 0.6f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("TripleHeavyAttack", TEXT("BossTripleHeavyAttack_Montage"), EBossAttackRange::Short, 0.f, 320.f, 18.f, 8.f,
			{ Weapon(0.12f, 0.88f, true, 0.45f) });
		Definition.MinStage = 1;
		Definition.DamageTypeTag = ImpactTag;
		Definition.TrackingEndTime = 0.15f;
		Definition.RecoveryTime = 1.1f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("SpearCombo", TEXT("BossSpearComboAttack_Montage"), EBossAttackRange::Medium, 250.f, 650.f, 13.f, 7.f,
			{ Weapon(0.15f, 0.85f, true, 0.35f) });
		Definition.MinStage = 1;
		Definition.DamageTypeTag = PunctureTag;
		Definition.MaxAngle = 45.f;
		Definition.TrackingEndTime = 0.2f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("LongAttack", TEXT("BossLongAttack_Montage"), EBossAttackRange::Medium, 300.f, 800.f, 18.f, 5.f,
			{ Weapon(0.2f, 0.75f) });
		Definition.MinStage = 1;
		Definition.MaxAngle = 50.f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("LongCombo", TEXT("BossLongComboAttack_Montage"), EBossAttackRange::Medium, 300.f, 800.f, 13.f, 9.f,
			{ Weapon(0.15f, 0.88f, true, 0.4f) });
		Definition.MinStage = 1;
		Definition.MaxAngle = 50.f;
		Definition.RecoveryTime = 1.f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("JumpingSlam", TEXT("BossJumpingAttack_Montage"), EBossAttackRange::Long, 650.f, 1500.f, 24.f, 10.f,
			{ Weapon(0.4f, 0.7f), Sphere(0.5f, 0.62f, FVector(150.f, 0.f, -40.f), 330.f) });
		Definition.MinStage = 1;
		Definition.DamageTypeTag = ImpactTag;
		Definition.MaxAngle = 40.f;
		Definition.TrackingEndTime = 0.3f;
		Definition.bShowGroundTelegraph = true;
		Definition.TelegraphRadius = 330.f;
		Definition.TelegraphOffset = FVector(150.f, 0.f, 0.f);
		Definition.RecoveryTime = 1.2f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("AirSlash", TEXT("BossAirSlashAttack_Montage"), EBossAttackRange::Long, 750.f, 2600.f, 16.f, 5.5f,
			{ Projectile(0.4f, 1) });
		Definition.MinStage = 2;
		Definition.DamageTypeTag = SlashTag;
		Definition.MaxAngle = 60.f;
		Definition.TrackingEndTime = 0.38f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("AirSlashCombo", TEXT("BossAiSlashAttackCombo_Montage"), EBossAttackRange::Long, 900.f, 2600.f, 12.f, 9.5f,
			{ Projectile(0.3f, 1), Projectile(0.5f, 3, 30.f), Projectile(0.7f, 1) });
		Definition.MinStage = 2;
		Definition.DamageTypeTag = SlashTag;
		Definition.MaxAngle = 60.f;
		Definition.TrackingEndTime = 0.68f;
		Definition.TrackingSpeed = 120.f;
		Definition.RecoveryTime = 1.05f;
		Attacks.Add(Definition);
	}

	SummonComponent->SummonMontage = FindMontage(TEXT("BossHeavySingleAttack_Montage"));
	CeilingBreakComponent->BreakMontage = FindMontage(TEXT("BossJumpingAttack_Montage"));
	CeilingBreakComponent->BreakEventTime = 0.55f;

	EvadeMontages.Reset();
	const TCHAR* EvadeNames[] = { TEXT("BossDodgeB_Montage"), TEXT("BossDodgeL_Montage"), TEXT("BossDodgeR_Montage") };
	for (const TCHAR* EvadeName : EvadeNames)
	{
		if (UAnimMontage* EvadeMontage = FindMontage(EvadeName))
		{
			EvadeMontages.Add(EvadeMontage);
		}
	}
}

void ABossEnemy::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		if (MaxHealthOverride > 0.f)
		{
			AbilitySystemComponent->SetNumericAttributeBase(UVelmaraAttributeSet::GetMaxHealthAttribute(), MaxHealthOverride);
			AbilitySystemComponent->SetNumericAttributeBase(UVelmaraAttributeSet::GetHealthAttribute(), MaxHealthOverride);
		}

		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UVelmaraAttributeSet::GetHealthAttribute())
			.AddUObject(this, &ABossEnemy::HandleHealthChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UVelmaraAttributeSet::GetMaxHealthAttribute())
			.AddUObject(this, &ABossEnemy::HandleHealthBarAttributeChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UVelmaraAttributeSet::GetShieldAttribute())
			.AddUObject(this, &ABossEnemy::HandleHealthBarAttributeChanged);
		AbilitySystemComponent->GetGameplayAttributeValueChangeDelegate(UVelmaraAttributeSet::GetMaxShieldAttribute())
			.AddUObject(this, &ABossEnemy::HandleHealthBarAttributeChanged);
	}

	CombatComponent->SetWeaponComponent(WeaponMesh);
	CeilingBreakComponent->OnStageChanged.AddUniqueDynamic(this, &ABossEnemy::HandleStageChanged);
	if (SummonComponent)
	{
		SummonComponent->SetCurrentStage(CombatComponent ? CombatComponent->GetStage() : 0);
	}

	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed;
}

void ABossEnemy::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	GetWorldTimerManager().ClearTimer(HideHealthBarTimer);
	HideHealthBar();

	Super::EndPlay(EndPlayReason);
}

void ABossEnemy::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	if (const IGenericTeamAgentInterface* TeamAgent = Cast<IGenericTeamAgentInterface>(NewController))
	{
		TeamId = TeamAgent->GetGenericTeamId();
	}
}

void ABossEnemy::StartEncounter()
{
	if (bEncounterStarted || bDefeated) return;

	bEncounterStarted = true;

	SummonComponent->NotifyEncounterStarted();

	if (HealthBarClass && !HealthBarWidget)
	{
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(this, 0))
		{
			HealthBarWidget = CreateWidget<UPaladinBossHealthBar>(PlayerController, HealthBarClass);
			if (HealthBarWidget)
			{
				HealthBarWidget->AddToViewport();
			}
		}
	}
	UpdateHealthBar();

	OnEncounterStarted.Broadcast();
	BP_OnEncounterStarted();
}

void ABossEnemy::ReceiveDamage_Implementation(const FGameplayEventData DamagePayload)
{
	Super::ReceiveDamage_Implementation(DamagePayload);

	if (bDefeated) return;

	LastDamageCauser = const_cast<AActor*>(DamagePayload.Instigator.Get());

	StartEncounter();

	HitFlash(0.08f, 1.f);

	if (ABossAIController* BossController = Cast<ABossAIController>(GetController()))
	{
		BossController->NotifyBossDamaged(LastDamageCauser);
	}
}

void ABossEnemy::MortalDamage_Implementation(const FGameplayEventData DeathPayload)
{
	Super::MortalDamage_Implementation(DeathPayload);

	if (bDefeated) return;

	bDefeated = true;
	bSolarShieldActive = false;
	LastDamageCauser = const_cast<AActor*>(DeathPayload.Instigator.Get());

	if (SolarShieldAuraComponent)
	{
		SolarShieldAuraComponent->Deactivate();
	}

	if (bApplyShieldOverlayMaterial && GetMesh())
	{
		GetMesh()->SetOverlayMaterial(nullptr);
	}

	if (AbilitySystemComponent && !AbilitySystemComponent->HasMatchingGameplayTag(FVelmaraGameplayTags::Get().State_Dead))
	{
		AbilitySystemComponent->AddLooseGameplayTag(FVelmaraGameplayTags::Get().State_Dead);
	}

	CombatComponent->CancelCurrentAction();
	SummonComponent->OnBossDied();
	CeilingBreakComponent->OnBossDied();

	if (ABossAIController* BossController = Cast<ABossAIController>(GetController()))
	{
		BossController->HandleBossDeath();
	}

	GetCharacterMovement()->StopMovementImmediately();
	GetCharacterMovement()->DisableMovement();
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Pawn, ECR_Ignore);

	if (DeathAnimation)
	{
		GetMesh()->PlayAnimation(DeathAnimation, false);
	}

	UpdateHealthBar();
	if (HideHealthBarDelay > 0.f)
	{
		GetWorldTimerManager().SetTimer(HideHealthBarTimer, this, &ABossEnemy::HideHealthBar, HideHealthBarDelay, false);
	}
	else
	{
		HideHealthBar();
	}

	OnDead.Broadcast(this);
	OnBossDefeated.Broadcast();
	BP_OnBossDefeated();
}

float ABossEnemy::GetHealthPercent() const
{
	if (!AbilitySystemComponent) return 0.f;

	const float MaxHealth = AbilitySystemComponent->GetNumericAttribute(UVelmaraAttributeSet::GetMaxHealthAttribute());
	if (MaxHealth <= 0.f) return 0.f;

	return FMath::Clamp(AbilitySystemComponent->GetNumericAttribute(UVelmaraAttributeSet::GetHealthAttribute()) / MaxHealth, 0.f, 1.f);
}

float ABossEnemy::GetShieldPercent() const
{
	if (!AbilitySystemComponent) return 0.f;

	const float MaxShield = AbilitySystemComponent->GetNumericAttribute(UVelmaraAttributeSet::GetMaxShieldAttribute());
	if (MaxShield <= 0.f) return 0.f;

	return FMath::Clamp(AbilitySystemComponent->GetNumericAttribute(UVelmaraAttributeSet::GetShieldAttribute()) / MaxShield, 0.f, 1.f);
}

void ABossEnemy::ActivateSolarShield(const float ShieldAmount)
{
	if (bDefeated) return;

	const float FinalAmount = ShieldAmount > 0.f ? ShieldAmount : Stage3ShieldAmount;
	if (FinalAmount <= 0.f) return;

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->SetNumericAttributeBase(UVelmaraAttributeSet::GetMaxShieldAttribute(), FinalAmount);
		AbilitySystemComponent->SetNumericAttributeBase(UVelmaraAttributeSet::GetShieldAttribute(), FinalAmount);
	}

	bSolarShieldActive = true;

	if (SolarShieldAuraComponent && SolarShieldAuraSystem)
	{
		SolarShieldAuraComponent->SetAsset(SolarShieldAuraSystem);
		SolarShieldAuraComponent->Activate(true);
	}

	if (bApplyShieldOverlayMaterial && SolarShieldOverlayMaterial && GetMesh())
	{
		GetMesh()->SetOverlayMaterial(SolarShieldOverlayMaterial);
	}

	if (SolarShieldActivateSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SolarShieldActivateSound, GetActorLocation());
	}

	UpdateHealthBar();
	OnSolarShieldActivated.Broadcast(FinalAmount);
	BP_OnSolarShieldActivated(FinalAmount);
}

bool ABossEnemy::IsPossessedMinionAttacker(const AActor* AttackerActor, const AActor* InstigatorActor,
	const UAbilitySystemComponent* SourceASC) const
{
	if (AttackerActor == this || (!AttackerActor && InstigatorActor == this)) return false;

	const ACustomPlayerController* PC = Cast<ACustomPlayerController>(InstigatorActor);
	if (!PC)
	{
		if (const APawn* AttackerPawn = Cast<APawn>(AttackerActor))
		{
			PC = Cast<ACustomPlayerController>(AttackerPawn->GetController());
		}
	}
	if (!PC)
	{
		if (const APawn* InstigatorPawn = Cast<APawn>(InstigatorActor))
		{
			PC = Cast<ACustomPlayerController>(InstigatorPawn->GetController());
		}
	}
	if (!PC && GetWorld())
	{
		PC = Cast<ACustomPlayerController>(UGameplayStatics::GetPlayerController(GetWorld(), 0));
	}

	if (PC && (PC->IsPossessedHost(AttackerActor) || (InstigatorActor != this && PC->IsPossessedHost(InstigatorActor))))
	{
		return true;
	}

	const FGameplayTag PossessedTag = FGameplayTag::RequestGameplayTag(FName("Character.Possessed"), false);
	if (PossessedTag.IsValid())
	{
		if (SourceASC && SourceASC->HasMatchingGameplayTag(PossessedTag))
		{
			return true;
		}

		if (const AActor* Candidate = AttackerActor)
		{
			if (const IAbilitySystemInterface* AbilityInterface = Cast<IAbilitySystemInterface>(Candidate))
			{
				if (const UAbilitySystemComponent* CandidateASC = AbilityInterface->GetAbilitySystemComponent())
				{
					if (CandidateASC->HasMatchingGameplayTag(PossessedTag))
					{
						return true;
					}
				}
			}
			else if (const UAbilitySystemComponent* CandidateASC = Candidate->FindComponentByClass<UAbilitySystemComponent>())
			{
				if (CandidateASC->HasMatchingGameplayTag(PossessedTag))
				{
					return true;
				}
			}
		}
	}

	return false;
}

void ABossEnemy::ResolveIncomingDamage(const float RawDamage, const float CurrentShield, const AActor* AttackerActor,
	const AActor* InstigatorActor, const UAbilitySystemComponent* SourceASC, float& OutDamageToShield,
	float& OutDamageToHealth, bool& bOutHitByPossessedMinion) const
{
	bOutHitByPossessedMinion = IsPossessedMinionAttacker(AttackerActor, InstigatorActor, SourceASC);

	const float ShieldMultiplier = bOutHitByPossessedMinion
		? PossessedMinionShieldMultiplier
		: (bSolarShieldActive ? UnpossessedSolarShieldMultiplier : 1.0f);
	const float HealthMultiplier = bOutHitByPossessedMinion ? PossessedMinionHealthMultiplier : 1.0f;

	if (CurrentShield > 0.f)
	{
		const float EffectiveShieldDamage = RawDamage * ShieldMultiplier;
		if (EffectiveShieldDamage <= CurrentShield)
		{
			OutDamageToShield = EffectiveShieldDamage;
			OutDamageToHealth = 0.f;
		}
		else
		{
			OutDamageToShield = CurrentShield;
			const float RawConsumedByShield = ShieldMultiplier > KINDA_SMALL_NUMBER ? (CurrentShield / ShieldMultiplier) : RawDamage;
			const float RemainingRaw = FMath::Max(0.f, RawDamage - RawConsumedByShield);
			OutDamageToHealth = RemainingRaw * HealthMultiplier;
		}
	}
	else
	{
		OutDamageToShield = 0.f;
		OutDamageToHealth = RawDamage * HealthMultiplier;
	}
}

void ABossEnemy::NotifyHitByPossessedMinion(AActor* MinionActor, const float ShieldDamage, const float HealthDamage)
{
	if (UWorld* World = GetWorld())
	{
		const double Now = World->GetTimeSeconds();
		if (PossessedMinionHitEnergyGain > 0.f && (Now - LastPossessedMinionEnergyGainTime) >= PossessedMinionEnergyGainCooldown)
		{
			ACustomPlayerController* PC = nullptr;
			if (const APawn* MinionPawn = Cast<APawn>(MinionActor))
			{
				PC = Cast<ACustomPlayerController>(MinionPawn->GetController());
			}
			if (!PC)
			{
				PC = Cast<ACustomPlayerController>(UGameplayStatics::GetPlayerController(World, 0));
			}

			if (PC)
			{
				PC->RestoreOriginalBodyEnergy(PossessedMinionHitEnergyGain);
				LastPossessedMinionEnergyGainTime = Now;
			}
		}
	}

	OnHitByPossessedMinion.Broadcast(MinionActor, ShieldDamage, HealthDamage);
	BP_OnHitByPossessedMinion(MinionActor, ShieldDamage, HealthDamage);
}

void ABossEnemy::HandleSolarShieldBroken(AActor* BreakerActor)
{
	bSolarShieldActive = false;

	if (SolarShieldAuraComponent)
	{
		SolarShieldAuraComponent->Deactivate();
	}

	if (bApplyShieldOverlayMaterial && GetMesh())
	{
		GetMesh()->SetOverlayMaterial(nullptr);
	}

	if (SolarShieldBreakSound)
	{
		UGameplayStatics::PlaySoundAtLocation(this, SolarShieldBreakSound, GetActorLocation());
	}

	if (SolarShieldBreakEnergyBonus > 0.f)
	{
		if (UWorld* World = GetWorld())
		{
			ACustomPlayerController* PC = nullptr;
			if (const APawn* BreakerPawn = Cast<APawn>(BreakerActor))
			{
				PC = Cast<ACustomPlayerController>(BreakerPawn->GetController());
			}
			if (!PC)
			{
				PC = Cast<ACustomPlayerController>(UGameplayStatics::GetPlayerController(World, 0));
			}

			if (PC)
			{
				PC->RestoreOriginalBodyEnergy(SolarShieldBreakEnergyBonus);
			}
		}
	}

	UpdateHealthBar();
	OnSolarShieldBroken.Broadcast(BreakerActor);
	BP_OnSolarShieldBroken(BreakerActor);

	if (!bDefeated && IsAlive() && GetHealthPercent() > 0.f)
	{
		if (ABossAIController* BossController = Cast<ABossAIController>(GetController()))
		{
			BossController->TriggerStagger(SolarShieldBreakStaggerDuration, SolarShieldBreakStaggerMontage);
		}
	}
}

AActor* ABossEnemy::GetCombatTarget() const
{
	const ABossAIController* BossController = Cast<ABossAIController>(GetController());
	return BossController ? BossController->GetCombatTarget() : nullptr;
}

void ABossEnemy::HandleHealthChanged(const FOnAttributeChangeData& Data)
{
	UpdateHealthBar();

	if (!bDefeated)
	{
		CeilingBreakComponent->EvaluateHealth(GetHealthPercent());
	}
}

void ABossEnemy::HandleHealthBarAttributeChanged(const FOnAttributeChangeData& Data)
{
	UpdateHealthBar();
}

void ABossEnemy::HandleStageChanged(const int32 NewStage)
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed + WalkSpeedBonusPerStage * NewStage;

	if (SummonComponent)
	{
		SummonComponent->SetCurrentStage(NewStage);
	}

	BP_OnStageChanged(NewStage);
}

void ABossEnemy::UpdateHealthBar() const
{
	if (!HealthBarWidget || !AbilitySystemComponent) return;

	HealthBarWidget->ReceivePercentagesFromBoss(GetHealthPercent(), GetShieldPercent());
}

void ABossEnemy::HideHealthBar()
{
	if (HealthBarWidget)
	{
		HealthBarWidget->RemoveFromParent();
		HealthBarWidget = nullptr;
	}
}
