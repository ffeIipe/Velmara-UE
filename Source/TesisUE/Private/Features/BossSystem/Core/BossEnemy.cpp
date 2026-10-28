#include "Features/BossSystem/Core/BossEnemy.h"

#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "GenericTeamAgentInterface.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimMontage.h"
#include "Animation/AnimSequenceBase.h"
#include "Blueprint/UserWidget.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/StaticMesh.h"
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
#include "Engine/World.h"
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

	CombatComponent = CreateDefaultSubobject<UBossCombatComponent>(TEXT("BossCombat"));
	SummonComponent = CreateDefaultSubobject<UBossSummonComponent>(TEXT("BossSummon"));
	CeilingBreakComponent = CreateDefaultSubobject<UBossCeilingBreakComponent>(TEXT("BossCeilingBreak"));

	WeaponMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponMesh"));
	WeaponMesh->SetupAttachment(GetMesh(), TEXT("hand_r"));
	WeaponMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponMesh->SetGenerateOverlapEvents(false);
	WeaponMesh->CanCharacterStepUpOn = ECB_No;

	AIControllerClass = ABossAIController::StaticClass();
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;

	GetCapsuleComponent()->InitCapsuleSize(48.f, 96.f);
	// Las armas del jugador (ASword) detectan golpes con el canal EnemyTrace, igual que en CBP_Enemy.
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

	CombatComponent->ProjectileClass = ABossProjectile::StaticClass();

	const FGameplayTag ImpactTag = FGameplayTag::RequestGameplayTag(FName("Damage.Impact"), false);
	const FGameplayTag PunctureTag = FGameplayTag::RequestGameplayTag(FName("Damage.Puncture"), false);
	const FGameplayTag SlashTag = FGameplayTag::RequestGameplayTag(FName("Damage.Slash"), false);

	TArray<FBossAttackDefinition>& Attacks = CombatComponent->Attacks;
	Attacks.Reset();

	// ---------- Corto alcance ----------
	{
		FBossAttackDefinition Definition = Attack("SingleAttack", TEXT("BossSingleAttack_Montage"), EBossAttackRange::Short, 0.f, 300.f, 14.f, 2.5f,
			{ Weapon(0.2f, 0.65f) });
		Definition.Weight = 1.2f;
		Definition.RecoveryTime = 0.7f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("DoubleAttack", TEXT("BossDoubleAttack2_Montage"), EBossAttackRange::Short, 0.f, 300.f, 12.f, 4.f,
			{ Weapon(0.15f, 0.8f, true) });
		Definition.TrackingEndTime = 0.2f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("BackDoubleAttack", TEXT("BossBackDoubleAttack_Montage"), EBossAttackRange::Short, 0.f, 300.f, 12.f, 5.f,
			{ Weapon(0.15f, 0.8f, true) });
		Definition.Weight = 0.8f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("SpinAttack", TEXT("BossSpinAttack_Montage"), EBossAttackRange::Short, 0.f, 350.f, 16.f, 7.f,
			{ Weapon(0.15f, 0.85f, true, 0.5f) });
		Definition.MaxAngle = 180.f;
		Definition.TrackingEndTime = 0.1f;
		Definition.bShowGroundTelegraph = true;
		Definition.TelegraphRadius = 330.f;
		Definition.TelegraphOffset = FVector::ZeroVector;
		Definition.RecoveryTime = 0.9f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("TripleHeavyAttack", TEXT("BossTripleHeavyAttack_Montage"), EBossAttackRange::Short, 0.f, 320.f, 18.f, 8.f,
			{ Weapon(0.12f, 0.88f, true, 0.45f) });
		Definition.DamageTypeTag = ImpactTag;
		Definition.TrackingEndTime = 0.15f;
		Definition.RecoveryTime = 1.1f;
		Definition.MinStage = 1;
		Attacks.Add(Definition);
	}
	// Giran hacia atrás por sí solos: sin tracking para no duplicar el giro.
	{
		FBossAttackDefinition Definition = Attack("TurningAttack", TEXT("BossTurningAttack1_Montage"), EBossAttackRange::Short, 0.f, 350.f, 15.f, 4.f,
			{ Weapon(0.15f, 0.75f) });
		Definition.MinAngle = 100.f;
		Definition.MaxAngle = 180.f;
		Definition.TrackingEndTime = 0.f;
		Definition.Weight = 2.f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("TurningSingleAttack", TEXT("BossTurningAttackSingle_Montage"), EBossAttackRange::Short, 0.f, 330.f, 14.f, 3.5f,
			{ Weapon(0.15f, 0.65f) });
		Definition.MinAngle = 100.f;
		Definition.MaxAngle = 180.f;
		Definition.TrackingEndTime = 0.f;
		Definition.Weight = 2.f;
		Attacks.Add(Definition);
	}

	// ---------- Mediano alcance ----------
	{
		FBossAttackDefinition Definition = Attack("SpearStab", TEXT("BossSpearStab_Montage"), EBossAttackRange::Medium, 250.f, 650.f, 18.f, 4.f,
			{ Weapon(0.25f, 0.6f) });
		Definition.DamageTypeTag = PunctureTag;
		Definition.MaxAngle = 45.f;
		Definition.TrackingEndTime = 0.3f;
		Definition.RecoveryTime = 0.9f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("SpearCombo", TEXT("BossSpearComboAttack_Montage"), EBossAttackRange::Medium, 250.f, 650.f, 12.f, 7.f,
			{ Weapon(0.15f, 0.85f, true, 0.35f) });
		Definition.DamageTypeTag = PunctureTag;
		Definition.MaxAngle = 45.f;
		Definition.TrackingEndTime = 0.2f;
		Definition.MinStage = 1;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("LongAttack", TEXT("BossLongAttack_Montage"), EBossAttackRange::Medium, 300.f, 800.f, 18.f, 5.f,
			{ Weapon(0.2f, 0.75f) });
		Definition.MaxAngle = 50.f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("LongCombo", TEXT("BossLongComboAttack_Montage"), EBossAttackRange::Medium, 300.f, 800.f, 13.f, 9.f,
			{ Weapon(0.15f, 0.88f, true) });
		Definition.MaxAngle = 50.f;
		Definition.MinStage = 1;
		Definition.RecoveryTime = 1.f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("DodgeAttack", TEXT("BossDodgeAttack_Montage"), EBossAttackRange::Medium, 150.f, 600.f, 15.f, 6.f,
			{ Weapon(0.3f, 0.85f) });
		Definition.TrackingEndTime = 0.35f;
		Definition.Weight = 0.7f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("DodgeLeftAttack", TEXT("BossDogeLAttack_Montage"), EBossAttackRange::Medium, 150.f, 600.f, 15.f, 6.f,
			{ Weapon(0.3f, 0.85f) });
		Definition.TrackingEndTime = 0.35f;
		Definition.Weight = 0.6f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("DodgeRightAttack", TEXT("BossDogeRAttack_Montage"), EBossAttackRange::Medium, 150.f, 600.f, 15.f, 6.f,
			{ Weapon(0.3f, 0.85f) });
		Definition.TrackingEndTime = 0.35f;
		Definition.Weight = 0.6f;
		Attacks.Add(Definition);
	}

	// ---------- Largo alcance ----------
	{
		FBossAttackDefinition Definition = Attack("AirSlash", TEXT("BossAirSlashAttack_Montage"), EBossAttackRange::Long, 750.f, 2600.f, 16.f, 5.f,
			{ Projectile(0.4f) });
		Definition.DamageTypeTag = SlashTag;
		Definition.MaxAngle = 60.f;
		// Sigue apuntando hasta soltar el corte; después se esquiva de costado.
		Definition.TrackingEndTime = 0.38f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("AirSlashCombo", TEXT("BossAiSlashAttackCombo_Montage"), EBossAttackRange::Long, 900.f, 2600.f, 12.f, 9.f,
			{ Projectile(0.3f), Projectile(0.5f, 3, 30.f), Projectile(0.7f) });
		Definition.DamageTypeTag = SlashTag;
		Definition.MaxAngle = 60.f;
		Definition.TrackingEndTime = 0.68f;
		Definition.TrackingSpeed = 120.f;
		Definition.MinStage = 1;
		Definition.RecoveryTime = 1.f;
		Attacks.Add(Definition);
	}
	{
		FBossAttackDefinition Definition = Attack("JumpingSlam", TEXT("BossJumpingAttack_Montage"), EBossAttackRange::Long, 650.f, 1500.f, 24.f, 10.f,
			{ Weapon(0.4f, 0.7f), Sphere(0.5f, 0.62f, FVector(150.f, 0.f, -40.f), 330.f) });
		Definition.DamageTypeTag = ImpactTag;
		Definition.MaxAngle = 40.f;
		Definition.TrackingEndTime = 0.3f;
		Definition.bShowGroundTelegraph = true;
		Definition.TelegraphRadius = 330.f;
		Definition.TelegraphOffset = FVector(150.f, 0.f, 0.f);
		Definition.RecoveryTime = 1.2f;
		Attacks.Add(Definition);
	}

	// ---------- Habilidades ----------
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
	}

	CombatComponent->SetWeaponComponent(WeaponMesh);
	CeilingBreakComponent->OnStageChanged.AddUniqueDynamic(this, &ABossEnemy::HandleStageChanged);

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

	// Si el jugador lo ataca por sorpresa, la pelea empieza igual.
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
	LastDamageCauser = const_cast<AActor*>(DeathPayload.Instigator.Get());

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

	// Pasa a modo single node: el AnimBP ya no corre y se queda con la última pose.
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

void ABossEnemy::HandleStageChanged(const int32 NewStage)
{
	GetCharacterMovement()->MaxWalkSpeed = WalkSpeed + WalkSpeedBonusPerStage * NewStage;

	BP_OnStageChanged(NewStage);
}

void ABossEnemy::UpdateHealthBar() const
{
	if (!HealthBarWidget || !AbilitySystemComponent) return;

	const float MaxShield = AbilitySystemComponent->GetNumericAttribute(UVelmaraAttributeSet::GetMaxShieldAttribute());
	const float ShieldPercent = MaxShield > 0.f
		? FMath::Clamp(AbilitySystemComponent->GetNumericAttribute(UVelmaraAttributeSet::GetShieldAttribute()) / MaxShield, 0.f, 1.f)
		: 0.f;

	HealthBarWidget->ReceivePercentagesFromBoss(GetHealthPercent(), ShieldPercent);
}

void ABossEnemy::HideHealthBar()
{
	if (HealthBarWidget)
	{
		HealthBarWidget->RemoveFromParent();
		HealthBarWidget = nullptr;
	}
}
