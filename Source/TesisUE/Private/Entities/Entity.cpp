#include "Entities/Entity.h"

#include "AbilitySystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Camera/CameraComponent.h"
#include "DamageTypes/BaseDamageType.h"
#include "DataAssets/EntityData.h"
#include "DataAssets/MontagesData.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "GAS/VelmaraAbilityInputID.h"
#include "GAS/VelmaraGameplayTags.h"
#include "GAS/VelmaraAttributeSet.h"
#include "GAS/VelmaraGameplayAbility.h"
#include "Items/Weapons/Sword.h"
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
	
	InventoryComponent = CreateDefaultSubobject<UInventoryComponent>(TEXT("Inventory"));

	TargetingComponent = CreateDefaultSubobject<UTargetingComponent>(TEXT("Targeting"));

	SpringArmComponent = CreateDefaultSubobject<USpringArmComponent>(TEXT("Spring Arm"));
	SpringArmComponent->SetupAttachment(GetRootComponent());

	CameraComponent = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	CameraComponent->SetupAttachment(GetSpringArmComponent());
	
	GetSpringArmComponent()->bUsePawnControlRotation = true;
}

void AEntity::GetOwnedGameplayTags(FGameplayTagContainer& TagContainer) const
{
	AbilitySystemComponent->GetOwnedGameplayTags(TagContainer);
}

void AEntity::AddGameplayTag(const FGameplayTag Tag) const
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->AddLooseGameplayTag(Tag);
	}
}

void AEntity::RemoveGameplayTag(const FGameplayTag Tag) const
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(Tag);
	}
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

void AEntity::GetHit(AActor* DamageCauser, const FVector& ImpactPoint, FDamageEvent const& DamageEvent, const float DamageReceived)
{
	if (!IsAlive()) return;

	//if (IsBlocking()) CombatComponent->ReceiveBlock(MontagesData->Montages.BlockMontage);
	
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

bool AEntity::IsHittable()
{
	return !IsShielded() && IsAlive() && !IsBlocking();
}

TScriptInterface<IWeaponInterface> AEntity::GetCurrentWeapon() const
{
	return GetInventoryComponent()->GetCurrentWeapon();
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

void AEntity::HitReactJumpToSection(const FName Section)
{
	if (MontagesData->Montages.HitReactMontage)
	{
		PlayAnimMontage(MontagesData->Montages.HitReactMontage, 1.f, Section);
	}
}

bool AEntity::IsEquipped() const
{
	if (!AbilitySystemComponent) return false;

	const bool bIsEquipped = IsValid(InventoryComponent->GetCurrentWeapon().GetObject()); //TODO: check

	return bIsEquipped;
}

bool AEntity::IsInAir()
{
	if (!GetCharacterMovementComponent()) return false;
	
	const bool bIsInAir = GetCharacterMovementComponent()->MovementMode == MOVE_Flying
	|| GetCharacterMovementComponent()->MovementMode == MOVE_Falling;

	return bIsInAir;
}

bool AEntity::IsBlocking() const
{
	if (!AbilitySystemComponent) return false;

	const bool bIsBlocking = AbilitySystemComponent->HasMatchingGameplayTag(FVelmaraGameplayTags::Get().State_Blocking);
	
	return bIsBlocking;
}

bool AEntity::CanBeFinished()
{
	const float HealthPercentage = FMath::Clamp(AttributeSet->GetHealth(), 0.f, 1.f);
	
	const bool bHasReachedLowHealth = HealthPercentage <= .4f;

	if (bHasReachedLowHealth)
	{
		if (OnCanBeFinished.IsBound()) OnCanBeFinished.Broadcast();
	}
	
	return bHasReachedLowHealth;
}

bool AEntity::IsLaunchable() const
{
	return !IsShielded();
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

void AEntity::InitializeAttributeSet()
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

void AEntity::GiveDefaultAbilities()
{
	if (AbilitySystemComponent && HasAuthority())
	{
		for (TSubclassOf AbilityClass : EntityData->DefaultAbilities)
		{
			if (AbilityClass)
			{
				EVelmaraAbilityInputID InputID = AbilityClass.GetDefaultObject()->AbilityInputID;
				FGameplayAbilitySpec Spec(AbilityClass, 1, static_cast<int32>(InputID), this);
                
				AbilitySystemComponent->GiveAbility(Spec);
			}
		}
	}
}

void AEntity::BeginPlay()
{
	Super::BeginPlay();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		InitializeAttributeSet();

		GiveDefaultAbilities();
	}
}

void AEntity::InitializeComponentsData() const
{
	if (EntityData)
	{
		InventoryComponent->InitializeValues(EntityData->InventoryData);
		
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

bool AEntity::IsAlive() const
{
	if (!AbilitySystemComponent) return false;

	const bool bHasHealth = AttributeSet && AttributeSet->GetHealth() > 0.0f;
	const bool bIsNotMarkedDead = !AbilitySystemComponent->HasMatchingGameplayTag(FVelmaraGameplayTags::Get().State_Dead);

	return bHasHealth && bIsNotMarkedDead; 
}

bool AEntity::IsShielded() const
{
	if (!AbilitySystemComponent) return false;
	
	const bool bHasShield = AttributeSet && AttributeSet->GetShield() > 0.0f;
	const bool bIsNotMarkedUnshielded = !AbilitySystemComponent->HasMatchingGameplayTag(FVelmaraGameplayTags::Get().State_Shielded);

	return bHasShield && bIsNotMarkedUnshielded;
}

bool AEntity::HasEnergy() const
{
	if (!AbilitySystemComponent) return false;

	const bool bHasEnergy = AttributeSet && AttributeSet->GetEnergy() > 0.0f;

	return bHasEnergy;
}

bool AEntity::IsStunned() const
{
	if (!AbilitySystemComponent) return false;

	const bool bIsStunned = AbilitySystemComponent->HasMatchingGameplayTag(FVelmaraGameplayTags::Get().State_Stunned);

	return bIsStunned;
}

void AEntity::Landed(const FHitResult& Hit)
{
	Super::Landed(Hit);
	
	if (IsAlive())
	{
		GetCharacterMovement()->DisableMovement();
		GetCharacterMovement()->StopMovementImmediately();
		GetCharacterMovement()->SetMovementMode(MOVE_None);

		Die(MontagesData->Montages.DeathMontage, FName("DeathFromHeight"));
	}
}

void AEntity::Die(UAnimMontage* DeathAnim, const FName Section)
{
	StopAnimMontage(GetCurrentMontage());
	AddGameplayTag(FVelmaraGameplayTags::Get().State_Dead);

	if (IsInAir())
	{
		ACharacter::LaunchCharacter(FVector(0.f, 0.f, -300.f), true, true);
		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
	}

	if (DeathAnim)
	{
		StopAnimMontage(GetCurrentMontage());
		PlayAnimMontage(DeathAnim, 1.f, Section);
	}
}