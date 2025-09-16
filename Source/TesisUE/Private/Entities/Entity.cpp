#include "Entities/Entity.h"

#include <NiagaraFunctionLibrary.h>

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraComponent.h"
#include "Components/MementoComponent.h"
#include "DamageTypes/SpectralTrapDamageType.h"
#include "DataAssets/EffectsData.h"
#include "DataAssets/EntityData.h"
#include "DataAssets/InputData.h"
#include "DataAssets/MontagesData.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interfaces/EntityAnimInstanceProvider.h"
#include "Items/Weapons/Sword.h"
#include "Items/Weapons/Strategies/HumanStrategy.h"
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

	GetExtraMovementComponent()->OnDodgeStarted.
	                             AddDynamic(TargetingComponent, &UTargetingComponent::RemoveCombatTarget);

	GetCombatComponent()->OnResetState.AddDynamic(ExtraMovementComponent, &UExtraMovementComponent::ResetDodge);
	GetCombatComponent()->OnResetState.AddDynamic(TargetingComponent, &UTargetingComponent::RemoveCombatTarget);
	GetCombatComponent()->OnSaveLightAttack.AddDynamic(this, &AEntity::PerformPrimaryAttack);
	GetCombatComponent()->OnSaveHeavyAttack.AddDynamic(this, &AEntity::Input_SecondaryAttack);
	GetCombatComponent()->OnLightAttack.AddDynamic(TargetingComponent, &UTargetingComponent::PerformSoftLock);
	GetCombatComponent()->OnHeavyAttack.AddDynamic(TargetingComponent, &UTargetingComponent::PerformSoftLock);

	GetAttributeComponent()->OnOutOfEnergy.AddDynamic(GetPossessionComponent(),
	                                                  &UPossessionComponent::TryReleasePossession);

	GetPossessionComponent()->OnPossessed.AddDynamic(AttributeComponent, &UAttributeComponent::StartDecreaseEnergy);

	GetSpringArmComponent()->bUsePawnControlRotation = true;
	//I think that I could solve the problem w/ the strange rotation of the hard lock system
}

void AEntity::GetHit(const TScriptInterface<ICombatTargetInterface> DamageCauser, const FVector& ImpactPoint,
                     FDamageEvent const& DamageEvent, const float DamageReceived)
{
	if (GetCharacterStateComponent()->IsActionEqualToAny({ ECharacterActionsStates::ECAS_Dead })) return;

	if (IsBlocking()) GetCombatComponent()->ReceiveBlock(MontagesData->Montages.BlockMontage);
	
	LastDamageCauser = DamageCauser;
	
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
	GetCombatComponent()->StartBufferBackwards();
	GetDirectionalReact(ImpactPoint);
}

void AEntity::GetFinished()
{
	
}

bool AEntity::IsHittable()
{
	return !GetAttributeComponent()->IsShielded() && IsAlive() && !IsBlocking();
}

ECharacterWeaponStates AEntity::SetWeaponState(const ECharacterWeaponStates NewState)
{
	return  GetCharacterStateComponent()->SetHumanState(NewState);
}

ECharacterActionsStates AEntity::SetAction(const ECharacterActionsStates NewAction)
{
	return GetCharacterStateComponent()->SetAction(NewAction);
}

ECharacterModeStates AEntity::SetMode(const ECharacterModeStates NewForm)
{
	return GetCharacterStateComponent()->SetMode(NewForm);
}

bool AEntity::IsWeaponStateEqualToAny(const TArray<ECharacterWeaponStates>& StatesToCheck)
{
	return GetCharacterStateComponent()->IsWeaponStateEqualToAny(StatesToCheck);
}

bool AEntity::IsActionStateEqualToAny(const TArray<ECharacterActionsStates>& ActionsToCheck)
{
	return GetCharacterStateComponent()->IsActionEqualToAny(ActionsToCheck);
}

bool AEntity::IsModeStateEqualToAny(const TArray<ECharacterModeStates>& FormsToCheck)
{
	return GetCharacterStateComponent()->IsModeEqualToAny(FormsToCheck);
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

	PlayAnimMontage(MontagesData->Montages.HitReactMontage, 1.f, Section);
}

void AEntity::ChangeWeaponAnimationState()
{
	if (const TScriptInterface<IEntityAnimInstanceProvider> AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->SetAnimationState(GetCurrentWeapon());
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
		CurrentStrategy->SetCurrentValues(GetExtraMovementComponent(), GetCharacterMovement(), GetTargetingComponent());
	}
	else if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5.f, FColor::Red, "MISSING! Strategy is not valid! Stop the game or it will crash.");
}

void AEntity::HitReactJumpToSection(const FName Section)
{
	if (MontagesData->Montages.HitReactMontage)
	{
		PlayAnimMontage(MontagesData->Montages.HitReactMontage, 1.f, Section);
	}
}

bool AEntity::IsEquipped()
{
	return GetCharacterStateComponent()->IsWeaponStateEqualToAny({ECharacterWeaponStates::ECWS_EquippedWeapon});
}

bool AEntity::CanBeFinished()
{
	if (GetAttributeComponent()->GetHealthPercent() <= .2f)
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
	if (GetCurrentWeapon())
	{
		GetCurrentWeapon()->SetWeaponCollisionEnabled(CollisionEnabled);
			
		if (CollisionEnabled != ECollisionEnabled::NoCollision)
		{
			GetCurrentWeapon()->ClearIgnoreActors();
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

	GetMementoComponent()->SaveState();
}

void AEntity::InitializeComponentsData() const
{
	if (EntityData)
	{
		CombatComponent->InitializeValues(EntityData->CombatData);
		ExtraMovementComponent->InitializeValues(EntityData->MovementData);
		AttributeComponent->InitializeValues(EntityData->AttributeData);
		PossessionComponent->InitializeValues(EntityData->PossessionData);
		InventoryComponent->InitializeValues(EntityData->InventoryData);
		TargetingComponent->InitializeValues(EntityData->TargetingData);
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

		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_Attack, ETriggerEvent::Started, this, &AEntity::OnPrimaryAttackStarted);
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_Attack, ETriggerEvent::Triggered, this, &AEntity::Input_PrimaryAttack);
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_Attack, ETriggerEvent::Completed, this, &AEntity::OnPrimaryAttackCompleted);
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_HeavyAttack, ETriggerEvent::Started, this, &AEntity::Input_SecondaryAttack);
		/*EnhancedInputComponent->BindAction(InputAction_Launch, ETriggerEvent::Started, this, &AEntity::Input_Launch);*/
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_Inventory, ETriggerEvent::Started, this, &AEntity::Input_ToggleHardLock);
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_ChangeHardLockTarget, ETriggerEvent::Started, this, &AEntity::Input_ChangeHardLockTarget);
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_Block, ETriggerEvent::Triggered, this, &AEntity::Input_Block);
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_Block, ETriggerEvent::Completed, this, &AEntity::Input_Block);

		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_Interact, ETriggerEvent::Started, this, &AEntity::Input_Interact);
		EnhancedInputComponent->BindAction(InputsData->Inputs.InputAction_ToggleWeapon, ETriggerEvent::Started, GetInventoryComponent(), &UInventoryComponent::ToggleInventorySlot);
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "MISSING! Input Data from: " + GetName());
	}
}

void AEntity::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	
	GetCombatComponent()->bIsLaunched = false;
	GetExtraMovementComponent()->CanDoubleJump = true;

	if (!GetAttributeComponent()->IsAlive())
	{
		GetCharacterMovement()->DisableMovement();
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->SetMovementMode(MOVE_None);

		Die(MontagesData->Montages.DeathMontage, FName("DeathFromHeight"));
	}
}

void AEntity::StunBehavior()
{
	GetMesh()->GlobalAnimRateScale = .5f;
	GetCharacterMovement()->MaxWalkSpeed = EntityData->MovementData.StunMaxWalkSpeed;
}
	
void AEntity::RemoveStunBehavior()
{
	GetMesh()->GlobalAnimRateScale = 1.f;
	GetCharacterMovement()->MaxWalkSpeed = CurrentStrategy->StrategyProperties.MaxWalkSpeed;
}

bool AEntity::IsEquipping() const
{
	return CharacterStateComponent->CurrentStates.WeaponState == ECharacterWeaponStates::ECWS_EquippingWeapon;
}

float AEntity::TakeDamage(const float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	LastDamageCauser = Cast<AEntity>(DamageCauser);

	if (GetAttributeComponent()->IsShielded() && DamageEvent.DamageTypeClass == USpectralTrapDamageType::StaticClass())
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), EffectsData->EntityEffects.ShieldImpactSFX, GetAttributeComponent()->GetShieldMeshComponent()->GetComponentLocation());
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
	if (IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Block, ECharacterActionsStates::ECAS_Finish, ECharacterActionsStates::ECAS_Dead, ECharacterActionsStates::ECAS_Stun })) return;
	
	GetExtraMovementComponent()->PerformMove(Value.Get<FVector2D>(), Value.Get<bool>());
}

void AEntity::Input_Look(const FInputActionValue& Value)
{
	if (!IsAlive()) return;
	
	GetExtraMovementComponent()->PerformLook(Value.Get<FVector2D>());	
}

void AEntity::Jump()
{
	if (GetCharacterStateComponent()->IsActionEqualToAny({ ECharacterActionsStates::ECAS_Nothing, ECharacterActionsStates::ECAS_Attack }) && GetExtraMovementComponent()->CanDoubleJump && !IsEquipping())
	{
		PlayAnimMontage(GetExtraMovementComponent()->JumpMontage, 1.f);

		Super::Jump();

		if (GetCharacterMovement()->IsFalling() && GetExtraMovementComponent()->CanDoubleJump)
		{
			Input_DoubleJump();
		}
	}
}	

void AEntity::Input_DoubleJump()
{
	if (!IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Block, ECharacterActionsStates::ECAS_Finish, ECharacterActionsStates::ECAS_Dead }))
	{
		GetExtraMovementComponent()->PerformDoubleJump();
	}
}

void AEntity::Input_Dodge()
{
	if (!IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Finish, ECharacterActionsStates::ECAS_Stun }))
	{
		GetExtraMovementComponent()->PerformDodge();
	}
}

void AEntity::OnPrimaryAttackStarted()
{
	if (GetCharacterStateComponent()->IsWeaponStateEqualToAny({ ECharacterWeaponStates::ECWS_EquippedWeapon }))
	{
		PerformPrimaryAttack();
		TimeOfPrimaryAttackPressed = GetWorld()->GetTimeSeconds();
	}
	else if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Entity does not have an equipped weapon.");
	}
}

void AEntity::Input_PrimaryAttack(const FInputActionValue& Value)
{
	if (IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Stun, ECharacterActionsStates::ECAS_Dead })) return;
	
	GetExtraMovementComponent()->bIsSaveDodge = false;
	GetCombatComponent()->bIsSaveHeavyAttack = false;
	
	if (GetCombatComponent()->CanAttack() && GetTargetingComponent()->IsLocking() && IsMovingBackwards())
	{
		if (GetWorld()->GetTimeSeconds() - TimeOfPrimaryAttackPressed > .8f)
		{
			SetAction(ECharacterActionsStates::ECAS_Attack);
			GetCombatComponent()->PerformLaunch(GetTargetingComponent()->GetCurrentTarget());
		}
	}
}

void AEntity::OnPrimaryAttackCompleted()
{
	TimeOfPrimaryAttackPressed = 0.f;
}

void AEntity::PerformPrimaryAttack()
{
	GetExtraMovementComponent()->bIsSaveDodge = false;
	GetCombatComponent()->bIsSaveHeavyAttack = false;
	
	if (!IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Attack, ECharacterActionsStates::ECAS_Dodge }))
	{
		GetCurrentStrategy()->Strategy_UseFirstAttack(GetCombatComponent()->IsInAir(), GetCurrentWeapon());
	}
	else
	{
		GetCombatComponent()->bIsSaveLightAttack = true;
	}
}

void AEntity::Input_SecondaryAttack()
{
	if (IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Stun, ECharacterActionsStates::ECAS_Dead })) return;

	GetExtraMovementComponent()->bIsSaveDodge = false;
	GetCombatComponent()->bIsSaveLightAttack = false;

	if (!IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Attack, ECharacterActionsStates::ECAS_Dodge }))
	{
		GetCurrentStrategy()->Strategy_UseSecondAttack(GetCombatComponent()->IsInAir(), GetCurrentWeapon());
	}
	else
	{
		GetCombatComponent()->bIsSaveHeavyAttack = true;
	}
}

void AEntity::Input_Block(const FInputActionValue& Value)
{
	if (IsActionStateEqualToAny({ ECharacterActionsStates::ECAS_Stun, ECharacterActionsStates::ECAS_Dead })) return;

	GetCombatComponent()->PerformBlock(Value.Get<bool>(), MontagesData->Montages.BlockMontage);
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

void AEntity::Input_Execute()
{
	if (IsActionStateEqualToAny({
		ECharacterActionsStates::ECAS_Dead, ECharacterActionsStates::ECAS_Finish, ECharacterActionsStates::ECAS_Stun
	})) return;
	
	GetCurrentStrategy()->Strategy_UseAbility(this);
}

void AEntity::Input_Interact(const FInputActionValue& InputActionValue)
{
	if (const TScriptInterface<IWeaponInterface> WeaponReached = GetInventoryComponent()->PerformInteract())
	{
		WeaponReached->OnWeaponUsed.AddDynamic(GetCombatComponent(), &UCombatComponent::StartAttackBufferEvent);
		WeaponReached->OnWeaponUsed.AddDynamic(GetTargetingComponent(), &UTargetingComponent::PerformSoftLock);
	}
}

void AEntity::Die(UAnimMontage* DeathAnim, const FName Section)
{
	if (GetCharacterStateComponent()->CurrentStates.Action == ECharacterActionsStates::ECAS_Dead) return;

	StopAnimMontage();
	GetCharacterStateComponent()->SetAction(ECharacterActionsStates::ECAS_Dead);

	if (GetCombatComponent()->IsInAir())
	{
		ACharacter::LaunchCharacter(FVector(0.f, 0.f, -300.f), true, true);
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	if (DeathAnim)
	{
		StopAnimMontage();
		PlayAnimMontage(DeathAnim, 1.f, Section);
	}
}

void AEntity::OnWallCollision()
{
	StopAnimMontage();
	HitReactJumpToSection(FName("ReactToShield"));
}