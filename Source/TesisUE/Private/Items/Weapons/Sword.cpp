#include "Items/Weapons/Sword.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Interfaces/HitInterface.h"
#include "Engine/DamageEvents.h"
#include "Player/PlayerMain.h"
#include "NiagaraFunctionLibrary.h"

#include "DamageTypes/MeleeDamage.h" //don't delete pls
#include "DataAssets/Items/Weapons/SwordDataAsset.h"
#include "Items/Weapons/Commands/ComboCommand.h"
#include "Subsystems/EffectsManager.h"

ASword::ASword()
{
	BoxCollider->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	WeaponDamageBox = CreateDefaultSubobject<UBoxComponent>(TEXT("Weapon Box"));
	WeaponDamageBox->SetupAttachment(GetRootComponent());
	WeaponDamageBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	WeaponDamageBox->SetCollisionResponseToAllChannels(ECR_Overlap);
	WeaponDamageBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
	WeaponDamageBox->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	WeaponDamageBox->OnComponentBeginOverlap.AddDynamic(this, &ASword::OnBoxOverlap);

	BoxTraceStart = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace Start"));
	BoxTraceStart->SetupAttachment(GetRootComponent());

	BoxTraceEnd = CreateDefaultSubobject<USceneComponent>(TEXT("Box Trace End"));
	BoxTraceEnd->SetupAttachment(GetRootComponent());
}

void ASword::BeginPlay()
{
	Super::BeginPlay();

	if (!SwordDataAsset)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "MISSING! Data asset is nullptr.");
		return;
	}
	
	if (!LightCommandInstance)
	{
		if (SwordDataAsset->LightComboCommandClass) LightCommandInstance = NewObject<UCommand>(this, SwordDataAsset->LightComboCommandClass);
		else if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5, FColor::Red, "MISSING Light Combo Command blueprint class in: " + GetName());
	}

	if (!JumpCommandInstance)
	{
		if (SwordDataAsset->JumpComboCommandClass) JumpCommandInstance = NewObject<UCommand>(this, SwordDataAsset->JumpComboCommandClass);
		else if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5, FColor::Red, "MISSING Jump Combo Command blueprint class in: " + GetName());
	}

	if (!HeavyCommandInstance)
	{
		if (SwordDataAsset->HeavyComboCommandClass) HeavyCommandInstance = NewObject<UCommand>(this, SwordDataAsset->HeavyComboCommandClass);
		else if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5, FColor::Red, "MISSING Heavy Combo Command blueprint class in: " + GetName());	
	}

	if (!HeavyJumpCommandInstance)
	{
		if (SwordDataAsset->HeavyJumpComboCommandClass) HeavyJumpCommandInstance = NewObject<UCommand>(this, SwordDataAsset->HeavyJumpComboCommandClass);
		else if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5, FColor::Red, "MISSING Heavy Combo Command blueprint class in: " + GetName());	
	}

	if (!AbilityCommandInstance)
	{
		if (SwordDataAsset->AbilityCommandClass) AbilityCommandInstance = NewObject<UCommand>(this, SwordDataAsset->AbilityCommandClass);
	}
}

void ASword::Unequip()
{
	EnableWeaponState(false);
	/*AttachMeshToSocket(AnimatorProvider->GetMeshComponent());*/
}

UPrimitiveComponent* ASword::GetCollisionComponent()
{
	return WeaponDamageBox;
}

void ASword::AttachMeshToSocket(USceneComponent* InParent)
{
	Super::AttachMeshToSocket(InParent);

	const FAttachmentTransformRules TransformRules(EAttachmentRule::SnapToTarget, true);
	ItemMesh->AttachToComponent(InParent, TransformRules, SwordDataAsset->Effects.CustomInSocketName);
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
		UEngineTypes::ConvertToTraceType(ECC_GameTraceChannel3),
		false,
		IgnoreActors,
		EDrawDebugTrace::None,
		HitResults,
		true
	);

	for (const FHitResult& Hit : HitResults)
	{
		if (IgnoreActors.Contains(Hit.GetActor())) return;
		
		if (const TScriptInterface<IHitInterface> HitInterface = Hit.GetActor())
		{
			IgnoreActors.Add(Hit.GetActor());
			
			FDamageEvent DamageEvent(DamageTypeClass);

			const bool bIsHittable = HitInterface->IsHittable();
			
			if (bIsHittable)
			{
				UGameplayStatics::ApplyDamage(
					Hit.GetActor(),
					CalculateDamage(),
					ControllerProvider->GetEntityController(),
					GetOwner(),
					DamageTypeClass
				);

				HitInterface->GetHit(GetOwner(), Hit.ImpactPoint, DamageEvent, SwordDataAsset->Stats.BaseDamage);
			}
			ImpactEffects(Hit, bIsHittable);
		}
	}
}

void ASword::UsePrimaryAttack_Implementation()
{
	Super::UsePrimaryAttack_Implementation();

	const TScriptInterface<ICombatComponentProvider> CombatComp = GetOwner();
	CombatComp->Execute_GetCombatComponent(GetOwner())->IsInAir() ? PerformBaseAttack(JumpCommandInstance) : PerformBaseAttack(LightCommandInstance);
}

void ASword::UseSecondaryAttack_Implementation()
{
	Super::UseSecondaryAttack_Implementation();

	const TScriptInterface<ICombatComponentProvider> CombatComp = GetOwner();
	CombatComp->Execute_GetCombatComponent(GetOwner())->IsInAir() ? PerformBaseAttack(HeavyJumpCommandInstance) : PerformBaseAttack(HeavyCommandInstance);
}

void ASword::UseAbilityAttack_Implementation()
{
	Super::UseAbilityAttack_Implementation();
	
}

void ASword::SetDamageType_Implementation(const TSubclassOf<UMeleeDamage> DamageType)
{
	Super::SetDamageType_Implementation(DamageType);

	DamageTypeClass = DamageType;
}

void ASword::ResetWeapon_Implementation()
{
	Super::ResetWeapon_Implementation();

	if (LightCommandInstance)
	{
		LightCommandInstance->ResetCommand();
	}

	if (HeavyCommandInstance)
	{
		HeavyCommandInstance->ResetCommand();
	}

	if (HeavyJumpCommandInstance)
	{
		HeavyJumpCommandInstance->ResetCommand();
	}

	if (JumpCommandInstance)
	{
		JumpCommandInstance->ResetCommand();
	}
}

float ASword::CalculateDamage() const
{
	const float CriticalDamage = SwordDataAsset->Stats.BaseDamage;
	
	if (FMath::FRandRange(0.f, 1.f) <= SwordDataAsset->Stats.CriticalChance)
	{
		return CriticalDamage * SwordDataAsset->Stats.CriticalDamageMultiplier;
	}
	return CriticalDamage;
}

bool ASword::PerformBaseAttack(UCommand* CommToPlay) const
{
	if (!CommToPlay)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 5, FColor::Yellow, "Missing Anim/Anims to play");
		return false;
	}
	
	CharacterStateProvider->Execute_GetCharacterStateComponent(GetOwner())->SetAction(ECharacterActionsStates::ECAS_Attack);
	CommToPlay->ExecuteCommand(GetOwner());
	
	if (OnWeaponUsed.IsBound()) OnWeaponUsed.Broadcast();
	
	return true;
}

void ASword::ClearIgnoreActors()
{
	IgnoreActors.Empty();
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Yellow, FString("From " + GetName() + ": Ignore Actors"));
}

void ASword::SetWeaponCollisionEnabled(const ECollisionEnabled::Type CollisionEnabled)
{
	Super::SetWeaponCollisionEnabled(CollisionEnabled);
	
	WeaponDamageBox->SetCollisionEnabled(CollisionEnabled);
}

void ASword::ImpactEffects(const FHitResult& Hit, const bool bIsHittable) const
{
	if (bIsHittable)
	{
		if (UEffectsManager* EffectsManager = GetWorld()->GetSubsystem<UEffectsManager>())
		{
			EffectsManager->HitStop(SwordDataAsset->Effects.HitStopPreset);
			EffectsManager->CameraShake(SwordDataAsset->Effects.CameraShakePreset, Hit.ImpactPoint);
		}

		if (SwordDataAsset)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				SwordDataAsset->Effects.HitEffect,
				Hit.ImpactPoint,
				Hit.ImpactNormal.Rotation(),
				FVector(1.f),
				true
			);
		}
	}
	else
	{
		if (SwordDataAsset)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(
				GetWorld(),
				SwordDataAsset->Effects.SparksEffect,
				Hit.ImpactPoint,
				Hit.ImpactNormal.Rotation(),
				FVector(1.f),
				true
			);
		}

		if (SwordDataAsset)
		{
			UGameplayStatics::PlaySoundAtLocation(GetWorld(), SwordDataAsset->Effects.ShieldImpact, Hit.ImpactPoint);
		}
	}
}
