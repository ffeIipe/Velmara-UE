#include "Components/CombatComponent.h"

#include "Components/CharacterStateComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "DataAssets/EntityData.h"

#include "Interfaces/AnimatorProvider.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "SceneEvents/NewGameModeBase.h"
#include "Interfaces/CharacterStateProvider.h"
#include "Interfaces/CharacterMovementProvider.h"
#include "Interfaces/OwnerUtilsInterface.h"
#include "Interfaces/CombatTargetInterface.h"
#include "Interfaces/Weapon/WeaponProvider.h"
#include "Interfaces/CameraProvider.h"
#include "Interfaces/ControllerProvider.h"
#include "Items/Weapons/Sword.h"
#include "Player/CharacterWeaponStates.h"
#include "Subsystems/EffectsManager.h"

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = true;

	BufferAttackTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BufferAttackTimeline"));
	LaunchCharacterTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("LaunchCharacterTimeline"));
	BufferBackwardsTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BufferBackwardsTimeline"));
}

void UCombatComponent::InitializeValues(const FCombatData& CombatData)
{
	UE_LOG(LogTemp, Warning, TEXT("Combat Component values assigned by CombatData"));

	BufferAttackDistance = CombatData.BufferAttackDistance;
	BufferCurve = CombatData.BufferCurve;
}

void UCombatComponent::ResetState()
{
	if (OwnerUtils->IsFlying())
	{
		OwnerUtils->SetMovementMode(MOVE_Falling);
	}

	if (!CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Block }))
	{
		CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Nothing);
	}

	if (const TScriptInterface<IResetMelee> MeleeToReset = GetCurrentWeapon().GetObject()) MeleeToReset->ResetMelee();

	if (OnResetState.IsBound()) OnResetState.Broadcast(); //this is called by AN_ResetState during the anim event
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetComponentTickEnabled(false);
	
	OwnerUtils = GetOwner();
	ControllerProvider = GetOwner();
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

	if (LaunchUpCurve)
	{
		FOnTimelineFloat ProgressLaunchUpFunction;
		ProgressLaunchUpFunction.BindUFunction(this, FName("UpdateLaunchCharacterUp"));
		LaunchCharacterTimeline->AddInterpFloat(LaunchUpCurve, ProgressLaunchUpFunction);

	}
		
	if (BufferBackwardsCurve)
	{
		FOnTimelineFloat ProgressBufferBackwardsFunction;
		ProgressBufferBackwardsFunction.BindUFunction(this, FName("UpdateBufferBackwards"));
		BufferBackwardsTimeline->AddInterpFloat(BufferBackwardsCurve, ProgressBufferBackwardsFunction);
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

		AnimatorProvider->PlayAnimMontage(ComboStarterAttack[AttackIndex - 1]);

		bIsSaveHeavyAttack = false;
		bIsSaveLightAttack = false;

		//PerformSoftLock();
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

		/*ResetLightAttackStats();
		ResetHeavyAttackStats();*/
		//PerformSoftLock();
	}
}

void UCombatComponent::StartAttackBufferEvent()
{
	StopAttackBufferEvent();
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
	UpdateBuffer(Alpha, -BufferAttackDistance);
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

void UCombatComponent::UpdateLaunchCharacterUp(const float Alpha)
{
	const FVector TargetLocation = FMath::Lerp(CurrentLocationLaunch, CurrentLocationLaunch + (UpVectorLaunch * 300.f), Alpha);
	GetOwner()->SetActorLocation(TargetLocation, true);
}

void UCombatComponent::PerformBlock(const bool bIsTriggered, UAnimMontage* BlockMontage) const
{
	if (CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Stun, ECharacterActionsStates::ECAS_Dead })) return;

	bIsTriggered && !CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Stun, ECharacterActionsStates::ECAS_Dead })
		? Block(BlockMontage) : ReleaseBlock(BlockMontage);
}

void UCombatComponent::Block(UAnimMontage* BlockMontage) const
{
	if (CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Stun })) return;
	
	if (!CharacterStateProvider->IsWeaponStateEqualToAny({ECharacterWeaponStates::ECWS_Unequipped }))
	{
		AnimatorProvider->PlayAnimMontage(BlockMontage, 1.f, FName("BlockIdle"));
		CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Block);
	}
}

void UCombatComponent::ReceiveBlock(UAnimMontage* BlockMontage) const
{
	AnimatorProvider->StopAnimMontage(BlockMontage);
	AnimatorProvider->PlayAnimMontage(BlockMontage, 1.f, FName("BlockReact"));
	CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Stun);
	
	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "TODO: Change the stun time.");
	
	if (UEffectsManager* EffectsManager = GetWorld()->GetSubsystem<UEffectsManager>())
	{
		EffectsManager->CameraShake(ECameraShakePreset::ECSP_SwordHit, GetOwner()->GetActorLocation());
	}
}

void UCombatComponent::ReleaseBlock(UAnimMontage* BlockMontage) const
{
	AnimatorProvider->StopAnimMontage(BlockMontage);
	CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Nothing);
}

bool UCombatComponent::CanAttack() const
{
	return !CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Attack, ECharacterActionsStates::ECAS_Dodge }) &&
		!CharacterStateProvider->IsWeaponStateEqualToAny({ ECharacterWeaponStates::ECWS_Unequipped }) &&
		!CharacterStateProvider->IsModeStateEqualToAny({ECharacterModeStates::ECMS_Spectral});
}

bool UCombatComponent::PerformLaunch(const TScriptInterface<ICombatTargetInterface>& TargetToCheck, const float DistanceToCheck, UAnimMontage* LaunchMontage)
{
	if (OnLightAttack.IsBound())
	{
		OnLightAttack.Broadcast();
	}
		
	if (CheckDistance(TargetToCheck, DistanceToCheck))
	{
		AnimatorProvider->PlayAnimMontage(LaunchMontage);
		return true;
	}
	return false;
}

bool UCombatComponent::IsInAir() const
{
	return OwnerUtils->IsFalling() || OwnerUtils->IsFlying() || bIsLaunched;
}

void UCombatComponent::PerformExecute(const TScriptInterface<ICombatTargetInterface>& Target, UAnimMontage* FinisherMontage) const
{
	if (!Target) return;
	
	if (Target->CanBeFinished() && Target->IsAlive())
	{
		CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Finish);

		const FVector Start = GetOwner()->GetActorLocation();
		const FVector End = Target->GetTargetActorLocation();
		const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(Start, End);
		GetOwner()->SetActorRotation(LookAtRotation);

		AnimatorProvider->PlayAnimMontage(FinisherMontage);

		Cast<ANewGameModeBase>(UGameplayStatics::GetGameMode(GetWorld()))->SetEnemiesAIEnabled(false);
	}
}

void UCombatComponent::LaunchCharacterUp(const TScriptInterface<ICombatTargetInterface> Target)
{
	if (!Target) return;
	
	if (Target->IsLaunchable())
	{
		OwnerUtils->SetMovementMode(MOVE_Flying);
		bIsLaunched = true;

		StartLaunchingUp();
		Target->LaunchUp();

		if (UEffectsManager* EffectsManager = GetWorld()->GetSubsystem<UEffectsManager>())
		{
			EffectsManager->TimeWarp(ETimeWarpPreset::ETWP_Crasher);
		}
	}
	else if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Not launchable");
}

void UCombatComponent::StartLaunchingUp()
{
	CurrentLocationLaunch = GetOwner()->GetActorLocation();
	UpVectorLaunch = GetOwner()->GetActorUpVector();

	LaunchCharacterTimeline->PlayFromStart();
}

void UCombatComponent::PerformCrasher()
{
	//PerformSoftLock();
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

bool UCombatComponent::CheckDistance(const TScriptInterface<ICombatTargetInterface>& TargetToCheck, const float DistanceToCheck)
{
	if (!TargetToCheck) return false;
	
	const float Distance = FVector::Distance(TargetToCheck->GetTargetActorLocation(), GetOwner()->GetActorLocation());
	if (Distance < DistanceToCheck)
		return true;
	
	return false;
}

bool UCombatComponent::IsBlocking() const
{
	return CharacterStateProvider->GetCurrentCharacterState().Action == ECharacterActionsStates::ECAS_Block;
}

void UCombatComponent::SaveLightAttackEvent()
{
	if (bIsSaveLightAttack)
	{
		bIsSaveLightAttack = false;
		CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Nothing);
		
		if (OnSaveLightAttack.IsBound())
		{
			OnSaveLightAttack.Broadcast();
		}
	}
	else if (bIsSaveHeavyAttack /*TODO: get the combo index from the strategy "it used to ask if the LightAttackIndex were greater than zero"*/)
	{
		if (CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Attack }))
		{
			CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Nothing);
			PerformComboStarter(1); //TODO: combo index from strategy "LightAttackIndex"
		}
	}
}

void UCombatComponent::SaveHeavyAttackEvent()
{
	if (bIsSaveHeavyAttack)
	{
		if (!bIsLaunched)
		{
			if (CharacterStateProvider->IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Attack }))
			{
				CharacterStateProvider->SetAction(ECharacterActionsStates::ECAS_Nothing);
			}
			
			if (OnSaveHeavyAttack.IsBound())
			{
				OnSaveHeavyAttack.Broadcast();
			}
		}
		else
		{
			if (GEngine)
			{
				GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "WARNING! Crasher anim needed on save heavy attack.");
			}
			/*AnimatorProvider->PlayAnimMontage(CrasherMontage); <=== */
		}
		bIsSaveHeavyAttack = false;
	}
	/*else if (bIsSaveLightAttack && ComboExtenderIndex > 0)
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
	}*/
}

void UCombatComponent::ResetAttackSave()
{
	bIsSaveLightAttack = false;
	bIsSaveHeavyAttack = false;
}

TScriptInterface<IWeaponInterface> UCombatComponent::GetCurrentWeapon()
{
	WeaponProvider = GetOwner();
	return WeaponProvider->GetCurrentWeapon();
}