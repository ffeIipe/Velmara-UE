#include "Entities/Entity.h"

#include "NiagaraFunctionLibrary.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Components/BufferComponent.h"
#include "Components/FieldCreationComponent.h"
#include "Components/MementoComponent.h"
#include "DamageTypes/EnvironmentalDamage.h"
#include "DamageTypes/MeleeDamage.h"
#include "DamageTypes/PistolDamage.h"
#include "DataAssets/CombatStrategyDataAsset.h"
#include "DataAssets/EffectsData.h"
#include "DataAssets/EntityData.h"
#include "DataAssets/InputData.h"
#include "DataAssets/MontagesData.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interfaces/EntityAnimInstanceProvider.h"
#include "Items/Weapons/Sword.h"
#include "Items/Weapons/Strategies/CombatStrategy.h"
#include "Kismet/GameplayStatics.h"

AEntity::AEntity()
{
	PrimaryActorTick.bCanEverTick = false;
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;

	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));

	AttributeComponent = CreateDefaultSubobject<UAttributeComponent>(TEXT("Attribute"));

	CharacterStateComponent = CreateDefaultSubobject<UCharacterStateComponent>(TEXT("CharacterState"));

	ExtraMovementComponent = CreateDefaultSubobject<UExtraMovementComponent>(TEXT("ExtraMovement"));

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));

	MementoComponent = CreateDefaultSubobject<UMementoComponent>(TEXT("Memento"));

	PossessionComponent = CreateDefaultSubobject<UPossessionComponent>(TEXT("Possession"));

	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("Targeting"));

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm"));
	SpringArmComponent->SetupAttachment(GetRootComponent());

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(GetSpringArmComponent());

	FieldCreationComponent = CreateDefaultSubobject<UFieldCreationComponent>(TEXT("Field Creation"));

	BufferComponent = CreateDefaultSubobject<UBufferComponent>(TEXT("Buffer"));
	
	ExtraMovementComponent->OnDodgeStarted.BindUFunction(TargetingComponent, "RemoveCombatTarget");
	ExtraMovementComponent->OnDodgeSaved.AddDynamic(this, &AEntity::Input_Dodge);

	CombatComponent->OnResetState.AddDynamic(ExtraMovementComponent, &UExtraMovementComponent::ResetDodge);
	CombatComponent->OnResetState.AddDynamic(TargetingComponent, &UTargetingComponent::RemoveCombatTarget);
	CombatComponent->OnSaveHeavyAttack.AddDynamic(this, &AEntity::Input_SecondaryAttack);
	CombatComponent->OnLightAttack.AddDynamic(TargetingComponent, &UTargetingComponent::PerformSoftLock);
	CombatComponent->OnHeavyAttack.AddDynamic(TargetingComponent, &UTargetingComponent::PerformSoftLock);

	GetAttributeComponent()->OnOutOfEnergy.AddDynamic(GetPossessionComponent(), &UPossessionComponent::TryReleasePossession);

	GetPossessionComponent()->OnPossessed.AddDynamic(AttributeComponent, &UAttributeComponent::StartDecreaseEnergy);

	GetSpringArmComponent()->bUsePawnControlRotation = true;

	TargetingComponent->OnHardLockToggled.AddDynamic(this, &AEntity::EnableControllerRotationYaw);
}

void AEntity::GetHit(const TScriptInterface<ICombatTargetInterface> DamageCauser, const FVector& ImpactPoint,
                     FDamageEvent const& DamageEvent, const float DamageReceived)
{
	if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActionsStates::ECAS_Dead })) return;

	if (IsBlocking()) CombatComponent->ReceiveBlock(MontagesData->Montages.BlockMontage);
	
	LastDamageCauser = DamageCauser;

	const UClass* DamageClass = DamageEvent.DamageTypeClass;

	if (!DamageClass) return;
	
	if (DamageClass->IsChildOf(UEnvironmentalDamage::StaticClass()))
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Blue, "Environmental Damage");
	}
	else if (DamageClass->IsChildOf(UMeleeDamage::StaticClass()))
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Blue, "Melee Damage");
		
		if (EffectsData->EntityEffects.ReceiveDamageSFX)
		{
			UGameplayStatics::PlaySoundAtLocation(
				GetWorld(),
				EffectsData->EntityEffects.ReceiveDamageSFX,
				GetTargetActorLocation()
			);
		}

		if (EffectsData->EntityEffects.HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(
				this,
				EffectsData->EntityEffects.HitSound,
				ImpactPoint
			);
		}
	
		if (EffectsData->EntityEffects.ReceiveDamageFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				EffectsData->EntityEffects.ReceiveDamageFX,
				ImpactPoint
			);
		}
		
		CombatComponent->StartBufferBackwards();
		GetDirectionalReact(ImpactPoint);
	}
	else if (DamageClass->IsChildOf(UPistolDamage::StaticClass()))
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Blue, "Pistol Damage");
		
		if (EffectsData->EntityEffects.ShieldImpactSFX)
		{
			UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			EffectsData->EntityEffects.ShieldImpactSFX,
			GetAttributeComponent()->GetShieldMeshComponent()->GetComponentLocation()
			);	
		}
	}
}

void AEntity::GetFinished()
{
	
}

bool AEntity::IsHittable()
{
	return !GetAttributeComponent()->IsShielded() && IsAlive() && !IsBlocking();
}

void AEntity::AddStunBehavior()
{
	GetMesh()->GlobalAnimRateScale = .5f;
	GetCharacterMovement()->MaxWalkSpeed = EntityData->MovementData.StunMaxWalkSpeed;
	CharacterStateComponent->SetAction(ECharacterActionsStates::ECAS_Stun);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Blue, "Stun Behavior added!");
	}
}

void AEntity::RemoveStunBehavior()
{
	GetMesh()->GlobalAnimRateScale = 1.f;
	GetCharacterMovement()->MaxWalkSpeed = CurrentStrategy->CombatStrategyData->StrategyProperties.MaxWalkSpeed;
	CharacterStateComponent->SetAction(ECharacterActionsStates::ECAS_Nothing);

	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, "Stun Behavior removed!");
	}
}

void AEntity::GetDirectionalReact(const FVector& ImpactPoint)
{
	const FVector Forward = GetActorForwardVector();
	const FVector ToHit = (ImpactPoint - GetActorLocation()).GetSafeNormal();

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

	Execute_PlayAnimMontage(this, MontagesData->Montages.HitReactMontage, 1.f, Section);
}

void AEntity::ChangeWeaponAnimationState()
{
	if (const TScriptInterface<IEntityAnimInstanceProvider> AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->SetAnimationState(Execute_GetCurrentWeapon(this));
	}
}

void AEntity::SetCombatStrategy(const ECharacterModeStates Mode)
{
	switch (Mode)
	{
	case ECharacterModeStates::ECMS_Human:
		if (HumanStrategyInstance)
			CurrentStrategy = HumanStrategyInstance;
		
		break;
		
	case ECharacterModeStates::ECMS_Spectral:
		if (SpectralStrategyInstance)
			CurrentStrategy = SpectralStrategyInstance;
		
		break;
		
	default:
		if (HumanStrategyInstance)
			CurrentStrategy = HumanStrategyInstance;
		
		break;
	}
	
	if (CurrentStrategy)
	{
		CurrentStrategy->InitializeStrategy();
		//CurrentStrategy->SetCurrentValues(this);
	}
	else if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, "MISSING! Strategy is not valid! Stop the game or it will crash.");
}

void AEntity::HitReactJumpToSection(const FName Section)
{
	if (MontagesData->Montages.HitReactMontage)
	{
		Execute_PlayAnimMontage(this, MontagesData->Montages.HitReactMontage, 1.f, Section);
	}
}

bool AEntity::IsEquipped()
{
	return CharacterStateComponent->IsWeaponStateEqualToAny({ECharacterWeaponStates::ECWS_EquippedWeapon});
}

bool AEntity::CanBeFinished()
{
	if (GetAttributeComponent()->GetHealthPercent() <= .4f)
	{
		if (OnCanBeFinished.IsBound())
		{
			if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red,
				"Can be Finished" + FString::SanitizeFloat(GetAttributeComponent()->GetHealth()));
			OnCanBeFinished.Broadcast();
		}
		return true;
	}
	return false;
}

bool AEntity::IsLaunchable()
{
	return false;
}

void AEntity::SetWeaponCollisionEnabled(const ECollisionEnabled::Type CollisionEnabled)
{
	if (Execute_GetCurrentWeapon(this))
	{
		Execute_GetCurrentWeapon(this)->SetWeaponCollisionEnabled(CollisionEnabled);
			
		if (CollisionEnabled != ECollisionEnabled::NoCollision)
		{
			Execute_GetCurrentWeapon(this)->ClearIgnoreActors();
		}
	}
}

void AEntity::BeginPlay()
{
	Super::BeginPlay();
	
	if (!HumanStrategyInstance)
	{
		if (EntityData->FirstModeStrategyClass) HumanStrategyInstance = NewObject<UCombatStrategy>(this, EntityData->FirstModeStrategyClass);
		else if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "MISSING! Human class from: " + GetName());
	}

	if (!SpectralStrategyInstance)
	{
		if (EntityData->SecondModeStrategyClass) SpectralStrategyInstance = NewObject<UCombatStrategy>(this, EntityData->SecondModeStrategyClass);
		else if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "MISSING! Spectral class from: " + GetName());
	}

	SetCombatStrategy(ECharacterModeStates::ECMS_Human);
}

void AEntity::InitializeComponentsData() const
{
	if (EntityData)
	{
		CombatComponent->InitializeValues(EntityData->CombatData);
		AttributeComponent->InitializeValues(EntityData->AttributeData);
		PossessionComponent->InitializeValues(EntityData->PossessionData);
		InventoryComponent->InitializeValues(EntityData->InventoryData);
		TargetingComponent->InitializeValues(EntityData->TargetingData);
		
		GetSpringArmComponent()->TargetArmLength = EntityData->SpringArmData.SpringArmLength;
		GetSpringArmComponent()->SocketOffset = EntityData->SpringArmData.SocketOffset;
		GetSpringArmComponent()->bEnableCameraLag = EntityData->SpringArmData.CameraLag;
		GetSpringArmComponent()->CameraLagSpeed = EntityData->SpringArmData.CameraLagSpeed;
		GetSpringArmComponent()->CameraRotationLagSpeed = EntityData->SpringArmData.CameraRotationLagSpeed;

		GetCharacterMovement()->GravityScale = EntityData->CharMoveData.GravityScale;
		GetCharacterMovement()->MaxAcceleration = EntityData->CharMoveData.MaxAcceleration;
		GetCharacterMovement()->MaxWalkSpeed = EntityData->CharMoveData.MaxWalkSpeed;
		GetCharacterMovement()->BrakingDecelerationWalking = EntityData->CharMoveData.BrakingDecelerationWalking;
		GetCharacterMovement()->JumpZVelocity = EntityData->CharMoveData.JumpZVelocity;
		GetCharacterMovement()->AirControl = EntityData->CharMoveData.AirControl;
		GetCharacterMovement()->AirControlBoostMultiplier = EntityData->CharMoveData.AirControlBoostMultiplier;
		GetCharacterMovement()->RotationRate = EntityData->CharMoveData.RotationRate;
		GetCharacterMovement()->MaxJumpApexAttemptsPerSimulation = EntityData->CharMoveData.MaxJumpApexAttemptsPerSimulation;
	}
}

void AEntity::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);
	
	InitializeComponentsData();
}

void AEntity::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent); EnhancedInputComponent && InputsData)
	{
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_Jump, ETriggerEvent::Started, this, &AEntity::Jump);
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_Move, ETriggerEvent::Triggered, this, &AEntity::Input_Move);
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_Move, ETriggerEvent::Completed, this, &AEntity::Input_Move);
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_Dodge, ETriggerEvent::Started, this, &AEntity::Input_Dodge);
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_Look, ETriggerEvent::Triggered, this, &AEntity::Input_Look);

		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_Attack, ETriggerEvent::Triggered, this, &AEntity::Input_PrimaryAttack);
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_HeavyAttack, ETriggerEvent::Started, this, &AEntity::Input_SecondaryAttack);

		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_Inventory, ETriggerEvent::Started, this, &AEntity::Input_ToggleHardLock);
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_ChangeHardLockTarget, ETriggerEvent::Started, this, &AEntity::Input_ChangeHardLockTarget);
		
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_Block, ETriggerEvent::Triggered, this, &AEntity::Input_Block);
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_Block, ETriggerEvent::Completed, this, &AEntity::Input_Block);

		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_Interact, ETriggerEvent::Started, this, &AEntity::Input_Interact);
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_ToggleWeapon, ETriggerEvent::Started, this, &AEntity::Input_ToggleWeapon);
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "MISSING! Input Data from: " + GetName());
	}
}

void AEntity::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	
	CombatComponent->bIsLaunched = false;
	ExtraMovementComponent->CanDoubleJump = true;

	if (!GetAttributeComponent()->IsAlive())
	{
		GetCharacterMovement()->DisableMovement();
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->SetMovementMode(MOVE_None);

		Die(MontagesData->Montages.DeathMontage, FName("DeathFromHeight"));
	}
}

bool AEntity::IsEquipping() const
{
	return CharacterStateComponent->CurrentStates.WeaponState == ECharacterWeaponStates::ECWS_EquippingWeapon;
}

float AEntity::TakeDamage(const float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	LastDamageCauser = Cast<AEntity>(DamageCauser);

	if (GetAttributeComponent()->IsShielded() && DamageEvent.DamageTypeClass == UPistolDamage::StaticClass())
	{
		GetAttributeComponent()->ReceiveShieldDamage(DamageAmount);

		if (OnShieldTakeDamage.IsBound()) OnShieldTakeDamage.Broadcast();
	}
	else
	{
		GetAttributeComponent()->ReceiveDamage(DamageAmount);
		CanBeFinished();
	}
	return DamageAmount;
}

void AEntity::Input_Move(const FInputActionValue& Value)
{
	if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActionsStates::ECAS_Block, ECharacterActionsStates::ECAS_Finish, ECharacterActionsStates::ECAS_Dead })) return;
	
	ExtraMovementComponent->PerformMove(Value.Get<FVector2D>(), Value.Get<bool>());
}

void AEntity::Input_Look(const FInputActionValue& Value)
{
	if (!IsAlive() && TargetingComponent->IsLocking()) return;
	
	ExtraMovementComponent->PerformLook(Value.Get<FVector2D>());	
}

void AEntity::Jump()
{
	if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActionsStates::ECAS_Nothing, ECharacterActionsStates::ECAS_Attack }) && ExtraMovementComponent->CanDoubleJump && !IsEquipping())
	{
		Execute_PlayAnimMontage(this, MontagesData->Montages.JumpMontage, 1.f, "Default");

		Super::Jump();

		if (GetCharacterMovement()->IsFalling() && ExtraMovementComponent->CanDoubleJump)
		{
			if (!CharacterStateComponent->IsActionEqualToAny({ ECharacterActionsStates::ECAS_Block, ECharacterActionsStates::ECAS_Finish, ECharacterActionsStates::ECAS_Dead }))
			{
				ExtraMovementComponent->PerformDoubleJump(MontagesData->Montages.DoubleJumpMontage);
			}
		}
	}
}	

void AEntity::Input_Dodge()
{
	if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActionsStates::ECAS_Stun, ECharacterActionsStates::ECAS_Dead })) return;
	
	GetCurrentStrategy()->Strategy_UseCommand(this, ECT_Dodge);
}

void AEntity::Input_PrimaryAttack(const FInputActionValue& Value)
{
	if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActionsStates::ECAS_Stun, ECharacterActionsStates::ECAS_Dead })) return;
	const bool bIsMovingBackwards = GetTargetingComponent()->IsLocking() && IsMovingBackwards();

	const bool bIsInAir = CombatComponent->IsInAir();
	
	bIsMovingBackwards && !bIsInAir ?
	GetCurrentStrategy()->Strategy_UseCommand(this, ECT_LaunchAttack) :
	GetCurrentStrategy()->Strategy_UseCommand(this, ECT_PrimaryAttack);
}

void AEntity::EnableControllerRotationYaw()
{
	bUseControllerRotationYaw = !bUseControllerRotationYaw;
	GetCharacterMovement()->bOrientRotationToMovement = !GetCharacterMovement()->bOrientRotationToMovement;
}

void AEntity::Input_SecondaryAttack()
{
	if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActionsStates::ECAS_Stun, ECharacterActionsStates::ECAS_Dead })) return;
	
	GetCurrentStrategy()->Strategy_UseCommand(this, ECT_SecondaryAttack);
}

void AEntity::Input_Ability() //'F' input
{
	GetCurrentStrategy()->Strategy_UseCommand(this, ECT_Ability);
}

void AEntity::Input_Block(const FInputActionValue& Value)
{
	if (!CharacterStateComponent->IsActionEqualToAny({ ECharacterActionsStates::ECAS_Stun, ECharacterActionsStates::ECAS_Dead })
		&& IsEquipped())
	{
		CombatComponent->PerformBlock(Value.Get<bool>(), MontagesData->Montages.BlockMontage);
	}
}

void AEntity::Input_ChangeHardLockTarget()
{
	if (CharacterStateComponent->IsWeaponStateEqualToAny({ECharacterWeaponStates::ECWS_EquippedWeapon}))
	{
		GetTargetingComponent()->ChangeHardLockTarget();
	}
}

void AEntity::Input_ToggleHardLock()
{
	if (CharacterStateComponent->IsWeaponStateEqualToAny({ECharacterWeaponStates::ECWS_EquippedWeapon}))
	{
		GetTargetingComponent()->ToggleHardLock();
	}
}

void AEntity::Input_Interact(const FInputActionValue& InputActionValue)
{
	if (const TScriptInterface<IWeaponInterface> WeaponReached = GetInventoryComponent()->PerformInteract())
	{
		WeaponReached->OnWeaponUsed.AddDynamic(CombatComponent, &UCombatComponent::StartAttackBufferEvent);
		WeaponReached->OnWeaponUsed.AddDynamic(GetTargetingComponent(), &UTargetingComponent::PerformSoftLock);
	}
}

void AEntity::Input_ToggleWeapon()
{
	if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActionsStates::ECAS_Nothing, ECharacterActionsStates::ECAS_Stun }) &&
		CharacterStateComponent->IsModeEqualToAny({ ECharacterModeStates::ECMS_Human }) &&
		IsEquipped())
	{
		InventoryComponent->ToggleInventorySlot();
	}
}

void AEntity::Die(UAnimMontage* DeathAnim, const FName Section)
{
	if (CharacterStateComponent->CurrentStates.Action == ECharacterActionsStates::ECAS_Dead) return;

	Execute_StopAnimMontage(this, GetCurrentMontage());
	CharacterStateComponent->SetAction(ECharacterActionsStates::ECAS_Dead);

	if (CombatComponent->IsInAir())
	{
		ACharacter::LaunchCharacter(FVector(0.f, 0.f, -300.f), true, true);
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	if (DeathAnim)
	{
		Execute_StopAnimMontage(this, GetCurrentMontage());
		Execute_PlayAnimMontage(this, DeathAnim, 1.f, Section);
	}
}

void AEntity::OnWallCollision()
{
	Execute_StopAnimMontage(this, GetCurrentMontage()) ;
	HitReactJumpToSection(FName("ReactToShield"));
}