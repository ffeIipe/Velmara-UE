#include "Entities/Entity.h"

//Enhanced Input
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputActionValue.h"

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

AEntity::AEntity()
{
 	PrimaryActorTick.bCanEverTick = false;

	bUseControllerRotationPitch = false;
	bUseControllerRotationYaw = false;
	bUseControllerRotationRoll = false;

	GetCharacterMovement()->bOrientRotationToMovement = true;

	CombatComponent = CreateDefaultSubobject<UCombatComponent>(TEXT("Combat"));
	CombatComponent->FinisherLocation = CreateDefaultSubobject<USceneComponent>(TEXT("FinisherLocation"));
	CombatComponent->FinisherLocation->SetupAttachment(GetMesh());
	CombatComponent->FinisherLocation->SetRelativeLocation(FVector(-8.f, 80.f, 0.f));

	AttributeComponent = CreateDefaultSubobject<UAttributeComponent>(TEXT("Attribute"));
	
	CharacterStateComponent = CreateDefaultSubobject<UCharacterStateComponent>(TEXT("CharacterState"));
	
	ExtraMovementComponent = CreateDefaultSubobject<UExtraMovementComponent>(TEXT("ExtraMovement"));
	
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));
	
	MementoComponent = CreateDefaultSubobject<UMementoComponent>(TEXT("Memento"));
	
	PossessionComponent = CreateDefaultSubobject<UPossessionComponent>(TEXT("Possession"));

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm"));
	SpringArmComponent->SetupAttachment(GetRootComponent());
}

void AEntity::GetHit_Implementation(AActor* DamageCauser, const FVector& ImpactPoint, TSubclassOf<UDamageType> DamageType, const float DamageReceived)
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
	if (NiagaraSystem)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			GetWorld(),
			NiagaraSystem,
			ImpactPoint
		);
	}

	if (!IsEquipping())
	{
		GetCombatComponent()->GetDirectionalReact(ImpactPoint, DamageType);
	}
}

void AEntity::PlayCameraShake(const FVector& Epicenter, float InnerRadius, float OuterRadius)
{
	UGameplayStatics::PlayWorldCameraShake(this, CameraShake, Epicenter, InnerRadius, OuterRadius);
}

bool AEntity::CanBeFinished_Implementation()
{
	if (GetAttributeComponent()->GetHealthPercent() <= .2f)
	{
		if (OnCanBeFinished.IsBound())
		{
			OnCanBeFinished.Broadcast();
		}

		return true;
	}
	else return false;
}


void AEntity::SetWeaponCollisionEnabled(ECollisionEnabled::Type CollisionEnabled)
{
	if (InventoryComponent)
	{
		AItem* CurrentItem = InventoryComponent->GetEquippedItem();
		if (CurrentItem)
		{
			UPrimitiveComponent* ItemCollisionComponent = CurrentItem->GetCollisionComponent();
			if (ItemCollisionComponent)
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
		EnhancedInputComponent->BindAction(InputAction_Move, ETriggerEvent::Triggered, this, &AEntity::Move);
		EnhancedInputComponent->BindAction(InputAction_Look, ETriggerEvent::Triggered, this, &AEntity::Look);
		EnhancedInputComponent->BindAction(InputAction_Jump, ETriggerEvent::Started, this, &AEntity::Jump);
		EnhancedInputComponent->BindAction(InputAction_Dodge, ETriggerEvent::Started, this, &AEntity::Dodge);
		EnhancedInputComponent->BindAction(InputAction_Interact, ETriggerEvent::Started, this, &AEntity::Interact);
		EnhancedInputComponent->BindAction(InputAction_Attack, ETriggerEvent::Started, this, &AEntity::Attack);
		/*EnhancedInputComponent->BindAction(InputAction_HeavyAttack, ETriggerEvent::Started, this, &AEntity::HeavyAttack);
		EnhancedInputComponent->BindAction(InputAction_Launch, ETriggerEvent::Started, this, &AEntity::LaunchAttack);
		EnhancedInputComponent->BindAction(InputAction_Block, ETriggerEvent::Started, this, &AEntity::Block);
		EnhancedInputComponent->BindAction(InputAction_Block, ETriggerEvent::Completed, this, &AEntity::ReleaseBlock);*/

		//EnhancedInputComponent->BindAction(ChangeFormAction, ETriggerEvent::Started, this, &AEntity::ToggleForm);
		//EnhancedInputComponent->BindAction(PossessAction, ETriggerEvent::Started, this, &AEntity::PossessEnemy);
	}
}

void AEntity::Move(const FInputActionValue& Value)
{
	if (!GetCharacterStateComponent()->IsActionEqualToAny({ECharacterActions::ECA_Block, ECharacterActions::ECA_Finish, ECharacterActions::ECA_Dead, ECharacterActions::ECA_Stun}))
	{
		const FVector2D MoveVector = Value.Get<FVector2D>();

		const FRotator ControlRotation = GetControlRotation();
		const FRotator YawRotation(0.f, ControlRotation.Yaw, 0.f);

		const FVector DirectionForward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
		const FVector DirectionSideward = FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

		AddMovementInput(DirectionForward, MoveVector.Y);
		AddMovementInput(DirectionSideward, MoveVector.X);
	}
}

void AEntity::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	GetCombatComponent()->bIsLaunched = false;
	GetExtraMovementComponent()->CanDoubleJump = true;
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

void AEntity::Dodge(const FInputActionValue& Value)
{
	if (!IsEquipping())
	{
		GetExtraMovementComponent()->Input_Dodge();
	}
}

void AEntity::Look(const FInputActionValue& Value)
{
	const FVector2D LookingVector = Value.Get<FVector2D>();

	AddControllerPitchInput(LookingVector.Y);
	AddControllerYawInput(LookingVector.X);
}

void AEntity::Interact(const FInputActionValue& Value)
{
	if (GetInventoryComponent() && GetInventoryComponent()->IsInventoryOpen() && IsEquipping()) return;

	FVector TraceStart;
	FRotator CameraRotation;
	Controller->GetPlayerViewPoint(TraceStart, CameraRotation);

	FVector TraceDirection = CameraRotation.Vector();
	FVector TraceEnd = TraceStart + (TraceDirection * InteractTraceLenght);

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());

	FHitResult ResultHit;

	bool bHit = UKismetSystemLibrary::SphereTraceSingle(
		GetWorld(),
		TraceStart,
		TraceEnd,
		InteractTargetRadius,
		ETraceTypeQuery::TraceTypeQuery1, //visibility trace
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
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

void AEntity::Attack()
{
	if (!GetCharacterStateComponent()->IsActionEqualToAny({ ECharacterActions::ECA_Stun, ECharacterActions::ECA_Dead }) && !IsEquipping())
	{
		GetCombatComponent()->Input_Attack();
	}
}

void AEntity::HeavyAttack()
{
	if (!GetCharacterStateComponent()->IsActionEqualToAny({ ECharacterActions::ECA_Stun, ECharacterActions::ECA_Dead }) && !IsEquipping())
	{
		GetCombatComponent()->Input_HeavyAttack();
	}
}

void AEntity::LaunchAttack()
{
	if (!GetCharacterStateComponent()->IsActionEqualToAny({ ECharacterActions::ECA_Stun, ECharacterActions::ECA_Dead }) && !IsEquipping())
	{
		GetCombatComponent()->Input_Launch();
	}
}

void AEntity::Block()
{
	if (!GetCharacterStateComponent()->IsActionEqualToAny({ ECharacterActions::ECA_Stun, ECharacterActions::ECA_Dead }) && !IsEquipping())
	{
		GetCombatComponent()->Input_Block();
	}
}

void AEntity::ReleaseBlock()
{
	if (!GetCharacterStateComponent()->IsActionEqualToAny({ ECharacterActions::ECA_Stun, ECharacterActions::ECA_Dead }) && !IsEquipping())
	{
		GetCombatComponent()->Input_ReleaseBlock();
	}
}

void AEntity::Execute()
{
	if (!GetCharacterStateComponent()->IsActionEqualToAny({ ECharacterActions::ECA_Stun, ECharacterActions::ECA_Dead }) && !IsEquipping())
	{
		GetCombatComponent()->Input_Execute();
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

void AEntity::Die()
{
	if (GetCharacterStateComponent()->GetCurrentCharacterState().Action == ECharacterActions::ECA_Dead) return;

	GetCharacterMovement()->DisableMovement();
	GetCharacterStateComponent()->SetCharacterAction(ECharacterActions::ECA_Dead);

	if (DeathMontage)
	{
		StopAnimMontage();
		PlayAnimMontage(DeathMontage);
	}
}

//void AEntity::Equipping(bool bIsSwordBeingEquipped)
//{
//}

bool AEntity::IsEquipping()
{
	return GetCharacterStateComponent()->GetCurrentCharacterState().SpectralState == ECharacterSpectralStates::ECSS_EquippingPistol
		|| GetCharacterStateComponent()->GetCurrentCharacterState().State == ECharacterStates::ECS_EquippingSword;
}

float AEntity::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	LastDamageCauser = DamageCauser;

	if (GetAttributeComponent()->IsAlive())
	{
		if (GetAttributeComponent()->IsShielded() && DamageEvent.DamageTypeClass == USpectralTrapDamageType::StaticClass())
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShieldImpactSFX, GetAttributeComponent()->GetShieldMeshComponent()->GetComponentLocation());
			GetAttributeComponent()->ReceiveShieldDamage(DamageAmount);

			if (OnShieldTakeDamage.IsBound())
			{
				OnShieldTakeDamage.Broadcast();
			}
		}
		else if (!GetAttributeComponent()->IsShielded() && DamageEvent.DamageTypeClass != USpectralTrapDamageType::StaticClass())
		{
			GetAttributeComponent()->ReceiveDamage(DamageAmount);

			Execute_CanBeFinished(this);
		}
	}
	return DamageAmount;
}

void AEntity::OnWallCollision(const FHitResult& HitResult)
{
	StopAnimMontage();
	GetCombatComponent()->HitReactJumpToSection(FName("ReactToShield"));
}
