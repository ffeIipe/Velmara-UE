#include "Components/CombatComponent.h"
#include "Components/TimelineComponent.h"
#include "Curves/CurveFloat.h"
#include "GameFramework/Character.h"
#include "GameFramework/CharacterMovementComponent.h"

#include "Kismet/KismetMathLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "SceneEvents/NewGameModeBase.h"

#include "Enemy/Paladin.h"
#include "Interfaces/LaunchableInterface.h"
#include "Interfaces/FormInterface.h"

#include "Player/PlayerMain.h"
#include "Camera/CameraActor.h"
#include <Enemy/Paladin/ShieldedPaladin.h>

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	BufferAttackTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BufferAttackTimeline"));

	SoftLockTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("SoftLockTimeline"));

	//FinisherLocation = CreateDefaultSubobject<USceneComponent>(TEXT("FinisherPosition"));
	//
	//CameraFinisherLocation = CreateDefaultSubobject<USceneComponent>(TEXT("CameraFinisherLocation"));
}

ECharacterActions UCombatComponent::SetCharacterAction(ECharacterActions NewAction)
{
	if (NewAction != CharacterAction)
	{
		CharacterAction = NewAction;
	}
	return NewAction;
}

ECharacterStates UCombatComponent::SetCharacterState(ECharacterStates NewState)
{
	if (NewState != CharacterState)
	{
		CharacterState = NewState;
	}
	return NewState;
}

bool UCombatComponent::IsActionEqualToAny(const TArray<ECharacterActions>& ActionsToCheck)
{
	return ActionsToCheck.Contains(CharacterAction);
}

bool UCombatComponent::IsStateEqualToAny(const TArray<ECharacterStates>& StatesToCheck)
{
	return StatesToCheck.Contains(CharacterState);
}

bool UCombatComponent::IsFormEqualToAny(const TArray<ECharacterForm>& FormsToCheck)
{
	return FormsToCheck.Contains(PlayerForm->Execute_GetCharacterForm(GetOwner()));
}

void UCombatComponent::ResetState()
{
	if (OwningCharacter->GetCharacterMovement()->IsFlying())
	{
		OwningCharacter->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	}

	const TArray<ECharacterActions> CharacterActionsToCheck = { ECharacterActions::ECA_Block };
	if (!IsActionEqualToAny(CharacterActionsToCheck))
	{
		SetCharacterAction(ECharacterActions::ECA_Nothing);
	}

	ResetLightAttackStats();
	ResetJumpAttackStats();
	ResetHeavyAttackStats();

	if(PlayerForm) PlayerForm->Execute_ResetSpectralAttack(GetOwner());
	//save dodge = false;
}


void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	OwningCharacter = Cast<ACharacter>(GetOwner());

	PlayerForm = Cast<IFormInterface>(GetOwner());

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
	}
}

void UCombatComponent::LightAttack(int AttackIndex)
{
	if (GetCharacterAction() != ECharacterActions::ECA_Attack && GetCharacterState() != ECharacterStates::ECS_Unequipped)
	{
		StopAttackBufferEvent();
		StartAttackBufferEvent(BufferAttackDistance);
		SetCharacterAction(ECharacterActions::ECA_Attack);
		SoftLockOn();
		OwningCharacter->PlayAnimMontage(LightAttackCombo[AttackIndex]);

		LightAttackIndex++;

		if (LightAttackIndex >= LightAttackCombo.Num())
		{
			LightAttackIndex = 0;
		}
	}
}

void UCombatComponent::JumpAttack(int AttackIndex)
{
	if (GEngine)GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Yellow, FString("Enter JumpAttack"));
	if (GetCharacterAction() != ECharacterActions::ECA_Attack && GetCharacterState() != ECharacterStates::ECS_Unequipped)
	{
		if (GEngine)GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Green, FString("Performing JumpAttack"));
		OwningCharacter->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
		SetCharacterAction(ECharacterActions::ECA_Attack);
		SoftLockOn();

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
		if (GEngine)GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("Cannot perform JumpAttack"));
	}
}

void UCombatComponent::PerformComboStarter(int AttackIndex)
{
	if (GetCharacterAction() != ECharacterActions::ECA_Attack && GetCharacterAction() != ECharacterActions::ECA_Dodge && GetCharacterState() != ECharacterStates::ECS_Unequipped)
	{
		ComboExtenderIndex = AttackIndex;
		StopAttackBufferEvent();
		StartAttackBufferEvent(BufferAttackDistance);
		SetCharacterAction(ECharacterActions::ECA_Attack);

		OwningCharacter->PlayAnimMontage(ComboStarterAttack[AttackIndex - 1]);

		bIsSaveHeavyAttack = false;
		bIsSaveLightAttack = false;

		SoftLockOn();
	}
}

void UCombatComponent::PerformComboExtender(int AttackIndex)
{
	if (GetCharacterAction() != ECharacterActions::ECA_Attack && GetCharacterAction() != ECharacterActions::ECA_Dodge && GetCharacterState() != ECharacterStates::ECS_Unequipped)
	{
		StopAttackBufferEvent();
		StartAttackBufferEvent(BufferAttackDistance);
		SetCharacterAction(ECharacterActions::ECA_Attack);

		OwningCharacter->PlayAnimMontage(ComboExtenderAttack[AttackIndex - 1]);

		ResetLightAttackStats();
		ResetHeavyAttackStats();
		SoftLockOn();
	}
}

void UCombatComponent::HeavyAttack(int AttackIndex)
{
	if (GetCharacterAction() != ECharacterActions::ECA_Attack && GetCharacterState() != ECharacterStates::ECS_Unequipped)
	{
		StopAttackBufferEvent();
		StartAttackBufferEvent(BufferAttackDistance);
		SetCharacterAction(ECharacterActions::ECA_Attack);

		OwningCharacter->PlayAnimMontage(HeavyAttackCombo[AttackIndex]);

		SoftLockOn();

		HeavyAttackIndex++;

		if (HeavyAttackIndex >= HeavyAttackCombo.Num())
		{
			HeavyAttackIndex = 0;
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

void UCombatComponent::UpdateAttackBuffer(float Alpha)
{
	UpdateBuffer(Alpha, BufferAttackDistance);
}

void UCombatComponent::UpdateBuffer(float Alpha, float BufferDistance)
{
	FVector CurrentLocation = GetOwner()->GetActorLocation();
	FVector ForwardVector = GetOwner()->GetActorForwardVector();

	FVector TargetLocation = FMath::Lerp(CurrentLocation, CurrentLocation + (ForwardVector * BufferDistance), Alpha);

	GetOwner()->SetActorLocation(TargetLocation, true);
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
	if (OwningCharacter)
	{
		FVector Start = GetOwner()->GetActorLocation();
		FVector End = (OwningCharacter->GetLastMovementInputVector() * SoftLockDistance) + GetOwner()->GetActorLocation();

		if (AActor* Enemy = Cast<APaladin>(SphereTraceForEnemies(Start, End)))
		{
			SoftLockTarget = Enemy;
			RotationToTarget();
		}
		else SoftLockTarget = nullptr;
	}
}

void UCombatComponent::RotationToTarget()
{
	if (SoftLockTarget && PlayerForm && PlayerForm->Execute_GetCharacterForm(GetOwner()) != ECharacterForm::ECF_Spectral)
	{
		SoftLockTimeline->PlayFromStart();
	}
}

void UCombatComponent::UpdateSoftLockOn(float Alpha)
{
	FVector Start = GetOwner()->GetActorLocation();

	if (!SoftLockTarget) return;
	FVector End = SoftLockTarget->GetActorLocation();

	FRotator NewRotation = FRotator(
		GetOwner()->GetActorRotation().Pitch,
		UKismetMathLibrary::FindLookAtRotation(Start, End).Yaw,
		UKismetMathLibrary::FindLookAtRotation(Start, End).Roll
	);

	NewRotation = FMath::Lerp(GetOwner()->GetActorRotation(), NewRotation, Alpha);

	GetOwner()->SetActorRotation(NewRotation);
}

void UCombatComponent::GetDirectionalReact(FName Section)
{
	if (Section != FName("KnockDown")) Section = FName("Default");

	OwningCharacter->PlayAnimMontage(HitReactMontage, 1.f, Section);
}

void UCombatComponent::Block()
{
	if (GetCharacterState() != ECharacterStates::ECS_Unequipped)
	{
		OwningCharacter->PlayAnimMontage(BlockMontage, 1.f, FName("BlockIdle"));
		SetCharacterAction(ECharacterActions::ECA_Block);
	}
}

void UCombatComponent::ReceiveBlock()
{
	OwningCharacter->PlayAnimMontage(BlockMontage, 1.f, FName("BlockReact"));
}

void UCombatComponent::ReleaseBlock()
{
	OwningCharacter->StopAnimMontage(BlockMontage);
	SetCharacterAction(ECharacterActions::ECA_Nothing);
}

void UCombatComponent::FinishEnemy()
{
	if (GetCharacterAction() == ECharacterActions::ECA_Finish) return;

	if (AActor* Enemy = SphereTraceForEnemies(OwningCharacter->GetActorLocation(), OwningCharacter->GetActorLocation() + OwningCharacter->GetActorForwardVector() * 40.f))
	{
		if (Enemy->GetClass()->ImplementsInterface(UHitInterface::StaticClass()))
		{
			if (IHitInterface::Execute_CanBeFinished(Enemy))
			{
				SetCharacterAction(ECharacterActions::ECA_Finish);

				Enemy->SetActorLocation(FVector(
					GetOwner()->GetActorLocation().X + 160.f,
					GetOwner()->GetActorLocation().Y + 20.f,
					Enemy->GetActorLocation().Z), true);

				FVector Start = OwningCharacter->GetActorLocation();
				FVector End = Enemy->GetActorLocation();
				FRotator LookAtRotation = UKismetMathLibrary::FindLookAtRotation(Start, End);
				OwningCharacter->SetActorRotation(LookAtRotation);

				OwningCharacter->PlayAnimMontage(FinisherMontage, 1.0f);
				IHitInterface::Execute_GetFinished(Enemy);

				APlayerMain* Player = Cast<APlayerMain>(GetOwner());

				//Player->FollowCamera->AttachToComponent(
				//	CameraFinisherLocation,
				//	FAttachmentTransformRules::SnapToTargetIncludingScale
				//);

				Cast<ANewGameModeBase>(UGameplayStatics::GetGameMode(GetWorld()))->SetEnemiesAIEnabled(false);
			}
			else return;
		}
	}
}

void UCombatComponent::LaunchCharacterUp()
{
	bIsLaunched = true;
	OwningCharacter->AddActorWorldOffset(FVector(0.f, 0.f, 300.f), false);

	ILaunchableInterface* Paladin = Cast<ILaunchableInterface>(SoftLockTarget);

	if (SoftLockTarget && Paladin->Execute_IsLaunchable(SoftLockTarget))
	{
		Paladin->Execute_LaunchUp(SoftLockTarget);
		OwningCharacter->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
	}
	else if (SoftLockTarget && !Paladin->Execute_IsLaunchable(SoftLockTarget))
	{
		OwningCharacter->PlayAnimMontage(HitReactMontage);
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), BlockSound, GetOwner()->GetActorLocation());
		if (GEngine)GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Orange, FString("Not LaunchUp"));
	}
}

void UCombatComponent::Crasher()
{
	SoftLockOn();
	OwningCharacter->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	CharacterAction = ECharacterActions::ECA_Attack;

	FVector Start = OwningCharacter->GetActorLocation();
	FVector End = Start + FVector(0.f, 0.f, -100000.f);

	TArray<TEnumAsByte<EObjectTypeQuery>> ObjectTypes;
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldStatic));
	ObjectTypes.Add(UEngineTypes::ConvertToObjectType(ECollisionChannel::ECC_WorldDynamic));

	TArray<AActor*> ObjectsToIgnore;
	ObjectsToIgnore.Add(GetOwner());

	FHitResult Hit;

	bool bHit = UKismetSystemLibrary::LineTraceSingleForObjects(
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
		OwningCharacter->SetActorLocation(Hit.ImpactPoint);
	}
}

AActor* UCombatComponent::SphereTraceForEnemies(FVector Start, FVector End)
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
		EDrawDebugTrace::ForDuration,
		ResultHit,
		true
	);

	return ResultHit.GetActor();
}

void UCombatComponent::Input_Attack(const FInputActionValue& Value)
{
	//save dodge = false
	bIsSaveHeavyAttack = false;

	const TArray<ECharacterActions> CharacterActionsToCheck = { ECharacterActions::ECA_Attack, ECharacterActions::ECA_Dodge};
	if (IsActionEqualToAny(CharacterActionsToCheck))
	{
		bIsSaveLightAttack = true;
	}
	else
	{
		LightAttackEvent();
	}
}

void UCombatComponent::Input_HeavyAttack(const FInputActionValue& Value)
{
	//save dodge = false;
	bIsSaveLightAttack = false;

	const TArray<ECharacterActions> CharacterActionsToCheck = { ECharacterActions::ECA_Attack, ECharacterActions::ECA_Dodge };
	if (IsActionEqualToAny(CharacterActionsToCheck))
	{
		bIsSaveHeavyAttack = true;
	}
	else
	{
		if (!bIsLaunched)
		{
			HeavyAttackEvent();
		}
		else
		{
			Crasher();
		}
	}
}

void UCombatComponent::Input_Launch(const FInputActionValue& Value)
{
	const TArray<ECharacterActions> CharacterActionsToCheck = { ECharacterActions::ECA_Attack };
	const TArray<ECharacterStates> CharacterStatesToCheck = { ECharacterStates::ECS_EquippedSword };
	const TArray<ECharacterForm> CharacterFormToCheck = { ECharacterForm::ECF_Human};

	if (!IsActionEqualToAny(CharacterActionsToCheck) && IsStateEqualToAny(CharacterStatesToCheck) && IsFormEqualToAny(CharacterFormToCheck))
	{
		SetCharacterAction(ECharacterActions::ECA_Attack);
		SoftLockOn();
		OwningCharacter->PlayAnimMontage(LaunchMontage);
	}
}

void UCombatComponent::LightAttackEvent()
{
	if (GEngine)GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Yellow, FString("LIGHT ATTACK EVENT"));
	if (PlayerForm && PlayerForm->Execute_GetCharacterForm(GetOwner()) == ECharacterForm::ECF_Spectral)
	{
		PlayerForm->Execute_PerformSpectralAttack(GetOwner());
	}
	else if (bIsLaunched)
	{
		if (GEngine)GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Green, FString("bIsLaunched = true"));
		JumpAttack(JumpAttackIndex);
	}
	else
	{
		if (GEngine)GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("bIsLaunched = false"));
		LightAttack(LightAttackIndex);
	}
}

void UCombatComponent::HeavyAttackEvent()
{
	if (PlayerForm && PlayerForm->Execute_GetCharacterForm(GetOwner()) == ECharacterForm::ECF_Spectral)
	{
		PlayerForm->Execute_PerformSpectralBarrier(GetOwner());
	}
	else
	{
		HeavyAttack(HeavyAttackIndex);
	}
}

void UCombatComponent::SaveLightAttackEvent()
{
	const TArray<ECharacterActions> CharacterActionsToCheck = { ECharacterActions::ECA_Attack };

	if (bIsSaveLightAttack)
	{
		bIsSaveLightAttack = false;
		SetCharacterAction(ECharacterActions::ECA_Nothing);
		LightAttackEvent();
	}
	else if (bIsSaveLightAttack && LightAttackIndex > 0)
	{
		if (IsActionEqualToAny(CharacterActionsToCheck))
		{
			SetCharacterAction(ECharacterActions::ECA_Nothing);
			PerformComboStarter(LightAttackIndex);
		}
	}
}

void UCombatComponent::SaveHeavyAttackEvent()
{
	const TArray<ECharacterActions> CharacterActionsToCheck = { ECharacterActions::ECA_Attack };

	if (bIsSaveHeavyAttack)
	{
		bIsSaveHeavyAttack = false;
		if (!bIsLaunched)
		{
			if (IsActionEqualToAny(CharacterActionsToCheck))
			{
				SetCharacterAction(ECharacterActions::ECA_Nothing);
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
		if (IsActionEqualToAny(CharacterActionsToCheck))
		{
			SetCharacterAction(ECharacterActions::ECA_Nothing);
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