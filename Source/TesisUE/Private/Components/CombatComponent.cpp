#include "Components/CombatComponent.h"

#include "GameplayTagAssetInterface.h"
#include "Components/AttributeComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "DataAssets/EntityData.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "SceneEvents/VelmaraGameMode.h"
#include "Interfaces/CombatTargetInterface.h"
#include "Interfaces/Weapon/WeaponInterface.h"
#include "Player/CharacterWeaponStates.h"
#include "Subsystems/EffectsManager.h"

void UCombatComponent::HandleWeaponChanged(TScriptInterface<IWeaponInterface> NewWeapon)
{
	if (NewWeapon)
	{
		NewWeapon->OnWeaponUsed.AddUniqueDynamic(this, &UCombatComponent::StartAttackBufferEvent);
	}
}

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
	LaunchCharacterUpCurve = CombatData.LaunchUpCurve;
	BufferBackwardsCurve = CombatData.BufferBackwardsCurve;
	BufferCurve = CombatData.BufferCurve;
}

void UCombatComponent::ResetState()
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Purple, "RESET STATE!");
	
	if (OwnerCharacter->GetCharacterMovement()->IsFlying())
	{
		OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	}
	
	StateComponent->SetAction(ECharacterActionsStates::ECAS_Nothing);

	if (OnResetState.IsBound()) OnResetState.Broadcast(); //this is called by AN_ResetState during the anim event
}

void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();
	
	PrimaryComponentTick.bStartWithTickEnabled = false;
	SetComponentTickEnabled(false);
	
	OwnerCharacter = Cast<ACharacter>(GetOwner());
	
	if (GetOwner())
	{
		StateComponent = GetOwner()->FindComponentByClass<UCharacterStateComponent>();
		AttributeComponent = GetOwner()->FindComponentByClass<UAttributeComponent>();
	}
	
	if (BufferCurve)
	{
		FOnTimelineFloat ProgressAttackFunction;
		ProgressAttackFunction.BindUFunction(this, FName("UpdateAttackBuffer"));
		BufferAttackTimeline->AddInterpFloat(BufferCurve, ProgressAttackFunction);
	}

	if (LaunchCharacterUpCurve)
	{
		FOnTimelineFloat ProgressLaunchUpFunction;
		ProgressLaunchUpFunction.BindUFunction(this, FName("UpdateLaunchCharacterUp"));
		LaunchCharacterTimeline->AddInterpFloat(LaunchCharacterUpCurve, ProgressLaunchUpFunction);

		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Magenta, GetOwner()->GetName() + " using curve: " + LaunchCharacterUpCurve->GetName());
	}
		
	if (BufferBackwardsCurve)
	{
		FOnTimelineFloat ProgressBufferBackwardsFunction;
		ProgressBufferBackwardsFunction.BindUFunction(this, FName("UpdateBufferBackwards"));
		BufferBackwardsTimeline->AddInterpFloat(BufferBackwardsCurve, ProgressBufferBackwardsFunction);
	}
}

bool UCombatComponent::CanAttack(const FGameplayTag ActionTag) const
{
	if (!OwnerCharacter || !StateComponent) return false;

	FGameplayTagContainer OwnerTags;
	if (const IGameplayTagAssetInterface* TagInterface = Cast<IGameplayTagAssetInterface>(GetOwner()))
	{
		TagInterface->GetOwnedGameplayTags(OwnerTags);
	}
	if (OwnerTags.HasAny(BlockAttackTags)) return false;

	if (CanPerformActionDelegate.IsBound())
	{
		if (!CanPerformActionDelegate.Execute(ActionTag)) 
		{
			return false; 
		}
	}

	return !StateComponent->IsWeaponStateEqualToAny({ ECharacterWeaponStates::ECWS_Unequipped });
}

//void UCombatComponent::PerformComboStarter(const int AttackIndex)
//{
//	if (CanAttack())
//	{
//		ComboExtenderIndex = AttackIndex;
//		StopAttackBufferEvent();
//		StartAttackBufferEvent();
//		StateComponent->SetAction(ECharacterActionsStates::ECAS_Attack);
//
//		OwnerCharacter->Execute_PlayAnimMontage(GetOwner(), ComboStarterAttack[AttackIndex - 1], 1.f, "Default");
//
//		bIsSaveHeavyAttack = false;
//		bIsSaveLightAttack = false;
//
//		//PerformSoftLock();
//	}
//}

//void UCombatComponent::PerformComboExtender(const int AttackIndex)
//{
//	if (CanAttack())
//	{
//		StopAttackBufferEvent();
//		StartAttackBufferEvent();
//		StateComponent->SetAction(ECharacterActionsStates::ECAS_Attack);
//
//		//CombatComponentOwner->Execute_PlayAnimMontage(GetOwner(),  ComboExtenderAttack[AttackIndex - 1]);
//
//		/*ResetLightAttackStats();
//		ResetHeavyAttackStats();*/
//		//PerformSoftLock();
//	}
//}

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

//void UCombatComponent::ValidateWall()
//{
//	if (!CanAttack()) return;
//	
//	TArray<AActor*> ObjectsToIgnore;
//	ObjectsToIgnore.Add(GetOwner());
//
//	FHitResult Hit;
//
//	FCollisionQueryParams QueryParams;
//	QueryParams.AddIgnoredActor(GetOwner());
//	QueryParams.bTraceComplex = true;
//	QueryParams.bReturnPhysicalMaterial = false;
//
//	const FVector Start = GetOwner()->GetActorLocation();
//	const FVector End = Start + (GetOwner()->GetActorForwardVector() * 100.f);
//
//	const bool bHit = GetWorld()->LineTraceSingleByChannel(
//		Hit,
//		Start,
//		End,
//		ECC_GameTraceChannel4,
//		QueryParams
//	);
//
//	if (bHit && OnWallHit.IsBound())
//	{
//		OnWallHit.Broadcast(Hit);
//	}
//
//	DrawDebugLine(
//		GetWorld(),
//		Start,
//		bHit ? Hit.ImpactPoint : End,
//		FColor::Red,
//		false,
//		2.0f,
//		0,
//		1.0f
//	);
//}

void UCombatComponent::UpdateLaunchCharacterUp(const float Alpha)
{
	const FVector TargetLocation = FMath::Lerp(CurrentLocationLaunch, CurrentLocationLaunch + (UpVectorLaunch * 300.f), Alpha);
	GetOwner()->SetActorLocation(TargetLocation, true);
}

void UCombatComponent::PerformBlock(const bool bIsTriggered, UAnimMontage* BlockMontage) const
{
	bIsTriggered ? Block(BlockMontage) : ReleaseBlock(BlockMontage);
}

void UCombatComponent::Block(UAnimMontage* BlockMontage) const
{
	OwnerCharacter->PlayAnimMontage(BlockMontage, 1.f, FName("BlockIdle"));
	StateComponent->SetAction(ECharacterActionsStates::ECAS_Block);
}

void UCombatComponent::ReceiveBlock(UAnimMontage* BlockMontage) const
{
	OwnerCharacter->StopAnimMontage(BlockMontage);
	OwnerCharacter->PlayAnimMontage(BlockMontage, 1.f, FName("BlockReact"));
	StateComponent->SetAction(ECharacterActionsStates::ECAS_Stun);
	
	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "TODO: Change the stun time.");
	
	if (UEffectsManager* EffectsManager = GetWorld()->GetSubsystem<UEffectsManager>())
	{
		EffectsManager->CameraShake(ECameraShakePreset::ECSP_SwordHit, GetOwner()->GetActorLocation());
	}
}

void UCombatComponent::ReleaseBlock(UAnimMontage* BlockMontage) const
{
	OwnerCharacter->StopAnimMontage(BlockMontage);
	StateComponent->SetAction(ECharacterActionsStates::ECAS_Nothing);
}

bool UCombatComponent::PerformLaunch(const TScriptInterface<ICombatTargetInterface>& TargetToCheck, const float DistanceToCheck, UAnimMontage* LaunchMontage)
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Blue, "PerformLaunch");
	
	if (OnLightAttack.IsBound())
	{
		OnLightAttack.Broadcast();
	}
		
	if (CheckDistance(TargetToCheck, DistanceToCheck))
	{
		OwnerCharacter->PlayAnimMontage(LaunchMontage, 1.f, "Default");
		return true;
	}
	return false;
}

bool UCombatComponent::IsInAir() const
{
	return OwnerCharacter->GetCharacterMovement()->IsFalling() || OwnerCharacter->GetCharacterMovement()->IsFlying() || bIsLaunched;
}

void UCombatComponent::PerformExecute(const TScriptInterface<ICombatTargetInterface>& Target, UAnimMontage* FinisherMontage) const
{
	if (!Target) return;
	
	if (Target->CanBeFinished() && Target->IsAlive())
	{
		StateComponent->SetAction(ECharacterActionsStates::ECAS_Finish);

		const FVector Start = GetOwner()->GetActorLocation();
		const FVector End = Target->GetTargetActorLocation();
		const FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(Start, End);
		GetOwner()->SetActorRotation(LookAtRotation);

		OwnerCharacter->PlayAnimMontage(FinisherMontage, 1.f, "Default");

		Cast<AVelmaraGameMode>(UGameplayStatics::GetGameMode(GetWorld()))->SetEnemiesAIEnabled(false);
	}
}

void UCombatComponent::LaunchCharacterUp(const TScriptInterface<ICombatTargetInterface> Target)
{
	/*if (!Target) return;*/
	
	/*if (Target->IsLaunchable())*/
	{
		OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Flying);
		bIsLaunched = true;

		StartLaunchingUp();
		/*Target->LaunchUp();*/

		if (UEffectsManager* EffectsManager = GetWorld()->GetSubsystem<UEffectsManager>())
		{
			EffectsManager->TimeWarp(ETimeWarpPreset::ETWP_Crasher);
		}
	}
	/*else if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Not launchable");*/
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
	OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	StateComponent->SetAction(ECharacterActionsStates::ECAS_Attack);

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
		OwnerCharacter->GetCharacterMovement()->SetMovementMode(MOVE_Falling);

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
	return StateComponent->CurrentStates.Action == ECharacterActionsStates::ECAS_Block;
}

void UCombatComponent::SaveLightAttackEvent()
{
	if (bIsSaveLightAttack)
	{
		bIsSaveLightAttack = false;
		StateComponent->SetAction(ECharacterActionsStates::ECAS_Nothing);
		
		if (OnSaveLightAttack.IsBound())
		{
			OnSaveLightAttack.Broadcast();
		}
	}
	else if (bIsSaveHeavyAttack /*TODO: get the combo index from the strategy "it used to ask if the LightAttackIndex were greater than zero"*/)
	{
		if (StateComponent->IsActionEqualToAny({ ECharacterActionsStates::ECAS_Attack }))
		{
			StateComponent->SetAction(ECharacterActionsStates::ECAS_Nothing);
			//PerformComboStarter(1); //TODO: combo index from strategy "LightAttackIndex"
		}
	}
}

void UCombatComponent::SaveHeavyAttackEvent()
{
	if (bIsSaveHeavyAttack)
	{
		if (!bIsLaunched)
		{
			if (StateComponent->IsActionEqualToAny({ ECharacterActionsStates::ECAS_Attack }))
			{
				StateComponent->SetAction(ECharacterActionsStates::ECAS_Nothing);
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
			/*OwnerCharacter->Execute_PlayAnimMontage(GetOwner(),  CrasherMontage); <=== */
		}
		bIsSaveHeavyAttack = false;
	}
	/*else if (bIsSaveLightAttack && ComboExtenderIndex > 0)
	{
		if (StateComponent->IsActionEqualToAny({ ECharacterActionsStates::ECAS_Attack }))
		{
			StateComponent->SetAction(ECharacterActionsStates::ECAS_Nothing);
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

/*TScriptInterface<IWeaponInterface> UCombatComponent::GetCurrentWeapon()
{
	WeaponProvider = GetOwner();
	return WeaponProvider->GetCurrentWeapon();
}*/