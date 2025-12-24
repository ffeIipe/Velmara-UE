#include "Items/Weapons/Sword.h"

#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Kismet/GameplayStatics.h"
#include "Interfaces/HitInterface.h"
#include "Engine/DamageEvents.h"
#include "NiagaraFunctionLibrary.h"

#include "DataAssets/Items/Weapons/SwordData.h"
#include "GAS/VelmaraAbilityInputID.h"
#include "GAS/VelmaraGameplayAbility.h"
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
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "MISSING! Data asset of: " + GetName() + " is nullptr.");
	}
}

void ASword::Pick(AActor* NewOwner)
{
	Super::Pick(NewOwner);

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(NewOwner);
	if (!ASI)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "ASI is nullptr");
		return;
	}

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC)
	{
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "ASC is nullptr");
		return;
	}
		
	if (SwordDataAsset && SwordDataAsset->WeaponTag.IsValid())
	{
		ASC->AddLooseGameplayTag(SwordDataAsset->WeaponTag);
	}

	if (SwordDataAsset)
	{
		for (TSubclassOf AbilityClass : SwordDataAsset->AbilitiesToGrant)
		{
			if (AbilityClass)
			{
				EVelmaraAbilityInputID InputID = EVelmaraAbilityInputID::None;
				if (UVelmaraGameplayAbility* VGA = Cast<UVelmaraGameplayAbility>(AbilityClass.GetDefaultObject()))
				{
					InputID = VGA->AbilityInputID;
				}

				FGameplayAbilitySpec Spec(AbilityClass, 1, static_cast<int32>(InputID), this); 
				FGameplayAbilitySpecHandle Handle = ASC->GiveAbility(Spec);
				GrantedAbilityHandles.Add(Handle);

				
			}
		}
	}
	
}

void ASword::Unequip()
{
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
	ItemMesh->AttachToComponent(InParent, TransformRules, SwordDataAsset->CustomInSocketName);
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
				OnHit(Hit.GetActor(), CalculateDamage());
				
				HitInterface->GetHit(GetOwner(), Hit.ImpactPoint, DamageEvent, SwordDataAsset->Stats.BaseDamage);
			}
			
			ImpactEffects(Hit, bIsHittable);
		}
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
