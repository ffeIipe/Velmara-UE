#include "Entities/Entity.h"

#include "AbilitySystemComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NiagaraFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "Components/BufferComponent.h"
#include "Components/FieldCreationComponent.h"
#include "DamageTypes/PistolDamage.h"
#include "DamageTypes/BaseDamageType.h"
#include "DataAssets/CombatStrategyData.h"
#include "DataAssets/EntityData.h"
#include "DataAssets/InputData.h"
#include "DataAssets/MontagesData.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GAS/VelmaraAttributeSet.h"
#include "Interfaces/EntityAnimInstanceProvider.h"
#include "Items/Weapons/Sword.h"
#include "Items/Weapons/Strategies/CombatStrategy.h"
#include "Kismet/GameplayStatics.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"

AEntity::AEntity()
{
	PrimaryActorTick.bCanEverTick = false;
	
	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;

	AbilitySystemComponent = CreateDefaultSubobject<UAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AbilitySystemComponent->SetIsReplicated(false);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Minimal);

	AttributeSet = CreateDefaultSubobject<UVelmaraAttributeSet>(TEXT("AttributeSet"));
	
	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));

	AttributeComponent = CreateDefaultSubobject<UAttributeComponent>(TEXT("Attribute"));

	CharacterStateComponent = CreateDefaultSubobject<UCharacterStateComponent>(TEXT("CharacterState"));

	ExtraMovementComponent = CreateDefaultSubobject<UExtraMovementComponent>(TEXT("ExtraMovement"));

	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));

	PossessionComponent = CreateDefaultSubobject<UPossessionComponent>(TEXT("Possession"));

	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("Targeting"));

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm"));
	SpringArmComponent->SetupAttachment(GetRootComponent());

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(GetSpringArmComponent());

	FieldCreationComponent = CreateDefaultSubobject<UFieldCreationComponent>(TEXT("Field Creation"));

	BufferComponent = CreateDefaultSubobject<UBufferComponent>(TEXT("Buffer"));
	
	GetSpringArmComponent()->bUsePawnControlRotation = true;
}

UAbilitySystemComponent* AEntity::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

void AEntity::OnSaveGame_Implementation(FEntitySaveData& OutData)
{
	if (InventoryComponent)
	{
		InventoryComponent->SaveInventory();
	}

	FMemoryWriter MemWriter(OutData.ByteData);
	FObjectAndNameAsStringProxyArchive Ar(MemWriter, true);
	Ar.ArIsSaveGame = true;

	this->Serialize(Ar);

	if (InventoryComponent)
	{
		InventoryComponent->Serialize(Ar);
	}
}

void AEntity::OnLoadGame_Implementation(const FEntitySaveData& InData)
{
	FMemoryReader MemReader(InData.ByteData);
	FObjectAndNameAsStringProxyArchive Ar(MemReader, true);
	Ar.ArIsSaveGame = true;

	this->Serialize(Ar);

	if (InventoryComponent)
	{
		InventoryComponent->Serialize(Ar);
	}

	if (InventoryComponent)
	{
		InventoryComponent->LoadInventory();
	}
}

void AEntity::GetHit(const TScriptInterface<ICombatTargetInterface> DamageCauser, const FVector& ImpactPoint,
                     FDamageEvent const& DamageEvent, const float DamageReceived)
{
	if (CharacterStateComponent->IsActionEqualToAny({ ECharacterActionsStates::ECAS_Dead })) return;

	if (IsBlocking()) CombatComponent->ReceiveBlock(MontagesData->Montages.BlockMontage);
	
	LastDamageCauser = DamageCauser;

	if (const UBaseDamageType* DamageTypeCDO = Cast<UBaseDamageType>(DamageEvent.DamageTypeClass->GetDefaultObject()))
	{
		if (DamageTypeCDO->HitSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, DamageTypeCDO->HitSound, ImpactPoint);
		}

		if (DamageTypeCDO->HitFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(), DamageTypeCDO->HitFX, ImpactPoint);
		}
	}

	GetDirectionalReact(ImpactPoint);
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
	if (const TObjectPtr<UCombatStrategy>* FoundStrategy = StrategyInstances.Find(Mode))
	{
		CurrentStrategy = *FoundStrategy;
        
		if (CurrentStrategy)
		{
			CurrentStrategy->InitializeStrategy();
			CurrentStrategy->SetCurrentValues(this); 
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("AEntity::SetCombatStrategy: Strategy not found %d in mode %s"), (int32)Mode, *GetName());
	}
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

void AEntity::InitializeAttributes()
{
	if (AbilitySystemComponent && DefaultAttributeEffect)
	{
		FGameplayEffectContextHandle ContextHandle = AbilitySystemComponent->MakeEffectContext();
		ContextHandle.AddSourceObject(this);

		const FGameplayEffectSpecHandle SpecHandle = AbilitySystemComponent->MakeOutgoingSpec(DefaultAttributeEffect, 1.f, ContextHandle);

		if (SpecHandle.IsValid())
		{
			AbilitySystemComponent->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), AbilitySystemComponent);
		}
	}
}

void AEntity::BeginPlay()
{
	Super::BeginPlay();


	if (ExtraMovementComponent)
	{
		ExtraMovementComponent->OnDodgeStarted.BindUFunction(TargetingComponent, "RemoveCombatTarget");
		ExtraMovementComponent->OnDodgeSaved.AddDynamic(this, &AEntity::Input_Dodge);
	}

	if (CombatComponent)
	{
		CombatComponent->OnResetState.AddDynamic(ExtraMovementComponent, &UExtraMovementComponent::ResetDodge);
		CombatComponent->OnResetState.AddDynamic(TargetingComponent, &UTargetingComponent::RemoveCombatTarget);
		CombatComponent->OnSaveHeavyAttack.AddDynamic(this, &AEntity::Input_SecondaryAttack);
		CombatComponent->OnLightAttack.AddDynamic(TargetingComponent, &UTargetingComponent::PerformSoftLock);
		CombatComponent->OnHeavyAttack.AddDynamic(TargetingComponent, &UTargetingComponent::PerformSoftLock);
	}

	if (AttributeComponent)
	{
		AttributeComponent->OnOutOfEnergy.AddDynamic(GetPossessionComponent(), &UPossessionComponent::TryReleasePossession);
	}

	if (PossessionComponent)
	{
		PossessionComponent->OnPossessed.AddDynamic(AttributeComponent, &UAttributeComponent::StartDecreaseEnergy);
	}
	
	if (InventoryComponent)
	{
		InventoryComponent->OnWeaponChanged.AddDynamic(CombatComponent, &UCombatComponent::HandleWeaponChanged);
		InventoryComponent->OnWeaponChanged.AddDynamic(TargetingComponent, &UTargetingComponent::HandleWeaponChanged);
	}
	
	if (CombatComponent && AttributeComponent)
	{
		CombatComponent->CanPerformActionDelegate.BindUObject(AttributeComponent, &UAttributeComponent::RequiresEnergyForTag);
		CombatComponent->OnActionPerformed.AddUObject(AttributeComponent, &UAttributeComponent::ConsumeEnergyForTag);
	}

	if (TargetingComponent)
	{
		TargetingComponent->OnHardLockOn.AddDynamic(this, &AEntity::EnableControllerRotationYaw);
		TargetingComponent->OnHardLockOff.AddDynamic(this, &AEntity::DisableControllerRotationYaw);
	}
	
	if (EntityData)
	{
		for (const auto& Entry : EntityData->ModeStrategies)
		{
			ECharacterModeStates Mode = Entry.Key;

			if (TSubclassOf<UCombatStrategy> StrategyClass = Entry.Value)
			{
				UCombatStrategy* NewStrategy = NewObject<UCombatStrategy>(this, StrategyClass);
				StrategyInstances.Add(Mode, NewStrategy);
			}
		}
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
	bUseControllerRotationYaw = true;
	GetCharacterMovement()->bOrientRotationToMovement = false;
}

void AEntity::DisableControllerRotationYaw()
{
	bUseControllerRotationYaw = false;
	GetCharacterMovement()->bOrientRotationToMovement = true;
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
	GetInventoryComponent()->PerformInteract();
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