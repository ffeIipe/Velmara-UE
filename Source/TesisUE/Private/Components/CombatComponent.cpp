#include "Components/CombatComponent.h"

#include "Camera/CameraActor.h"
#include "Components/AttributeComponent.h"
#include "Components/TimelineComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/ExtraMovementComponent.h"
#include "Curves/CurveFloat.h"
#include "DataAssets/EntityData.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "SceneEvents/NewGameModeBase.h"

#include "Enemy/Paladin.h"
#include "Interfaces/HitInterface.h"
#include "Interfaces/CharacterState.h"
#include "Interfaces/FormInterface.h"

#include "Player/PlayerMain.h"
#include "Items/Weapons/Sword.h"
#include "Subsystems/EffectsManager.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BufferAttackTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BufferAttackTimeline"));

	SoftLockTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("SoftLockTimeline"));
	
	LaunchCharacterTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("LaunchCharacterTimeline"));
}

void UCombatComponent::InitializeValues(const FCombatData& CombatData)
{
	UE_LOG(LogTemp, Warning, TEXT("Combat Component values assigned by CombatData"));

	SoftLockDistance = CombatData.SoftLockDistance;
	SoftLockRadius = CombatData.SoftLockRadius;
	TrackTargetRadius = CombatData.TrackTargetRadius;
	SoftLockCurve = CombatData.SoftLockCurve;

	BufferAttackDistance = CombatData.BufferAttackDistance;
	BufferCurve = CombatData.BufferCurve;
	
	LightAttackCombo = CombatData.LightAttackCombo;
	JumpAttackCombo = CombatData.JumpAttackCombo;
	HeavyAttackCombo = CombatData.HeavyAttackCombo;
	
	BlockMontage = CombatData.BlockMontage;
	FinisherMontage = CombatData.FinisherMontage;
	CrasherMontage = CombatData.CrasherMontage;
	LaunchMontage = CombatData.LaunchMontage;
	HitReactMontage = CombatData.HitReactMontage;
}

void UCombatComponent::ResetState()
{
	if (OwningCharacter->GetCharacterMovement()->IsFlying())
	{
		OwningCharacter->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	}

	if (const TArray<ECharacterActions> CharacterActionsToCheck = { ECharacterActions::ECA_Block }; !CharacterStateComponent->IsActionEqualToAny(CharacterActionsToCheck))
	{
		CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Nothing);
	}

	ResetLightAttackStats();
	ResetJumpAttackStats();
	ResetHeavyAttackStats();

	if (SpectralAttacks) SpectralAttacks->Execute_ResetSpectralAttack(GetOwner());

	if (OnAttackEnd.IsBound()) OnAttackEnd.Broadcast(); //this is called by AN_ResetState during the anim event

	ExtraMovementComponent->bIsSaveDodge = false;
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetComponentTickEnabled(false);
	
	EntityOwner = Cast<AEntity>(GetOwner());
	
	OwningCharacter = Cast<ACharacter>(GetOwner());

	SpectralAttacks = Cast<IFormInterface>(GetOwner());

	CharacterStateComponent = EntityOwner->GetCharacterStateComponent();

	ExtraMovementComponent = GetOwner()->GetComponentByClass<UExtraMovementComponent>();

	OwnerController = EntityOwner->GetController();
	
	if (BufferCurve)
	{
		FOnTimelineFloat ProgressAttackFunction;
		ProgressAttackFunction.BindUFunction(this, FName("UpdateAttackBuffer"));
		BufferAttackTimeline->AddInterpFloat(BufferCurve, ProgressAttackFunction);
	}

	if (SoftLockCurve)
	{
		FOnTimelineFloat ProgressSoftLockFunction;
		ProgressSoftLockFunction.BindUFunction(this, FName("UpdateSoftLockOn"));
		SoftLockTimeline->AddInterpFloat(SoftLockCurve, ProgressSoftLockFunction);

		FOnTimelineFloat ProgressLaunchUpFunction;
		ProgressLaunchUpFunction.BindUFunction(this, FName("UpdateLaunchCharacterUp"));
		LaunchCharacterTimeline->AddInterpFloat(SoftLockCurve, ProgressLaunchUpFunction);
	}
}

void UCombatComponent::LightAttack(const int AttackIndex)
{
	if (CanAttack())
	{
		StopAttackBufferEvent();
		StartAttackBufferEvent(BufferAttackDistance);
		CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Attack);
		SoftLockOn();

		if (ASword* TempSword = GetCurrentSword())
		{
			OwningCharacter->PlayAnimMontage(TempSword->LightAttackCombo[AttackIndex]);
			LightAttackIndex++;

			if (LightAttackIndex >= TempSword->LightAttackCombo.Num())
			{
				LightAttackIndex = 0;
			}
		}
		else
		{
			OwningCharacter->PlayAnimMontage(LightAttackCombo[AttackIndex]);
			LightAttackIndex++;

			if (LightAttackIndex >= LightAttackCombo.Num())
			{
				LightAttackIndex = 0;
			}
		}
	}
}

void UCombatComponent::JumpAttack(const int AttackIndex)
{
	if (CanAttack())
	{
		StopAttackBufferEvent();
		CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Attack);
		SoftLockOn();

		if (ASword* TempSword = GetCurrentSword())
		{
			OwningCharacter->PlayAnimMontage(TempSword->JumpAttackCombo[AttackIndex]);
			JumpAttackIndex++;

			if (JumpAttackIndex >= TempSword->JumpAttackCombo.Num())
			{
				JumpAttackIndex = 0;
				OwningCharacter->PlayAnimMontage(CrasherMontage, 1.f);
				bIsLaunched = false;
			}
		}
		else if (!JumpAttackCombo.IsEmpty())
		{
			OwningCharacter->PlayAnimMontage(JumpAttackCombo[AttackIndex]);
			JumpAttackIndex++;

			if (JumpAttackIndex >= JumpAttackCombo.Num())
			{
				JumpAttackIndex = 0;
				OwningCharacter->PlayAnimMontage(CrasherMontage, 1.f);
				bIsLaunched = false;
			}
		}
		else
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE,3.f,FColor::Red,"Jump Animations not valid!");
		}
	}
}

void UCombatComponent::PerformComboStarter(const int AttackIndex)
{
	if (CanAttack())
	{
		ComboExtenderIndex = AttackIndex;
		StopAttackBufferEvent();
		StartAttackBufferEvent(BufferAttackDistance);
		CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Attack);

		OwningCharacter->PlayAnimMontage(ComboStarterAttack[AttackIndex - 1]);

		bIsSaveHeavyAttack = false;
		bIsSaveLightAttack = false;

		SoftLockOn();
	}
}

void UCombatComponent::PerformComboExtender(const int AttackIndex)
{
	if (CanAttack())
	{
		StopAttackBufferEvent();
		StartAttackBufferEvent(BufferAttackDistance);
		CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Attack);

		OwningCharacter->PlayAnimMontage(ComboExtenderAttack[AttackIndex - 1]);

		ResetLightAttackStats();
		ResetHeavyAttackStats();
		SoftLockOn();
	}
}

void UCombatComponent::HeavyAttack(const int AttackIndex)
{
	if (CanAttack())
	{
		StopAttackBufferEvent();
		StartAttackBufferEvent(BufferAttackDistance);
		CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Attack);
		SoftLockOn();

		if (ASword* TempSword = GetCurrentSword())
		{
			OwningCharacter->PlayAnimMontage(TempSword->HeavyAttackCombo[AttackIndex]);
			HeavyAttackIndex++;

			if (HeavyAttackIndex >= TempSword->HeavyAttackCombo.Num())
			{
				HeavyAttackIndex = 0;
			}
		}
		else
		{
			if (HeavyAttackCombo.IsEmpty()) return;
			
			OwningCharacter->PlayAnimMontage(HeavyAttackCombo[AttackIndex]);
			HeavyAttackIndex++;

			if (HeavyAttackIndex >= HeavyAttackCombo.Num())
			{
				HeavyAttackIndex = 0;
			}
		}
	}
}

void UCombatComponent::StartAttackBufferEvent(float BufferAmount)
{
	if (BufferAttackTimeline)
	{
		BufferAttackTimeline->PlayFromStart();
	}
}

void UCombatComponent::StopAttackBufferEvent()
{
	if (BufferAttackTimeline)
	{
		BufferAttackTimeline->Stop();
	}
}

void UCombatComponent::UpdateAttackBuffer(const float Alpha)
{
	UpdateBuffer(Alpha, BufferAttackDistance * BufferMultiplier);
}

void UCombatComponent::UpdateBuffer(const float Alpha, const float BufferDistance)
{
	const FVector CurrentLocation = GetOwner()->GetActorLocation();
	const FVector ForwardVector = GetOwner()->GetActorForwardVector();

	const FVector TargetLocation = FMath::Lerp(CurrentLocation, CurrentLocation + (ForwardVector * BufferDistance), Alpha);

	GetOwner()->SetActorLocation(TargetLocation, true);
}

void UCombatComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (GEngine) GEngine->AddOnScreenDebugMessage(46,-1.f, FColor::Green, FString("Combat Component is ticking..."));
	
	if (bIsHardLocking && !CurrentHardLockTarget->IsHidden() && CurrentHardLockTarget->GetAttributeComponent()->IsAlive())
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE,-1.f, FColor::Purple, FString("Valid Hard Lock Target"));
		OwnerController->SetControlRotation(UKismetMathLibrary::RInterpTo(
			OwnerController->GetControlRotation(),
			UKismetMathLibrary::FindLookAtRotation(
				EntityOwner->GetFollowCamera()->GetActorLocation(),
				CurrentHardLockTarget->GetActorLocation() + FVector(0.0f, 0.0f, 50.0f)
				),
			DeltaTime,
			50.f
			));

		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE,-1.f, FColor::Cyan, FString::SanitizeFloat(HardLockTargetIndex));
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE,-1.f, FColor::Emerald, FString::SanitizeFloat(HardLockTargets.Num()));
		
		//RotateTowardsHardLockTarget(CurrentHardLockTarget, DeltaTime);
	}
	else PickHardLockTarget();
}

void UCombatComponent::ResetLightAttackStats()
{
	LightAttackIndex = 0;
	bIsSaveLightAttack = false;
}

void UCombatComponent::ResetJumpAttackStats()
{
	JumpAttackIndex = 0;
	bIsLaunched = false;
}

void UCombatComponent::ResetHeavyAttackStats()
{
	HeavyAttackIndex = 0;
	bIsSaveHeavyAttack = false;
}

void UCombatComponent::SoftLockOn()
{
	if (!OwningCharacter) return;

	if (!bIsHardLocking)
	{
		const FVector Start = GetOwner()->GetActorLocation();
		const FVector End = (OwningCharacter->GetLastMovementInputVector() * SoftLockDistance) + GetOwner()->GetActorLocation();

		if (APaladin* Enemy = Cast<APaladin>(SphereTraceForEnemies(Start, End)))
		{
			if (Enemy->GetCharacterStateComponent()->GetCurrentCharacterState().Action != ECharacterActions::ECA_Dead)
			{
				SoftLockTarget = Enemy;
				RotationToTarget();
			}
		}
		else SoftLockTarget = nullptr;
	}
	else
	{
		SoftLockTarget = CurrentHardLockTarget;
	}
}

void UCombatComponent::ValidateWall()
{
	TArray<AActor*> ObjectsToIgnore;
	ObjectsToIgnore.Add(GetOwner());

	FHitResult Hit;

	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetOwner());
	QueryParams.bTraceComplex = true;
	QueryParams.bReturnPhysicalMaterial = false;

	const FVector Start = GetOwner()->GetActorLocation();
	const FVector End = Start + (GetOwner()->GetActorForwardVector() * 100.f);

	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECC_GameTraceChannel4,
		QueryParams
	);

	if (bHit && OnWallHit.IsBound())
	{
		OnWallHit.Broadcast(Hit);
	}

	DrawDebugLine(
		GetWorld(),
		Start,
		bHit ? Hit.ImpactPoint : End,
		FColor::Red,
		false,
		2.0f,
		0,
		1.0f
	);
}

void UCombatComponent::RotationToTarget()
{
	if (SoftLockTarget)
	{
		SoftLockTimeline->PlayFromStart();
	}
	else
	{
		SoftLockOn();
	}
}

void UCombatComponent::UpdateSoftLockOn(float Alpha)
{
	const FVector Start = GetOwner()->GetActorLocation();

	if (!SoftLockTarget) return;
	const FVector End = SoftLockTarget->GetActorLocation();

	FRotator NewRotation = FRotator(
		GetOwner()->GetActorRotation().Pitch,
		UKismetMathLibrary::FindLookAtRotation(Start, End).Yaw,
		UKismetMathLibrary::FindLookAtRotation(Start, End).Roll
	);

	NewRotation = FMath::Lerp(GetOwner()->GetActorRotation(), NewRotation, Alpha);

	GetOwner()->SetActorRotation(NewRotation);
}

void UCombatComponent::UpdateLaunchCharacterUp(const float Alpha)
{
	const FVector TargetLocation = FMath::Lerp(CurrentLocationLaunch, CurrentLocationLaunch + (UpVectorLaunch * 300.f), Alpha);
	GetOwner()->SetActorLocation(TargetLocation, true);
}

void UCombatComponent::Block()
{
	if (CharacterStateComponent->GetCurrentCharacterState().State != ECharacterStates::ECS_Unequipped)
	{
		OwningCharacter->PlayAnimMontage(BlockMontage, 1.f, FName("BlockIdle"));
		CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Block);
	}
}

void UCombatComponent::ReceiveBlock()
{
	OwningCharacter->PlayAnimMontage(BlockMontage, 1.f, FName("BlockReact"));
}

void UCombatComponent::ReleaseBlock()
{
	OwningCharacter->StopAnimMontage(BlockMontage);
	CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Nothing);
}

void UCombatComponent::Execute()
{
	/*if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, FColor::Red, FString("Execute Finisher"));*/

	if (CharacterStateComponent->IsStateEqualToAny({ ECharacterStates::ECS_Unequipped })) return;

	if (CharacterStateComponent->IsActionEqualToAny({ 
		ECharacterActions::ECA_Dead,
		ECharacterActions::ECA_Finish,
		ECharacterActions::ECA_Stun
		})) return;

	if (EntityOwner->GetCharacterMovement()->MovementMode == MOVE_Falling || EntityOwner->GetCharacterMovement()->MovementMode == MOVE_Flying) return;

	if (AEntity* Enemy = SphereTraceForEnemies(OwningCharacter->GetActorLocation(), OwningCharacter->GetActorLocation() + OwningCharacter->GetActorForwardVector() * 40.f))
	{
		if (Enemy->GetClass()->ImplementsInterface(UHitInterface::StaticClass()))
		{
			if (IHitInterface::Execute_CanBeFinished(Enemy) && Enemy->GetCharacterStateComponent()->GetCurrentCharacterState().Action != ECharacterActions::ECA_Dead)
			{
				CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Finish);

				const FVector Start = OwningCharacter->GetActorLocation();
				const FVector End = Enemy->GetActorLocation();
				const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(Start, End);
				OwningCharacter->SetActorRotation(LookAtRotation);

				OwningCharacter->PlayAnimMontage(FinisherMontage, 1.0f);

				Cast<ANewGameModeBase>(UGameplayStatics::GetGameMode(GetWorld()))->SetEnemiesAIEnabled(false);
			}
		}
	}
}

void UCombatComponent::GetDirectionalReact(const FVector& ImpactPoint)
{
	const FVector Forward = GetOwner()->GetActorForwardVector();
	const FVector ToHit = (ImpactPoint - GetOwner()->GetActorLocation()).GetSafeNormal();

	const double CosAngle = FVector::DotProduct(Forward, ToHit);

	double Angle = FMath::Acos(CosAngle);

	Angle = FMath::RadiansToDegrees(Angle);

	if (const FVector CrossProduct = FVector::CrossProduct(Forward, ToHit); CrossProduct.Z < 0)
	{
		Angle *= -1.f;
	}

	FName Section("FromBack");

	if (Angle >= -45.f && Angle < 45.f)
	{
		Section = FName("FromFront");
	}

	else if (Angle >= -135.f && Angle < -45.f)
	{
		Section = FName("FromLeft");
	}

	else if (Angle >= 45.f && Angle < 135.f)
	{
		Section = FName("FromRight");
	}

	OwningCharacter->PlayAnimMontage(HitReactMontage, 1.f, Section);
}

void UCombatComponent::HitReactJumpToSection(FName Section)
{
	if (HitReactMontage)
	{
		OwningCharacter->PlayAnimMontage(HitReactMontage, 1.f, Section);
	}
}

void UCombatComponent::LaunchCharacterUp()
{
	if (const IHitInterface* Paladin = Cast<IHitInterface>(SoftLockTarget); SoftLockTarget && Paladin->Execute_IsLaunchable(SoftLockTarget))
	{
			
		OwningCharacter->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
		bIsLaunched = true;

		StartLaunchingUp();

		Paladin->Execute_LaunchUp(SoftLockTarget, FVector(GetOwner()->GetActorLocation()));

		if (UEffectsManager* EffectsManager = GetWorld()->GetSubsystem<UEffectsManager>())
		{
			EffectsManager->TimeWarp(ETimeWarpPreset::ETWP_Crasher);
		}
	}
}

void UCombatComponent::StartLaunchingUp()
{
	CurrentLocationLaunch = GetOwner()->GetActorLocation();
	UpVectorLaunch = GetOwner()->GetActorUpVector();

	LaunchCharacterTimeline->PlayFromStart();
}

void UCombatComponent::Crasher()
{
	SoftLockOn();
	OwningCharacter->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Attack);

	const FVector Start = GetOwner()->GetActorLocation();
	const FVector End = Start + FVector(0.f, 0.f, -100000.f);

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldDynamic));

	TArray<AActor*> ObjectsToIgnore;
	ObjectsToIgnore.Add(GetOwner());

	FHitResult Hit;

	const bool bHit = UKismetSystemLibrary::LineTraceSingleForObjects(
		GetWorld(),
		Start,
		End,
		ObjectTypes,
		false,
		ObjectsToIgnore,
		EDrawDebugTrace::None,
		Hit,
		true
	);

	if (bHit)
	{
		OwningCharacter->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);

		const FVector NewLocation = FVector(Hit.ImpactPoint.X, Hit.ImpactPoint.Y, Hit.ImpactPoint.Z + 50.f);
		GetOwner()->SetActorLocation(NewLocation);
	}
}

void UCombatComponent::ToggleHardLock()
{
	if (bIsHardLocking)
	{
		bIsHardLocking = false;
		SetComponentTickEnabled(bIsHardLocking);
		return;
	}
	
	PickHardLockTarget() ? bIsHardLocking = true : bIsHardLocking = false;
	SetComponentTickEnabled(bIsHardLocking);
}

bool UCombatComponent::PickHardLockTarget()
{
	if (HardLockTargets = GetHardLockTargets(HardLockRadius); HardLockTargets.Num() > 0)
	{
		HardLockTargetIndex = 0;
		CurrentHardLockTarget = HardLockTargets[HardLockTargetIndex];

		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE,3.f, FColor::Yellow, FString("Target picked..."));
		return true;
	}

	bIsHardLocking = false;
	HardLockTargets.Empty();
	CurrentHardLockTarget = nullptr;
	SoftLockTarget = nullptr; //I could use one variable to handle this, I mean this can use one target var and be used by both (soft & hard lock target system)
	SetComponentTickEnabled(false);
	
	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE,3.f, FColor::Red, FString("Failed to pick target..."));
	return false;
}

void UCombatComponent::ChangeHardLockTarget()
{
	if (!bIsHardLocking) return;

	if (HardLockTargets.Num() > 0)
	{
		HardLockTargetIndex++;
		if (HardLockTargetIndex >= HardLockTargets.Num())
		{
			HardLockTargetIndex = 0;
		}
		
		CurrentHardLockTarget = HardLockTargets[HardLockTargetIndex];
	}
	else HardLockTargetIndex = 0;
}

bool UCombatComponent::IsValidAndAlive(const AEntity* TargetToCheck)
{
	if (TargetToCheck != nullptr)
	{
		if (!TargetToCheck->GetAttributeComponent()->IsAlive() || TargetToCheck->IsHidden())
		{
			return PickHardLockTarget();
		}
	}
	
	return false;
}

TArray<AEntity*> UCombatComponent::GetHardLockTargets(const float Radius) const
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

	TArray<AEntity*> FinalEntities;
	if (Hits.Num() > 0)
	{
		for (AActor* Target : Hits)
		{
			if (AEntity* Entity = Cast<AEntity>(Target); !Entity->IsHidden() && Entity->GetAttributeComponent()->IsAlive())
			{
				FinalEntities.Add(Entity);
			}
		}
	}
	
	return FinalEntities;
}

void UCombatComponent::RotateTowardsHardLockTarget(const AEntity* HardLockTarget, float DeltaTime) const
{
	const FVector Start = GetOwner()->GetActorLocation();

	if (!HardLockTarget) return;
	const FVector End = HardLockTarget->GetActorLocation();

	FRotator NewRotation = FRotator(
		GetOwner()->GetActorRotation().Pitch,
		UKismetMathLibrary::FindLookAtRotation(Start, End).Yaw,
		UKismetMathLibrary::FindLookAtRotation(Start, End).Roll
	);

	NewRotation = FMath::Lerp(GetOwner()->GetActorRotation(), NewRotation, DeltaTime);
	
	GetOwner()->SetActorRotation(NewRotation);
}

AEntity* UCombatComponent::SphereTraceForEnemies(const FVector& Start, const FVector& End)
{
	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_Pawn));

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());

	FHitResult ResultHit;

	UKismetSystemLibrary::SphereTraceSingleForObjects(
		GetWorld(),
		Start,
		End,
		TrackTargetRadius,
		ObjectTypes,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		ResultHit,
		true
	);

	return Cast<AEntity>(ResultHit.GetActor());
}

bool UCombatComponent::CanAttack()
{
	return (!CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Attack, ECharacterActions::ECA_Dodge }) &&
			!CharacterStateComponent->IsStateEqualToAny({ ECharacterStates::ECS_Unequipped }) &&
			!CharacterStateComponent->IsFormEqualToAny({ECharacterForm::ECF_Spectral}));
}

bool UCombatComponent::CheckDistance(const FVector& Origin, const FVector& Target, const float DistanceToCheck)
{
	if (const float Distance = FVector::Distance(Target, Origin); Distance < DistanceToCheck)
		return true;
	
	return false;
}

void UCombatComponent::Input_Attack()
{
	if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Stun, ECharacterActions::ECA_Dead }) || EntityOwner->IsEquipping()) return;
	
	ExtraMovementComponent->bIsSaveDodge = false;

	bIsSaveHeavyAttack = false;

	if (!CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Attack, ECharacterActions::ECA_Dodge }))
	{
		LightAttackEvent();
	}
	else
	{
		bIsSaveLightAttack = true;
	}
}

void UCombatComponent::Input_HeavyAttack()
{
	if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Stun, ECharacterActions::ECA_Dead }) || EntityOwner->IsEquipping()) return;

	ExtraMovementComponent->bIsSaveDodge = false;

	bIsSaveLightAttack = false;

	if (!CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Attack, ECharacterActions::ECA_Dodge }))
	{
		if (!bIsLaunched || OwningCharacter->GetCharacterMovement()->MovementMode != EMovementMode::MOVE_Falling)
		{
			HeavyAttackEvent();
		}
		else
		{	
			OwningCharacter->PlayAnimMontage(CrasherMontage);
		}	
	}
	else
	{
		bIsSaveHeavyAttack = true;
	}
}

void UCombatComponent::Input_Launch()
{
	if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Stun, ECharacterActions::ECA_Dead }) || EntityOwner->IsEquipping()) return;

	if (CanAttack() && bIsHardLocking && EntityOwner->GetExtraMovementComponent()->IsMovingBackwards())
	{
		SoftLockOn();
		if (SoftLockTarget && CheckDistance(GetOwner()->GetActorLocation(), SoftLockTarget->GetActorLocation(), 1000.f)) //cm
		{
			CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Attack);
			OwningCharacter->PlayAnimMontage(LaunchMontage);
		}
	}
}

void UCombatComponent::Input_Block()
{
	if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Stun, ECharacterActions::ECA_Dead }) || EntityOwner->IsEquipping()) return;

	Block();
}

void UCombatComponent::Input_ReleaseBlock()
{
	if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Stun, ECharacterActions::ECA_Dead }) || EntityOwner->IsEquipping()) return;

	ReleaseBlock();
}

void UCombatComponent::Input_Execute()
{
	if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Stun, ECharacterActions::ECA_Dead }) || EntityOwner->IsEquipping()) return;

	Execute();
}

void UCombatComponent::LightAttackEvent()
{
	if (CharacterStateComponent->GetCurrentCharacterState().Form == ECharacterForm::ECF_Spectral)
	{
		SpectralAttacks->Execute_PerformSpectralAttack(GetOwner());
	}
	else if (bIsLaunched 
		|| OwningCharacter->GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Flying 
		|| OwningCharacter->GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling)
	{
		JumpAttack(JumpAttackIndex);
		ValidateWall();
	}
	else
	{
		LightAttack(LightAttackIndex);
		ValidateWall();
	}
}

void UCombatComponent::HeavyAttackEvent()
{
	if (CharacterStateComponent->GetCurrentCharacterState().Form == ECharacterForm::ECF_Spectral)
	{
		SpectralAttacks->Execute_PerformSpectralBarrier(GetOwner());
	}
	else if (OwningCharacter->GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling)
	{
		OwningCharacter->PlayAnimMontage(CrasherMontage);
	}
	else
	{
		HeavyAttack(HeavyAttackIndex);
	}
}

void UCombatComponent::SaveLightAttackEvent()
{
	if (bIsSaveLightAttack)
	{
		bIsSaveLightAttack = false;
		CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Nothing);
		LightAttackEvent();
	}
	else if (bIsSaveLightAttack && LightAttackIndex > 0)
	{
		if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Attack }))
		{
			CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Nothing);
			PerformComboStarter(LightAttackIndex);
		}
	}
}

void UCombatComponent::SaveHeavyAttackEvent()
{
	if (bIsSaveHeavyAttack)
	{
		bIsSaveHeavyAttack = false;
		if (!bIsLaunched)
		{
			if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Attack }))
			{
				CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Nothing);
				HeavyAttackEvent();
			}
			else
			{
				HeavyAttackEvent();
			}
		}
		else
		{
			OwningCharacter->PlayAnimMontage(CrasherMontage);
		}
	}
	else if (bIsSaveHeavyAttack && ComboExtenderIndex > 0)
	{
		if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Attack }))
		{
			CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Nothing);
		}
		else if (ComboExtenderAttack.Num() >= ComboExtenderIndex)
		{
			PerformComboExtender(ComboExtenderIndex);
			ComboExtenderIndex++;
		}
	}
}

void UCombatComponent::ResetAttackSave()
{
	bIsSaveLightAttack = false;
	bIsSaveHeavyAttack = false;
}

ASword* UCombatComponent::GetCurrentSword()
{
	if (const APlayerMain* PlayerRef = Cast<APlayerMain>(GetOwner()))
	{
		if (const UInventoryComponent* InventoryComp = PlayerRef->GetComponentByClass<UInventoryComponent>())
		{
			return Cast<ASword>(InventoryComp->EquippedItem);
		}
	}
	
	return nullptr;
}