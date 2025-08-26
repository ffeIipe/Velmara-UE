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
}

void AEntity::GetHit_Implementation(AEntity* DamageCauser, const FVector& ImpactPoint, FDamageEvent const& DamageEvent, const float DamageReceived)
{
	if (GetCharacterStateComponent()->IsActionEqualToAny({ ECharacterActions::ECA_Dead })) return;

	LastDamageCauser = DamageCauser;
	
	if (ReceiveDamageSFX)
	{
		UGameplayStatics::PlaySoundAtLocation(
			GetWorld(),
			ReceiveDamageSFX,
			GetActorLocation()
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
		GetCombatComponent()->GetDirectionalReact(ImpactPoint);
	}
}

void AEntity::PlayCameraShake(const FVector& Epicenter, const float InnerRadius, const float OuterRadius)
{
	UGameplayStatics::PlayWorldCameraShake(this, CameraShake, Epicenter, InnerRadius, OuterRadius);
}

bool AEntity::CanBeFinished_Implementation()
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

bool AEntity::IsLaunchable_Implementation()
{
	return false;
}

void AEntity::SetWeaponCollisionEnabled(const ECollisionEnabled::Type CollisionEnabled)
{
	if (InventoryComponent)
	{
		if (AItem* CurrentItem = InventoryComponent->GetEquippedItem())
		{
			if (UPrimitiveComponent* ItemCollisionComponent = CurrentItem->GetCollisionComponent())
			{
				ItemCollisionComponent->SetCollisionEnabled(CollisionEnabled);
				if (ASword* Sword = Cast<ASword>(CurrentItem))
				{
					if (CollisionEnabled != ECollisionEnabled::NoCollision)
					{
						Sword->IgnoreActors.Empty();
					}
				}
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

void AEntity::InitializeComponentsData() const
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

void AEntity::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	InitializeComponentsData();
}

void AEntity::AttachFollowCamera(USpringArmComponent* AttachTarget)
{
	if (FollowCamera && PlayerControllerRef)
	{
		FollowCamera->AttachToComponent(AttachTarget, FAttachmentTransformRules::SnapToTargetNotIncludingScale, FName("SpringEndpoint"));
		PlayerControllerRef->SetViewTargetWithBlend(FollowCamera, 1.f);
	}
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
		EnhancedInputComponent->BindAction(InputAction_Block, ETriggerEvent::Triggered, GetExtraMovementComponent(), &UExtraMovementComponent::Input_Run);
		EnhancedInputComponent->BindAction(InputAction_Block, ETriggerEvent::Completed, GetExtraMovementComponent(), &UExtraMovementComponent::Input_Run);

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
		if (ASword* HitSword = Cast<ASword>(ResultHit.GetActor()))
		{
			if (GetCharacterStateComponent()->GetCurrentCharacterState().Form != ECharacterForm::ECF_Spectral)
			{
				if (GetInventoryComponent()->TryAddItem(HitSword))
				{
					ActorsToIgnore.Add(HitSword);
					HitSword->OnWallHit.AddDynamic(this, &AEntity::OnWallCollision);

					Equipping(true);
				}
			}
		}
		else if (ISpectralInteractable* SpectralObjectInteractable = Cast<ISpectralInteractable>(ResultHit.GetActor()))
		{
			if (GetCharacterStateComponent()->GetCurrentCharacterState().Form == ECharacterForm::ECF_Spectral)
			{
				SpectralObjectInteractable->Execute_SpectralInteract(ResultHit.GetActor(), this);
			}
		}
		else if (AItem* HitItem = Cast<AItem>(ResultHit.GetActor()))
		{
			HitItem->Use(this);
			Equipping(false);
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

bool AEntity::IsEquipping()
{
	return GetCharacterStateComponent()->GetCurrentCharacterState().SpectralState == ECharacterSpectralStates::ECSS_EquippingPistol
		|| GetCharacterStateComponent()->GetCurrentCharacterState().State == ECharacterStates::ECS_EquippingSword;
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
		CanBeFinished_Implementation();
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
	if (GetCharacterStateComponent()->GetCurrentCharacterState().Action == ECharacterActions::ECA_Dead) return;

	GetCharacterStateComponent()->SetCharacterAction(ECharacterActions::ECA_Dead);

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
	GetCombatComponent()->HitReactJumpToSection(FName("ReactToShield"));
}