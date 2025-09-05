#include "Entities/Entity.h"

#include <NiagaraFunctionLibrary.h>

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/MementoComponent.h"
#include "DamageTypes/SpectralTrapDamageType.h"
#include "DataAssets/EntityData.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Interfaces/EntityAnimInstanceProvider.h"
#include "Items/Weapons/Sword.h"
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

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm"));
	SpringArmComponent->SetupAttachment(GetRootComponent());
	SpringArmComponent->bUsePawnControlRotation = true; //I think that I could solve the problem w/ the strange rotation of the hard lock system

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(GetSpringArmComponent());

	GetAttributeComponent()->OnOutOfEnergy.AddDynamic(this, &AEntity::OutOfEnergy);

	GetExtraMovementComponent()->OnDodgeStarted.AddDynamic(GetCombatComponent(), &UCombatComponent::RemoveCombatTarget);

	GetPossessionComponent()->OnPossessed.AddDynamic(GetAttributeComponent(), &UAttributeComponent::StartDecreaseEnergy);

	GetCombatComponent()->OnAttackEnd.AddDynamic(GetExtraMovementComponent(), &UExtraMovementComponent::ResetDodge);
}

void AEntity::GetHit(const TScriptInterface<ICombatTargetInterface> DamageCauser, const FVector& ImpactPoint,
	FDamageEvent const& DamageEvent, const float DamageReceived)
{
	if (GetCharacterStateComponent()->IsActionEqualToAny({ ECharacterActionsStates::ECAS_Dead })) return;

	if (IsBlocking())
	{
		/*if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Green, "Is Blocking...");*/
		GetCombatComponent()->ReceiveBlock();
	}
	
	LastDamageCauser = DamageCauser;
	
	if (ReceiveDamageSFX)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			ReceiveDamageSFX,
			GetTargetActorLocation()
		);
	}

	if (HitSound)
	{
		UGameplayStatics::PlaySoundAtLocation(
			this,
			HitSound,
			ImpactPoint
		);
	}
	
	if (ReceiveDamageFX)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			ReceiveDamageFX,
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

void AEntity::AttachFollowCamera()
{
	// if (const UCameraManagerSubsystem* CameraSubsystem = UCameraManagerSubsystem::Get(this))
	// {
	// 	FollowCamera = CameraSubsystem->GetMainCamera();
	//
	// 	if (!FollowCamera) return;
	// 	
	// 	FollowCamera->AttachToComponent(GetSpringArmComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("SpringEndpoint"));
	// 	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "FollowCamera attached to: " + GetName());
	// 	//GetController()->SetViewTargetWithBlend(FollowCamera, 1.f);
	// }
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

bool AEntity::IsWeaponStateEqualToAny(const TArray<ECharacterWeaponStates>& StatesToCheck) const
{
	return GetCharacterStateComponent()->IsHumanStateEqualToAny(StatesToCheck);
}

bool AEntity::IsActionStateEqualToAny(const TArray<ECharacterActionsStates>& ActionsToCheck) const
{
	return GetCharacterStateComponent()->IsActionEqualToAny(ActionsToCheck);
}

bool AEntity::IsModeStateEqualToAny(const TArray<ECharacterModeStates>& FormsToCheck) const
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

	PlayAnimMontage(HitReactMontage, 1.f, Section);
}

void AEntity::ChangeWeaponAnimationState()
{
	if (const TScriptInterface<IEntityAnimInstanceProvider> AnimInstance = GetMesh()->GetAnimInstance())
	{
		AnimInstance->SetAnimationState(GetWeaponEquipped());
	}
}

void AEntity::HitReactJumpToSection(FName Section)
{
	if (HitReactMontage)
	{
		PlayAnimMontage(HitReactMontage, 1.f, Section);
	}
}

bool AEntity::IsEquipped()
{
	return GetCharacterStateComponent()->IsHumanStateEqualToAny({ECharacterWeaponStates::ECWS_EquippedWeapon});
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

void AEntity::PlayCameraShake(const FVector& Epicenter, const float InnerRadius, const float OuterRadius)
{
	UGameplayStatics::PlayWorldCameraShake(this, CameraShake, Epicenter, InnerRadius, OuterRadius);
}

bool AEntity::IsLaunchable()
{
	return false;
}

void AEntity::SetWeaponCollisionEnabled(const ECollisionEnabled::Type CollisionEnabled)
{
	if (GetWeaponEquipped())
	{
		GetWeaponEquipped()->Execute_SetWeaponCollisionEnabled(GetWeaponEquipped().GetObject(), CollisionEnabled);
			
		if (CollisionEnabled != ECollisionEnabled::NoCollision)
		{
			GetWeaponEquipped()->ClearIgnoreActors();
		}
	}
}

void AEntity::BeginPlay()
{
	Super::BeginPlay();
	
	//PlayerControllerRef = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	GetMementoComponent()->SaveState();

	InitializeComponentsData();
}

void AEntity::InitializeComponentsData() const
{
	if (EntityData)
	{
		/*if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "On Construction");*/
		
		GetCombatComponent()->InitializeValues(EntityData->CombatData);
		GetExtraMovementComponent()->InitializeValues(EntityData->MovementData);
		GetAttributeComponent()->InitializeValues(EntityData->AttributeData);
		GetPossessionComponent()->InitializeValues(EntityData->PossessionData);
		GetInventoryComponent()->InitializeValues(EntityData->InventoryData);
	}
}

void AEntity::AttachFollowCamera(USpringArmComponent* AttachTarget)
{
	if (FollowCamera && PlayerControllerRef)
	{
		FollowCamera->AttachToComponent(AttachTarget, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("SpringEndpoint"));
		PlayerControllerRef->SetViewTargetWithBlend(FollowCamera, 1.f);
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

	if (UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		EnhancedInputComponent->BindAction(InputAction_Jump, ETriggerEvent::Started, this, &AEntity::Jump);

		EnhancedInputComponent->BindAction(InputAction_Move, ETriggerEvent::Triggered, GetExtraMovementComponent(), &UExtraMovementComponent::Input_Move);
		EnhancedInputComponent->BindAction(InputAction_Move, ETriggerEvent::Completed, GetExtraMovementComponent(), &UExtraMovementComponent::Input_Move);
		EnhancedInputComponent->BindAction(InputAction_Dodge, ETriggerEvent::Started, GetExtraMovementComponent(), &UExtraMovementComponent::Input_Dodge);
		EnhancedInputComponent->BindAction(InputAction_Look, ETriggerEvent::Triggered, GetExtraMovementComponent(), &UExtraMovementComponent::Input_Look);

		EnhancedInputComponent->BindAction(InputAction_Attack, ETriggerEvent::Started, GetCombatComponent(), &UCombatComponent::Input_Attack);
		EnhancedInputComponent->BindAction(InputAction_HeavyAttack, ETriggerEvent::Started, GetCombatComponent(), &UCombatComponent::Input_HeavyAttack);
		EnhancedInputComponent->BindAction(InputAction_Launch, ETriggerEvent::Started, GetCombatComponent(), &UCombatComponent::Input_Launch);
		EnhancedInputComponent->BindAction(InputAction_Inventory, ETriggerEvent::Started, GetCombatComponent(), &UCombatComponent::ToggleHardLock);
		EnhancedInputComponent->BindAction(InputAction_ChangeHardLockTarget, ETriggerEvent::Started, GetCombatComponent(), &UCombatComponent::ChangeHardLockTarget);
		EnhancedInputComponent->BindAction(InputAction_Block, ETriggerEvent::Triggered, GetCombatComponent(), &UCombatComponent::Input_Block);
		EnhancedInputComponent->BindAction(InputAction_Block, ETriggerEvent::Completed, GetCombatComponent(), &UCombatComponent::Input_Block);

		EnhancedInputComponent->BindAction(InputAction_Interact, ETriggerEvent::Started, GetInventoryComponent(), &UInventoryComponent::Interact);
		EnhancedInputComponent->BindAction(InputAction_ToggleWeapon, ETriggerEvent::Started, GetInventoryComponent(), &UInventoryComponent::ToggleInventorySlot);
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

		Die(DeathMontage, FName("DeathFromHeight"));
	}
}

void AEntity::Jump()
{
	if (GetCharacterStateComponent()->IsActionEqualToAny({ ECharacterActionsStates::ECAS_Nothing, ECharacterActionsStates::ECAS_Attack }) && GetExtraMovementComponent()->CanDoubleJump && !IsEquipping())
	{
		PlayAnimMontage(GetExtraMovementComponent()->JumpMontage, 1.f);

		Super::Jump();

		if (GetCharacterMovement()->IsFalling() && GetExtraMovementComponent()->CanDoubleJump)
		{
			GetExtraMovementComponent()->Input_DoubleJump();
		}
	}
}	

void AEntity::StunBehavior()
{
	GetMesh()->GlobalAnimRateScale = .5f;
	GetCharacterMovement()->MaxWalkSpeed = StunMaxWalkSpeed;
}
	
void AEntity::RemoveStunBehavior()
{
	GetMesh()->GlobalAnimRateScale = 1.f;
	GetCharacterMovement()->MaxWalkSpeed = DefaultMaxWalkSpeed;
}

bool AEntity::IsEquipping() const
{
	return GetCharacterStateComponent()->CurrentStates.WeaponState == ECharacterWeaponStates::ECWS_EquippingWeapon;
}

float AEntity::TakeDamage(const float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	LastDamageCauser = Cast<AEntity>(DamageCauser);

	if (GetAttributeComponent()->IsShielded() && DamageEvent.DamageTypeClass == USpectralTrapDamageType::StaticClass())
	{
		UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShieldImpactSFX, GetAttributeComponent()->GetShieldMeshComponent()->GetComponentLocation());
		GetAttributeComponent()->ReceiveShieldDamage(DamageAmount);

		if (OnShieldTakeDamage.IsBound())
		{
			OnShieldTakeDamage.Broadcast();
		}
	}
	else
	{
		GetAttributeComponent()->ReceiveDamage(DamageAmount);
		CanBeFinished();
	}
	return DamageAmount;
}

void AEntity::OutOfEnergy()
{
	if (GetPossessionComponent()->GetPossessedEntity())
	{
		GetPossessionComponent()->ReleasePossession();
	}
}

void AEntity::Die(UAnimMontage* DeathAnim, FName Section)
{
	if (GetCharacterStateComponent()->CurrentStates.Action == ECharacterActionsStates::ECAS_Dead) return;

	GetCharacterStateComponent()->SetAction(ECharacterActionsStates::ECAS_Dead);

	if (GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Falling || GetCharacterMovement()->MovementMode == EMovementMode::MOVE_Flying)
	{
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_Falling);
	}

	if (DeathAnim)
	{
		StopAnimMontage();
		PlayAnimMontage(DeathAnim, 1.f, Section);
	}
}

void AEntity::OnWallCollision(const FHitResult& HitResult)
{
	StopAnimMontage();
	HitReactJumpToSection(FName("ReactToShield"));
}