#include "Items/Weapons/Sword.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Interfaces/HitInterface.h"
#include "Engine/DamageEvents.h"
#include "Player/PlayerMain.h"
#include "Components/CharacterStateComponent.h"
#include <NiagaraFunctionLibrary.h>

#include "Interfaces/AnimatorProvider.h"
#include "Subsystems/EffectsManager.h"

ASword::ASword()
{
	BoxCollider->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	WeaponDamageBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Weapon Box"));
	WeaponDamageBox->SetupAttachment(GetRootComponent());
	WeaponDamageBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponDamageBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	WeaponDamageBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	BoxTraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace Start"));
	BoxTraceStart->SetupAttachment(GetRootComponent());

	BoxTraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace End"));
	BoxTraceEnd->SetupAttachment(GetRootComponent());
}

void ASword::BeginPlay()
{
	Super::BeginPlay();

	WeaponDamageBox->OnComponentBeginOverlap.AddDynamic(this, &ASword::OnBoxOverlap);
}

void ASword::Equip(USceneComponent* InParent, const FName InSocketName, AActor* NewOwner, APawn* NewInstigator)
{
	Super::Equip(InParent, InSocketName, NewOwner, NewInstigator);

	AttachMeshToSocket(InParent, InSocketName);
	SetOwner(NewOwner);
	SetInstigator(NewInstigator);
	ItemState = EItemState::EIS_Equipped;

	EnableSwordState(true);

	OwnerCharacterState = NewOwner;
	OwnerAnimator = NewOwner;
	if (OwnerCharacterState)
	{
		OwnerCharacterState->SetHumanState(ECharacterHumanStates::ECHS_EquippedSword);
	}
}

void ASword::Unequip()
{
	EnableSwordState(false);
	AttachMeshToSocket(OwnerAnimator->GetMesh(), FName("BackSocket"));
}

void ASword::AttachMeshToSocket(USceneComponent* InParent, const FName& InSocketName) const
{
	FAttachmentTransformRules TransformRules(EAttachmentRule::SnapToTarget, true);
	ItemMesh->AttachToComponent(InParent, TransformRules, InSocketName);
}

void ASword::EnableSwordState(const bool bEnable) const
{
	if (OwnerCharacterState)
	{
		const ECharacterHumanStates NewState = bEnable ? ECharacterHumanStates::ECHS_EquippedSword : ECharacterHumanStates::ECHS_Unequipped;
		OwnerCharacterState->SetHumanState(NewState);
	}
}

UPrimitiveComponent* ASword::GetCollisionComponent()
{
	return WeaponDamageBox;
}

int32 ASword::GetLightAttackComboMaxIndex()
{
	return LightAttackCombo.Num();
}

int32 ASword::GetHeavyAttackComboMaxIndex()
{
	return HeavyAttackCombo.Num();
}

int32 ASword::GetJumpAttackComboMaxIndex()
{
	return JumpAttackCombo.Num();
}

void ASword::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	const FVector Start = BoxTraceStart->GetComponentLocation();
	const FVector End = BoxTraceEnd->GetComponentLocation();

	IgnoreActors.Add(this);
	IgnoreActors.Add(GetOwner());

	TArray<FHitResult> HitResults;

	UKismetSystemLibrary::BoxTraceMulti(
		this,
		Start,
		End,
		FVector(25.f, 25.f, 25.f),
		BoxTraceStart->GetComponentRotation(),
		UEngineTypes::ConvertToTraceType(ECollisionChannel::ECC_GameTraceChannel3),
		false,
		IgnoreActors,
		EDrawDebugTrace::None,
		HitResults,
		true
	);

	for (const FHitResult& Hit : HitResults)
	{
		if (AActor* HitActor = Hit.GetActor())
		{
			if (IHitInterface* HitInterface = Cast<IHitInterface>(HitActor))
			{
				FDamageEvent DamageEvent(DamageTypeClass);
				HitInterface->GetHit(GetOwner(), Hit.ImpactPoint, DamageEvent, Damage);

				if (HitInterface->IsHittable())
				{
					UGameplayStatics::ApplyDamage(
						HitActor,
						CalculateDamage(),
						GetInstigator()->GetController(),
						GetOwner(),
						DamageTypeClass
					);

					//fx when the hit is true
					if (UEffectsManager* EffectsManager = GetWorld()->GetSubsystem<UEffectsManager>())
					{
						EffectsManager->HitStop(EHitStopPreset::EHSP_SwordHit);
						EffectsManager->CameraShake(ECameraShakePreset::ECSP_SwordHit, Hit.ImpactPoint);
					}

					if (HitEffect)
					{
						UNiagaraFunctionLibrary::SpawnSystemAtLocation(
							GetWorld(),
							HitEffect,
							Hit.ImpactPoint,
							Hit.ImpactNormal.Rotation(),
							FVector(1.f),
							true
						);
					}
					
					IgnoreActors.Add(HitActor);
				}
				else
				{
					if (SparksEffect)
					{
						UNiagaraFunctionLibrary::SpawnSystemAtLocation(
							GetWorld(),
							SparksEffect,
							Hit.ImpactPoint,
							Hit.ImpactNormal.Rotation(),
							FVector(1.f),
							true
						);
					}

					if (ShieldImpactSFX)
					{
						UGameplayStatics::PlaySoundAtLocation(GetWorld(), ShieldImpactSFX, Hit.ImpactPoint);
					}
				}
			}
		}
	}
}

void ASword::UsePrimaryAttack()
{
	// Super::UsePrimaryAttack();
	
}

float ASword::CalculateDamage() const
{
	if (FMath::FRandRange(0.f, 1.f) <= CriticalChance)
	{
		return Damage * CriticalDamageMultiplier;
	}
	return Damage;
}

void ASword::PerformLightAttack(const int32 ComboIndex)
{
	OwnerAnimator = GetOwner();
	if (OwnerAnimator)
		if (ComboIndex <= LightAttackCombo.Num())
			OwnerAnimator->PlayAnimMontage(LightAttackCombo[ComboIndex]);
}

void ASword::PerformHeavyAttack(const int32 ComboIndex)
{
	OwnerAnimator = GetOwner();
	if (OwnerAnimator)
		if (ComboIndex <= HeavyAttackCombo.Num())
			OwnerAnimator->PlayAnimMontage(HeavyAttackCombo[ComboIndex]);
		
}

void ASword::PerformJumpAttack(const int32 ComboIndex)
{
	OwnerAnimator = GetOwner();
	if (OwnerAnimator)
		if (ComboIndex <= JumpAttackCombo.Num())
			OwnerAnimator->PlayAnimMontage(JumpAttackCombo[ComboIndex]);
}

void ASword::SetWeaponCollisionEnabled(const ECollisionEnabled::Type CollisionEnabled)
{
	Super::SetWeaponCollisionEnabled(CollisionEnabled);

	WeaponDamageBox->SetCollisionEnabled(CollisionEnabled);
}
