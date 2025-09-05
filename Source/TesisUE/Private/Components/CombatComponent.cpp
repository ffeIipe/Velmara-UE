#include "Components/CombatComponent.h"

#include "InputActionValue.h"
#include "Components/CharacterStateComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "DataAssets/EntityData.h"

#include "Interfaces/AnimatorProvider.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "SceneEvents/NewGameModeBase.h"
#include "Interfaces/CameraProvider.h"
#include "Interfaces/CharacterMovementProvider.h"
#include "Interfaces/CharacterStateProvider.h"
#include "Interfaces/OwnerUtilsInterface.h"
#include "Interfaces/CombatTargetInterface.h"
#include "Interfaces/ControllerProvider.h"
#include "Interfaces/Weapon/WeaponProvider.h"
#include "Items/Weapons/Sword.h"
#include "Player/CharacterWeaponStates.h"
#include "Subsystems/EffectsManager.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BufferAttackTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BufferAttackTimeline"));

	SoftLockTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("SoftLockTimeline"));
	
	LaunchCharacterTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("LaunchCharacterTimeline"));

	BufferBackwardsTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BufferBackwardsTimeline"));
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
	
	BlockMontage = CombatData.BlockMontage;
	FinisherMontage = CombatData.FinisherMontage;
	CrasherMontage = CombatData.CrasherMontage;
	LaunchMontage = CombatData.LaunchMontage;
	HitReactMontage = CombatData.HitReactMontage;
}

void UCombatComponent::ResetState()
{
	if (OwnerUtils->IsFlying())
	{
		OwnerUtils->SetMovementMode(MOVE_Falling);
	}

	if (const TArray CharacterActionsToCheck = { ECharacterActionsStates::ECAS_Block }; !CharacterStateProvider->IsActionStateEqualToAny(CharacterActionsToCheck))
	{
		CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Nothing);
	}

	ResetLightAttackStats();
	ResetJumpAttackStats();
	ResetHeavyAttackStats();

	// if (SpectralAttacks) SpectralAttacks->Execute_ResetSpectralAttack(GetOwner());

	if (OnAttackEnd.IsBound()) OnAttackEnd.Broadcast(); //this is called by AN_ResetState during the anim event
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetComponentTickEnabled(false);
	
	OwnerUtils = GetOwner();
	ControllerProvider = GetOwner();
	OwnerController = ControllerProvider->GetEntityController();
	CharacterStateProvider = GetOwner();
	CharacterMovementProvider = GetOwner();
	CameraProvider = GetOwner();
	AnimatorProvider = GetOwner();
	
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

		FOnTimelineFloat ProgressBufferBackwardsFunction;
		ProgressBufferBackwardsFunction.BindUFunction(this, FName("UpdateBufferBackwards"));
		BufferBackwardsTimeline->AddInterpFloat(SoftLockCurve, ProgressBufferBackwardsFunction);
	}
}

void UCombatComponent::LightAttack(const int AttackIndex)
{
	if (!CanAttack()) return;
	
	StopAttackBufferEvent();
	StartAttackBufferEvent();
		
	CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Attack);
	SoftLockOn();
		
	if (const TScriptInterface<IMeleeWeapon> IsMelee = GetCurrentWeapon().GetObject())
	{
		/*if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.0f, FColor::Orange, "Valid Melee and proceeding to perform light attack");*/
		IsMelee->PerformLightAttack(AttackIndex);
		LightAttackIndex++;
				
		if (LightAttackIndex >= IsMelee->GetLightAttackComboMaxIndex())
		{
			LightAttackIndex = 0;
		}
	}
}

void UCombatComponent::JumpAttack(const int AttackIndex)
{
	if (CanAttack())
	{
		StopAttackBufferEvent();
		StartAttackBufferEvent();	
		CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Attack);
		SoftLockOn();

		if (GetCurrentWeapon())
		{
			if (IMeleeWeapon* IsMelee = Cast<IMeleeWeapon>(GetCurrentWeapon().GetObject()))
			{
				IsMelee->PerformJumpAttack(AttackIndex);
				JumpAttackIndex++;
				
				if (JumpAttackIndex >= IsMelee->GetJumpAttackComboMaxIndex())
				{
					JumpAttackIndex = 0;
				}
			}
			else
			{
				WeaponProvider->GetWeaponEquipped()->UsePrimaryAttack();
			}
		}
	}
}

void UCombatComponent::PerformComboStarter(const int AttackIndex)
{
	if (CanAttack())
	{
		ComboExtenderIndex = AttackIndex;
		StopAttackBufferEvent();
		StartAttackBufferEvent();
		CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Attack);

		//CombatComponentOwner->PlayAnimMontage(ComboStarterAttack[AttackIndex - 1]);

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
		StartAttackBufferEvent();
		CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Attack);

		//CombatComponentOwner->PlayAnimMontage(ComboExtenderAttack[AttackIndex - 1]);

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
		StartAttackBufferEvent();
		CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Attack);
		SoftLockOn();
		
		if (IMeleeWeapon* IsMelee = Cast<IMeleeWeapon>(GetCurrentWeapon().GetObject()))
		{
			IsMelee->PerformLightAttack(AttackIndex);
			HeavyAttackIndex++;
				
			if (HeavyAttackIndex >= IsMelee->GetHeavyAttackComboMaxIndex())
			{
				HeavyAttackIndex = 0;
			}
		}
		else
		{
			WeaponProvider->GetWeaponEquipped()->UseSecondaryAttack();
		}
	}
}

void UCombatComponent::StartAttackBufferEvent()
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

void UCombatComponent::StartBufferBackwards()
{
	if (BufferBackwardsTimeline)
	{
		StopAttackBufferEvent();
		BufferBackwardsTimeline->PlayFromStart();
	}
}

void UCombatComponent::StopBufferBackwards()
{
	if (BufferBackwardsTimeline)
	{
		BufferBackwardsTimeline->Stop();
	}
}

void UCombatComponent::UpdateBufferBackwards(const float Alpha)
{
	UpdateBuffer(Alpha, -(10.f * BufferMultiplier));
}

void UCombatComponent::UpdateAttackBuffer(const float Alpha) const
{
	UpdateBuffer(Alpha, BufferAttackDistance * BufferMultiplier);
}

void UCombatComponent::UpdateBuffer(const float Alpha, const float BufferDistance) const
{
	const FVector CurrentLocation = GetOwner()->GetActorLocation();
	const FVector ForwardVector = GetOwner()->GetActorForwardVector();

	const FVector TargetLocation = FMath::Lerp(CurrentLocation, CurrentLocation + (ForwardVector * BufferDistance), Alpha);

	GetOwner()->SetActorLocation(TargetLocation, false);
}

void UCombatComponent::TickComponent(const float DeltaTime, const ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	if (bIsHardLocking && CombatTarget->IsAlive())
	{
		OwnerController->SetControlRotation(UKismetMathLibrary::RInterpTo(
			OwnerController->GetControlRotation(),
			UKismetMathLibrary::FindLookAtRotation(
				CameraProvider->GetCameraLocation(),
				CombatTarget->GetTargetActorLocation() + FVector(0.0f, 0.0f, 50.0f)
				),
			DeltaTime,
			50.f
			));

		/*if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE,-1.f, FColor::Cyan, FString::SanitizeFloat(CombatTargetIndex));
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE,-1.f, FColor::Emerald, FString::SanitizeFloat(CombatTargets.Num()));*/
		
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
	if (!OwnerUtils) return;

	if (!bIsHardLocking)
	{
		const FVector Start = GetOwner()->GetActorLocation();
		const FVector End = CharacterMovementProvider->GetLastMovementInputVector() * SoftLockDistance + GetOwner()->GetActorLocation();

		CombatTarget = SearchCombatTarget(Start, End);
		if (CombatTarget && CombatTarget->IsAlive())
		{
			RotationToTarget();
		}
		else CombatTarget = nullptr;
	}
}

void UCombatComponent::ValidateWall()
{
	if (!CanAttack()) return;
	
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
	if (CombatTarget)
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

	if (!CombatTarget) return;
	const FVector End = CombatTarget->GetTargetActorLocation();

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

void UCombatComponent::Input_Block(const FInputActionValue& Value)
{
	if (CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Stun, ECharacterActionsStates::ECAS_Dead })) return;

	Value.Get<bool>() && !CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Stun, ECharacterActionsStates::ECAS_Dead })
		? Block() : ReleaseBlock();
}

void UCombatComponent::Block() const
{
	if (CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Stun })) return;
	
	if (!CharacterStateProvider->IsWeaponStateEqualToAny({ECharacterWeaponStates::ECWS_Unequipped }))
	{
		AnimatorProvider->PlayAnimMontage(BlockMontage, 1.f, FName("BlockIdle"));
		CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Block);
	}
}

void UCombatComponent::ReceiveBlock() const
{
	AnimatorProvider->StopAnimMontage(BlockMontage);
	AnimatorProvider->PlayAnimMontage(BlockMontage, 1.f, FName("BlockReact"));
	CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Stun);

	if (UEffectsManager* EffectsManager = GetWorld()->GetSubsystem<UEffectsManager>())
	{
		EffectsManager->CameraShake(ECameraShakePreset::ECSP_SwordHit, GetOwner()->GetActorLocation());
	}
}

void UCombatComponent::ReleaseBlock() const
{
	AnimatorProvider->StopAnimMontage(BlockMontage);
	CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Nothing);
}

void UCombatComponent::Execute()
{
	if (CharacterStateProvider->IsWeaponStateEqualToAny({ ECharacterWeaponStates::ECWS_Unequipped })) return;

	if (CharacterStateProvider->IsActionStateEqualToAny({ 
		ECharacterActionsStates::ECAS_Dead,
		ECharacterActionsStates::ECAS_Finish,
		ECharacterActionsStates::ECAS_Stun
		})) return;

	if (OwnerUtils->IsFalling() || OwnerUtils->IsFlying()) return;

	CombatTarget = SearchCombatTarget(GetOwner()->GetActorLocation(),
	                                  GetOwner()->GetActorLocation() + GetOwner()->GetActorForwardVector() * 40.f);
	if (CombatTarget)
	{
		if (CombatTarget->CanBeFinished() && CombatTarget->IsAlive())
		{
			CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Finish);

			const FVector Start = GetOwner()->GetActorLocation();
			const FVector End = CombatTarget->GetTargetActorLocation();
			const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(Start, End);
			GetOwner()->SetActorRotation(LookAtRotation);

			AnimatorProvider->PlayAnimMontage(FinisherMontage);

			Cast<ANewGameModeBase>(UGameplayStatics::GetGameMode(GetWorld()))->SetEnemiesAIEnabled(false);
		}
	}
}

void UCombatComponent::LaunchCharacterUp()
{
	if (CombatTarget)
	{
		if (CombatTarget->IsLaunchable())
		{
			OwnerUtils->SetMovementMode(MOVE_Flying);
			bIsLaunched = true;

			StartLaunchingUp();
			CombatTarget->LaunchUp();

			if (UEffectsManager* EffectsManager = GetWorld()->GetSubsystem<UEffectsManager>())
			{
				EffectsManager->TimeWarp(ETimeWarpPreset::ETWP_Crasher);
			}
		} else if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Not launchable");
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
	OwnerUtils->IsFalling();
	CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Attack);

	const FVector Start = GetOwner()->GetActorLocation();
	const FVector End = Start + FVector(0.f, 0.f, -100000.f);

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldStatic));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECC_WorldDynamic));

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
		OwnerUtils->IsFalling();

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
	if (CombatTargets = GetCombatTargets(HardLockRadius); CombatTargets.Num() > 0)
	{
		CombatTargetIndex = 0;
		CombatTarget = CombatTargets[CombatTargetIndex];
		
		return true;
	}

	bIsHardLocking = false;
	CombatTargets.Empty();
	CombatTarget = nullptr;
	SetComponentTickEnabled(false);
	
	return false;
}

void UCombatComponent::ChangeHardLockTarget()
{
	if (!bIsHardLocking) return;

	if (CombatTargets.Num() > 0 && CombatTargetIndex < CombatTargets.Num())
	{
		CombatTarget = CombatTargets[CombatTargetIndex];
		CombatTargetIndex++;
		
		if (CombatTargetIndex >= CombatTargets.Num())
		{
			CombatTargetIndex = 0;
		}
	}
	else CombatTargetIndex = 0;
}

bool UCombatComponent::IsValidAndAlive(const TScriptInterface<ICombatTargetInterface>& TargetToCheck)
{
	if (TargetToCheck != nullptr)
	{
		if (!TargetToCheck->IsAlive())
		{
			return PickHardLockTarget();
		}
	}
	
	return false;
}

TArray<TScriptInterface<ICombatTargetInterface>> UCombatComponent::GetCombatTargets(const float Radius) const
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
	if (Hits.Num() > 0)
	{
		for (AActor* Target : Hits)
		{
			if (TScriptInterface<ICombatTargetInterface> CombatTargetFound = Target; CombatTargetFound->IsAlive())
			{
				FinalCombatTargets.Add(CombatTargetFound);
			}
		}
	}
	
	return FinalCombatTargets;
}

void UCombatComponent::RotateTowardsHardLockTarget(const TScriptInterface<ICombatTargetInterface>& HardLockTarget, float DeltaTime) const
{
	const FVector Start = GetOwner()->GetActorLocation();

	if (!HardLockTarget) return;
	const FVector End = HardLockTarget->GetTargetActorLocation();

	FRotator NewRotation = FRotator(
		GetOwner()->GetActorRotation().Pitch,
		UKismetMathLibrary::FindLookAtRotation(Start, End).Yaw,
		UKismetMathLibrary::FindLookAtRotation(Start, End).Roll
	);

	NewRotation = FMath::Lerp(GetOwner()->GetActorRotation(), NewRotation, DeltaTime);
	
	GetOwner()->SetActorRotation(NewRotation);
}

TScriptInterface<ICombatTargetInterface> UCombatComponent::SearchCombatTarget(const FVector& Start, const FVector& End)
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
		TrackTargetRadius,
		ObjectTypes,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		ResultHit,
		true
	);

	if (bHit)
	{
		return CombatTarget = ResultHit.GetActor();
	}
	return nullptr;
}

bool UCombatComponent::CanAttack() const
{
	return (!CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Attack, ECharacterActionsStates::ECAS_Dodge }) &&
			!CharacterStateProvider->IsWeaponStateEqualToAny({ ECharacterWeaponStates::ECWS_Unequipped }) &&
			!CharacterStateProvider->IsModeStateEqualToAny({ECharacterModeStates::ECMS_Spectral}));
}

bool UCombatComponent::CheckDistance(const FVector& Origin, const FVector& Target, const float DistanceToCheck)
{
	if (const float Distance = FVector::Distance(Target, Origin); Distance < DistanceToCheck)
		return true;
	
	return false;
}

bool UCombatComponent::IsBlocking() const
{
	return CharacterStateProvider->GetCurrentCharacterState().Action == ECharacterActionsStates::ECAS_Block;
}

void UCombatComponent::Input_Attack()
{
	if (CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Stun, ECharacterActionsStates::ECAS_Dead }) /*|| EntityOwner->IsEquipping()*/) return;
	
	// ExtraMovementComponent->bIsSaveDodge = false; TODO: OnInputAttack()

	bIsSaveHeavyAttack = false;

	if (!CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Attack, ECharacterActionsStates::ECAS_Dodge }))
	{
		AttackEvent();
	}
	else
	{
		bIsSaveLightAttack = true;
	}
}

void UCombatComponent::Input_HeavyAttack()
{
	if (CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Stun, ECharacterActionsStates::ECAS_Dead }) /*|| EntityOwner->IsEquipping()*/) return;

	// ExtraMovementComponent->bIsSaveDodge = false; TODO: OnInputHeavyAttack()

	bIsSaveLightAttack = false;

	if (!CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Attack, ECharacterActionsStates::ECAS_Dodge }))
	{
		if (!bIsLaunched || OwnerUtils->IsFalling())
		{
			HeavyAttackEvent();
		}
		else
		{	
			AnimatorProvider->PlayAnimMontage(CrasherMontage);
		}	
	}
	else
	{
		bIsSaveHeavyAttack = true;
	}
}

void UCombatComponent::Input_Launch()
{
	if (CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Stun, ECharacterActionsStates::ECAS_Dead }) /*|| EntityOwner->IsEquipping()*/) return;

	if (CanAttack() && bIsHardLocking && OwnerUtils->IsMovingBackwards())
	{
		SoftLockOn();
		if (CombatTarget && CheckDistance(GetOwner()->GetActorLocation(), CombatTarget->GetTargetActorLocation(), 1000.f)) //cm
		{
			CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Attack);
			AnimatorProvider->PlayAnimMontage(LaunchMontage);
		}
	}
}

void UCombatComponent::Input_Execute()
{
	if (CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Stun, ECharacterActionsStates::ECAS_Dead }) /*|| EntityOwner->IsEquipping()*/) return;

	Execute();
}

void UCombatComponent::AttackEvent()
{
	if (!GetCurrentWeapon()) return;
	
	
	if (TScriptInterface<IMeleeWeapon> MeleeWeapon = GetCurrentWeapon().GetObject())
	{
		if (bIsLaunched || OwnerUtils->IsFlying() || OwnerUtils->IsFalling())
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
	else
	{
		if (!bIsLaunched && !OwnerUtils->IsFalling() && !OwnerUtils->IsFlying())
			GetCurrentWeapon()->UsePrimaryAttack();
	}

	/*if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.0f, FColor::Red, GetCurrentWeapon().GetObject()->GetName());*/
}

void UCombatComponent::HeavyAttackEvent()
{
	if (CharacterStateProvider->IsModeStateEqualToAny({ECharacterModeStates::ECMS_Spectral}))
	{
		//SpectralAttacks->Execute_PerformSpectralBarrier(GetOwner());
	}
	else if (OwnerUtils->IsFalling())
	{
		AnimatorProvider->PlayAnimMontage(CrasherMontage);
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
		CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Nothing);
		AttackEvent();
	}
	else if (bIsSaveLightAttack && LightAttackIndex > 0)
	{
		if (CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Attack }))
		{
			CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Nothing);
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
			if (CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Attack }))
			{
				CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Nothing);
				HeavyAttackEvent();
			}
			else
			{
				HeavyAttackEvent();
			}
		}
		else
		{
			AnimatorProvider->PlayAnimMontage(CrasherMontage);
		}
	}
	else if (bIsSaveHeavyAttack && ComboExtenderIndex > 0)
	{
		if (CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Attack }))
		{
			CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Nothing);
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

TScriptInterface<IWeaponInterface> UCombatComponent::GetCurrentWeapon()
{
	WeaponProvider = GetOwner();
	return WeaponProvider->GetWeaponEquipped();
}