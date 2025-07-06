#include "Components/CombatComponent.h"
#include "Components/TimelineComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/ExtraMovementComponent.h"
#include "Curves/CurveFloat.h"
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
#include "Camera/CameraActor.h"
#include "Items/Weapons/Sword.h"
#include <DamageTypes/SpectralTrapDamageType.h>

UCombatComponent::UCombatComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	BufferAttackTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("BufferAttackTimeline"));

	SoftLockTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("SoftLockTimeline"));
	
	LaunchCharacterTimeline = CreateDefaultSubobject<UTimelineComponent>(TEXT("LaunchCharacterTimeline"));
}

void UCombatComponent::ResetState()
{
	if (OwningCharacter->GetCharacterMovement()->IsFlying())
	{
		OwningCharacter->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	}

	const TArray<ECharacterActions> CharacterActionsToCheck = { ECharacterActions::ECA_Block };
	if (!CharacterStateComponent->IsActionEqualToAny(CharacterActionsToCheck))
	{
		CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Nothing);
	}

	ResetLightAttackStats();
	ResetJumpAttackStats();
	ResetHeavyAttackStats();

	if (SpectralAttacks) SpectralAttacks->Execute_ResetSpectralAttack(GetOwner());

	if (OnAttackEnd.IsBound()) OnAttackEnd.Broadcast();

	ExtraMovementComponent->bIsSaveDodge = false;
}


void UCombatComponent::BeginPlay()
{
	Super::BeginPlay();

	EntityOwner = Cast<AEntity>(GetOwner());

	OwningCharacter = Cast<ACharacter>(GetOwner());

	SpectralAttacks = Cast<IFormInterface>(GetOwner());

	CharacterStateComponent = EntityOwner->GetCharacterStateComponent();

	ExtraMovementComponent = GetOwner()->GetComponentByClass<UExtraMovementComponent>();

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

void UCombatComponent::LightAttack(int AttackIndex)
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

void UCombatComponent::JumpAttack(int AttackIndex)
{
	if (CanAttack())
	{
		StopAttackBufferEvent();
		StartAttackBufferEvent(BufferAttackDistance);
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
		else
		{
			OwningCharacter->PlayAnimMontage(JumpAttackCombo[AttackIndex]);
			JumpAttackIndex++;

			if (LightAttackIndex >= JumpAttackCombo.Num())
			{
				JumpAttackIndex = 0;
				OwningCharacter->PlayAnimMontage(CrasherMontage, 1.f);
				bIsLaunched = false;
			}
		}
	}
}

void UCombatComponent::PerformComboStarter(int AttackIndex)
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

void UCombatComponent::PerformComboExtender(int AttackIndex)
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

void UCombatComponent::HeavyAttack(int AttackIndex)
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

void UCombatComponent::UpdateAttackBuffer(float Alpha)
{
	UpdateBuffer(Alpha, BufferAttackDistance);
}

void UCombatComponent::UpdateBuffer(float Alpha, float BufferDistance)
{
	FVector CurrentLocation = GetOwner()->GetActorLocation();
	FVector ForwardVector = GetOwner()->GetActorForwardVector();

	FVector TargetLocation = FMath::Lerp(CurrentLocation, CurrentLocation + (ForwardVector * BufferDistance), Alpha);

	GetOwner()->SetActorLocation(TargetLocation, false);
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

		if (APaladin* Enemy = Cast<APaladin>(SphereTraceForEnemies(Start, End)))
		{
			if (Enemy->GetEnemyState() != EEnemyState::EES_Died)
			{
				SoftLockTarget = Enemy;
				RotationToTarget();
			}
			else SoftLockTarget = nullptr;
		}
		else SoftLockTarget = nullptr;
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

	FVector Start = GetOwner()->GetActorLocation();
	FVector End = Start + (GetOwner()->GetActorForwardVector() * 100.f);

	bool bHit = GetWorld()->LineTraceSingleByChannel(
		Hit,
		Start,
		End,
		ECollisionChannel::ECC_GameTraceChannel4,
		QueryParams
	);

	if (bHit)
	{
		AActor* HitActor = Hit.GetActor();
		if (HitActor)
		{
			OnWallHit.Broadcast(Hit);
		}
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

void UCombatComponent::UpdateLaunchCharacterUp(float Alpha)
{
	FVector TargetLocation = FMath::Lerp(CurrentLocationLaunch, CurrentLocationLaunch + (UpVectorLaunch * 300.f), Alpha);
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
	if (CharacterStateComponent->IsStateEqualToAny({ ECharacterStates::ECS_Unequipped })) return;

	if (CharacterStateComponent->IsActionEqualToAny({ 
		ECharacterActions::ECA_Dead,
		ECharacterActions::ECA_Dodge,
		ECharacterActions::ECA_Finish,
		ECharacterActions::ECA_Stun
		})) return;

	if (AActor* Enemy = SphereTraceForEnemies(OwningCharacter->GetActorLocation(), OwningCharacter->GetActorLocation() + OwningCharacter->GetActorForwardVector() * 40.f))
	{
		if (Enemy->GetClass()->ImplementsInterface(UHitInterface::StaticClass()))
		{
			if (IHitInterface::Execute_CanBeFinished(Enemy))
			{
				CharacterStateComponent->SetCharacterAction(ECharacterActions::ECA_Finish);

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

				Cast<ANewGameModeBase>(UGameplayStatics::GetGameMode(GetWorld()))->SetEnemiesAIEnabled(false);
			}
			else return;
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

	const FVector CrossProduct = FVector::CrossProduct(Forward, ToHit);
	if (CrossProduct.Z < 0)
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
	IHitInterface* Paladin = Cast<IHitInterface>(SoftLockTarget);

	if (SoftLockTarget && Paladin->Execute_IsLaunchable(SoftLockTarget, OwningCharacter))
	{
		OwningCharacter->GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Flying);
		bIsLaunched = true;
		/*GetOwner()->AddActorLocalOffset(FVector(0.f, 0.f, 300.f));
		GetOwner()->SetActorLocation()*/

		StartLaunchingUp();

		Paladin->Execute_LaunchUp(SoftLockTarget, FVector(GetOwner()->GetActorLocation())); //llamar a su componente de combate y activar su timeline de 
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

	FVector Start = GetOwner()->GetActorLocation();
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

		FVector NewLocation = FVector(Hit.ImpactPoint.X, Hit.ImpactPoint.Y, Hit.ImpactPoint.Z + 50.f);
		GetOwner()->SetActorLocation(NewLocation);
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
		EDrawDebugTrace::None,
		ResultHit,
		true
	);

	return ResultHit.GetActor();
}

bool UCombatComponent::CanAttack()
{
	return (!CharacterStateComponent->IsActionEqualToAny({ ECharacterActions::ECA_Attack, ECharacterActions::ECA_Dodge }) &&
			!CharacterStateComponent->IsStateEqualToAny({ ECharacterStates::ECS_Unequipped }) &&
			!CharacterStateComponent->IsFormEqualToAny({ECharacterForm::ECF_Spectral}));
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

	if (CanAttack())
	{
		SoftLockOn();
		if (SoftLockTarget)
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
	if (APlayerMain* PlayerRef = Cast<APlayerMain>(GetOwner()))
	{
		if (UInventoryComponent* InventoryComp = PlayerRef->GetComponentByClass<UInventoryComponent>())
		{
			return Cast<ASword>(InventoryComp->EquippedItem);
		}
		else return nullptr;
	}
	else return nullptr;
}