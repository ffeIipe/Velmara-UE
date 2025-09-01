#include "Entities/Entity.h"

//Enhanced Input
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"

//Components
#include "Components/CombatComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Components/ExtraMovementComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/PossessionComponent.h"
#include "Components/MementoComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Items/Weapons/Sword.h"
#include "Kismet/GameplayStatics.h"

//Misc
#include "DamageTypes/SpectralTrapDamageType.h"
#include <SpectralMode/Interfaces/SpectralInteractable.h>
#include <NiagaraFunctionLibrary.h>
#include <vector>

#include "Engine/DamageEvents.h"
#include "Camera/CameraActor.h"
#include "DataAssets/EntityData.h"

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
	SpringArmComponent->bUsePawnControlRotation = true;

	GetAttributeComponent()->OnOutOfEnergy.AddDynamic(this, &AEntity::OutOfEnergy);
	
	GetExtraMovementComponent()->OnDodgeStarted.AddDynamic(GetCombatComponent(), &UCombatComponent::RemoveCombatTarget);

	GetPossessionComponent()->OnPossessed.AddDynamic(GetAttributeComponent(), &UAttributeComponent::StartDecreaseEnergy);
}

void AEntity::GetHit(TScriptInterface<ICombatTargetInterface> DamageCauser, const FVector& ImpactPoint,
	FDamageEvent const& DamageEvent, const float DamageReceived)
{
	if (GetCharacterStateComponent()->IsActionEqualToAny({ ECharacterActions::ECA_Dead })) return;

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

	if (!IsEquipping())
	{
		GetDirectionalReact(ImpactPoint);
	}	
}

void AEntity::GetFinished()
{
	
}

bool AEntity::IsHittable()
{
	return !GetAttributeComponent()->IsShielded() && IsAlive();
}

void AEntity::AttachFollowCamera()
{
}

ECharacterHumanStates AEntity::SetHumanState(ECharacterHumanStates NewState)
{
	return  GetCharacterStateComponent()->SetHumanState(NewState);
}

ECharacterSpectralStates AEntity::SetSpectralState(ECharacterSpectralStates NewSpectralState)
{
	return GetCharacterStateComponent()->SetSpectralState(NewSpectralState);
}

ECharacterActions AEntity::SetAction(ECharacterActions NewAction)
{
	return GetCharacterStateComponent()->SetAction(NewAction);
}

ECharacterMode AEntity::SetMode(ECharacterMode NewForm)
{
	return GetCharacterStateComponent()->SetMode(NewForm);
}

bool AEntity::IsHumanStateEqualToAny(const TArray<ECharacterHumanStates>& StatesToCheck) const
{
	return GetCharacterStateComponent()->IsHumanStateEqualToAny(StatesToCheck);
}

bool AEntity::IsSpectralStateEqualToAny(const TArray<ECharacterSpectralStates>& SpectralStatesToCheck) const
{
	return GetCharacterStateComponent()->IsSpectralStateEqualToAny(SpectralStatesToCheck);
}

bool AEntity::IsActionEqualToAny(const TArray<ECharacterActions>& ActionsToCheck) const
{
	return GetCharacterStateComponent()->IsActionEqualToAny(ActionsToCheck);
}

bool AEntity::IsModeEqualToAny(const TArray<ECharacterMode>& FormsToCheck) const
{
	return GetCharacterStateComponent()->IsModeEqualToAny(FormsToCheck);
}

void AEntity::GetDirectionalReact(const FVector& ImpactPoint)
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

	PlayAnimMontage(HitReactMontage, 1.f, Section);
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
	return GetCharacterStateComponent()->IsHumanStateEqualToAny({ECharacterHumanStates::ECHS_EquippedSword});
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
	if (InventoryComponent)
	{
		if (InventoryComponent->GetWeaponEquipped())
		{
			GetWeaponEquipped()->SetWeaponCollisionEnabled(CollisionEnabled);
			
			if (CollisionEnabled != ECollisionEnabled::NoCollision)
			{
				GetWeaponEquipped()->ClearIgnoreActors();
			}
		}
	}
}

void AEntity::BeginPlay()
{
	Super::BeginPlay();
	
	PlayerControllerRef = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	GetMementoComponent()->SaveState();

	InitializeComponentsData();
}

void AEntity::InitializeComponentsData()
{
	if (EntityData)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "On Construction");
		
		GetCombatComponent()->InitializeValues(EntityData->CombatData);
		GetExtraMovementComponent()->InitializeValues(EntityData->MovementData);
		GetAttributeComponent()->InitializeValues(EntityData->AttributeData);
		GetPossessionComponent()->InitializeValues(EntityData->PossessionData);
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
		EnhancedInputComponent->BindAction(InputAction_Interact, ETriggerEvent::Started, this, &AEntity::Interact);

		EnhancedInputComponent->BindAction(InputAction_Move, ETriggerEvent::Triggered, GetExtraMovementComponent(), &UExtraMovementComponent::Input_Move);
		EnhancedInputComponent->BindAction(InputAction_Dodge, ETriggerEvent::Started, GetExtraMovementComponent(), &UExtraMovementComponent::Input_Dodge);
		EnhancedInputComponent->BindAction(InputAction_Look, ETriggerEvent::Triggered, GetExtraMovementComponent(), &UExtraMovementComponent::Input_Look);

		EnhancedInputComponent->BindAction(InputAction_Attack, ETriggerEvent::Started, GetCombatComponent(), &UCombatComponent::Input_Attack);
		EnhancedInputComponent->BindAction(InputAction_HeavyAttack, ETriggerEvent::Started, GetCombatComponent(), &UCombatComponent::Input_HeavyAttack);
		EnhancedInputComponent->BindAction(InputAction_Launch, ETriggerEvent::Started, GetCombatComponent(), &UCombatComponent::Input_Launch);
		EnhancedInputComponent->BindAction(InputAction_Inventory, ETriggerEvent::Started, GetCombatComponent(), &UCombatComponent::ToggleHardLock);
		EnhancedInputComponent->BindAction(InputAction_ChangeHardLockTarget, ETriggerEvent::Started, GetCombatComponent(), &UCombatComponent::ChangeHardLockTarget);
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
		GetCharacterMovement()->SetMovementMode(EMovementMode::MOVE_None);

		Die(DeathMontage, FName("DeathFromHeight"));
	}
}

void AEntity::Jump()
{
	if (GetCharacterStateComponent()->IsActionEqualToAny({ ECharacterActions::ECA_Nothing, ECharacterActions::ECA_Attack }) && GetExtraMovementComponent()->CanDoubleJump && !IsEquipping())
	{
		PlayAnimMontage(GetExtraMovementComponent()->JumpMontage, 1.f);

		Super::Jump();

		if (GetCharacterMovement()->IsFalling() && GetExtraMovementComponent()->CanDoubleJump)
		{
			GetExtraMovementComponent()->Input_DoubleJump();
		}
	}
}	

void AEntity::Interact(const FInputActionValue& Value)
{
	if (GetInventoryComponent() && GetInventoryComponent()->IsInventoryOpen() && IsEquipping()) return;

	FVector TraceStart;
	FRotator CameraRotation;
	Controller->GetPlayerViewPoint(TraceStart, CameraRotation);

	const FVector TraceDirection = CameraRotation.Vector();
	const FVector TraceEnd = TraceStart + (TraceDirection * InteractTraceLenght);

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());

	FHitResult ResultHit;

	const bool bHit = UKismetSystemLibrary::SphereTraceSingle(
		GetWorld(),
		TraceStart,
		TraceEnd,
		InteractTargetRadius,
		ETraceTypeQuery::TraceTypeQuery1, //visibility trace
		false,
		ActorsToIgnore,
		EDrawDebugTrace::None,
		ResultHit,
		true
	);


	if (bHit && GetInventoryComponent())
	{
		if (AItem* HitSword = Cast<AItem>(ResultHit.GetActor()))
		{
			if (GetCharacterStateComponent()->CurrentStates.Mode != ECharacterMode::ECM_Spectral)
			{
				if (GetInventoryComponent()->TryAddWeapon(HitSword))
				{
					ActorsToIgnore.Add(HitSword);
					//HitSword->OnWallHit.AddDynamic(this, &AEntity::OnWallCollision);
				}
			}
		}
		else if (ISpectralInteractable* SpectralObjectInteractable = Cast<ISpectralInteractable>(ResultHit.GetActor()))
		{
			if (GetCharacterStateComponent()->CurrentStates.Mode == ECharacterMode::ECM_Spectral)
			{
				SpectralObjectInteractable->Execute_SpectralInteract(ResultHit.GetActor(), this);
			}
		}
		// else if (AItem* HitItem = Cast<AItem>(ResultHit.GetActor()))
		// {
		// 	HitItem->Use(this);
		// 	Equipping(false);
		// }
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

bool AEntity::IsEquipping()
{
	return GetCharacterStateComponent()->CurrentStates.SpectralState == ECharacterSpectralStates::ECSS_EquippingPistol
		|| GetCharacterStateComponent()->CurrentStates.HumanState == ECharacterHumanStates::ECHS_EquippingSword;
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
	if (GetCharacterStateComponent()->CurrentStates.Action == ECharacterActions::ECA_Dead) return;

	GetCharacterStateComponent()->SetAction(ECharacterActions::ECA_Dead);

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