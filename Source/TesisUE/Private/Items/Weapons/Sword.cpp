#include "Items/Weapons/Sword.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/UObjectHash.h"

#include "DataAssets/Items/Weapons/SwordData.h"
#include "Entities/Entity.h"
#include "Entities/WeaponOverlayAnimInstance.h"
#include "Features/GlobalEffectsSystem/Interfaces/EffectManagerProvider.h"
#include "Features/WeaponUpgradeSystem/Core/WeaponUpgradeComponent.h"
#include "GameFramework/Character.h"
#include "GAS/ComboNode.h"
#include "GAS/VelmaraAbilityInputID.h"
#include "GAS/VelmaraGameplayAbility.h"

ASword::ASword()
{
	SphereCollider->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

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

	if (!SwordData)
	{
	}
}

void ASword::Equip()
{
	Super::Equip();

	RefreshComboUnlocks();

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner());
	if (!ASI) return;

	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return;
		
	if (SwordData && SwordData->WeaponTag.IsValid())
	{
		if (ASC->HasMatchingGameplayTag(SwordData->WeaponTag)) return;
		
		ASC->AddLooseGameplayTag(SwordData->WeaponTag);
	}

	if (SwordData)
	{
		for (TSubclassOf AbilityClass : SwordData->AbilitiesToGrant)
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

		UWeaponOverlayAnimInstance::LinkToCharacter(Cast<ACharacter>(GetOwner()), SwordData, ItemMesh);
	}
}

FName ASword::GetEquipSocketName() const
{
	return SwordData && !SwordData->CustomInSocketName.IsNone() ? SwordData->CustomInSocketName : Super::GetEquipSocketName();
}

void ASword::Holster()
{
	Super::Holster();

	IAbilitySystemInterface* ASI = Cast<IAbilitySystemInterface>(GetOwner());
	if (!ASI) return;
	
	UAbilitySystemComponent* ASC = ASI->GetAbilitySystemComponent();
	if (!ASC) return;
	
	for (const FGameplayAbilitySpecHandle& Handle : GrantedAbilityHandles)
	{
		if (Handle.IsValid())
		{
			ASC->ClearAbility(Handle);
		}
	}

	GrantedAbilityHandles.Empty();
	
	ASC->RemoveLooseGameplayTag(SwordData->WeaponTag);

	if (!ASC->HasMatchingGameplayTag(SwordData->HolsterTag))
	{
		ASC->AddLooseGameplayTag(SwordData->HolsterTag);
	}

	UWeaponOverlayAnimInstance::UnlinkFromCharacter(Cast<ACharacter>(GetOwner()), SwordData);
}

void ASword::AttachMeshToSocket(USceneComponent* InParent, const FName InSocketName)
{
	Super::AttachMeshToSocket(InParent, InSocketName);

	const FAttachmentTransformRules TransformRules(
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::SnapToTarget,
		EAttachmentRule::KeepWorld,
		true
		);

	if (InSocketName == NAME_None)
	{
		ItemMesh->AttachToComponent(InParent, TransformRules, SwordData->CustomInSocketName);
	}
	else
	{
		ItemMesh->AttachToComponent(InParent, TransformRules, InSocketName);
	}
}

void ASword::OnBoxOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	TraceAndApplyHits(OverlappedComponent, BoxTraceStart, BoxTraceEnd);
}

void ASword::TraceAndApplyHits(UPrimitiveComponent* OverlappedComponent, const USceneComponent* TraceStart, const USceneComponent* TraceEnd)
{
	const FVector Start = TraceStart->GetComponentLocation();
	const FVector End = TraceEnd->GetComponentLocation();

	IgnoreActors.Add(this);
	IgnoreActors.Add(GetOwner());

	TArray<FHitResult> HitResults;

	UKismetSystemLibrary::BoxTraceMulti(
		this,
		Start,
		End,
		TraceHalfExtent,
		TraceStart->GetComponentRotation(),
		UEngineTypes::ConvertToTraceType(ECC_GameTraceChannel3),
		false,
		IgnoreActors,
		EDrawDebugTrace::None,
		HitResults,
		true
	);

	for (const FHitResult& Hit : HitResults)
	{
		if (IgnoreActors.Contains(Hit.GetActor())) continue;

		IgnoreActors.Add(Hit.GetActor());
		
		if (const TScriptInterface<IAbilitySystemInterface> TargetASI = Hit.GetActor())
		{
			UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
			
			if (TargetASC && DamageEffectSpecHandle.IsValid())
			{
				DamageEffectSpecHandle.Data.Get()->AddDynamicAssetTag(CurrentDamageTag);
				
				FGameplayEffectContextHandle Context = GetOwner()->FindComponentByClass<UAbilitySystemComponent>()->MakeEffectContext();
				Context.AddHitResult(Hit);

				DamageEffectSpecHandle.Data->SetContext(Context);
				
				GetOwner()->FindComponentByClass<UAbilitySystemComponent>()->ApplyGameplayEffectSpecToTarget(
					*DamageEffectSpecHandle.Data.Get(),
					TargetASC
				);
				
				FGameplayCueParameters CueParameters;
				CueParameters.Location = Hit.ImpactPoint;
				CueParameters.Normal = Hit.ImpactNormal;
				CueParameters.Instigator = GetOwner();
				CueParameters.TargetAttachComponent = OverlappedComponent;

				TargetASC->ExecuteGameplayCue(CurrentCueTag, CueParameters);

				if (const USwordData* Data = GetBaseSwordData())
				{
					if (AEntity* TargetEntity = Cast<AEntity>(Hit.GetActor()))
					{
						TargetEntity->ApplyKnockback(GetOwner()->GetActorLocation(), Data->Knockback);
					}
				}

				ApplyJuggle(Hit.GetActor());
			}
		}

		OnActorsWeaponHit(HitResults);
	}
}

void ASword::ApplyJuggle(AActor* Target)
{
	static const FGameplayTag LaunchTag = FGameplayTag::RequestGameplayTag(FName("Damage.Launch"));
	static const FGameplayTag FromAirTag = FGameplayTag::RequestGameplayTag(FName("Damage.FromAir"));

	const bool bLaunch = CurrentDamageTag.MatchesTag(LaunchTag);
	const bool bFromAir = CurrentDamageTag.MatchesTag(FromAirTag);
	if (!bLaunch && !bFromAir) return;

	AEntity* TargetEntity = Cast<AEntity>(Target);
	if (TargetEntity && TargetEntity->IsLaunchable() && (bLaunch || TargetEntity->IsInAir()))
	{
		TargetEntity->StartJuggle(bLaunch ? -1.f : TargetEntity->AirComboJuggleDuration);
	}

	AEntity* OwnerEntity = Cast<AEntity>(GetOwner());
	if (OwnerEntity && OwnerEntity->IsInAir())
	{
		OwnerEntity->StartAttackHover();
	}
}

void ASword::ClearIgnoreActors()
{
	IgnoreActors.Empty();
}

void ASword::SetWeaponCollisionEnabled(const ECollisionEnabled::Type CollisionEnabled)
{
	Super::SetWeaponCollisionEnabled(CollisionEnabled);

	WeaponDamageBox->SetCollisionEnabled(CollisionEnabled);

	UpdateBladeTrail(BladeTrail, BoxTraceStart, BoxTraceEnd, CollisionEnabled != ECollisionEnabled::NoCollision);
}

void ASword::UpdateBladeTrail(TObjectPtr<UNiagaraComponent>& Trail, USceneComponent* TraceStart, const USceneComponent* TraceEnd, const bool bActive)
{
	const USwordData* TrailSource = GetBaseSwordData();
	if (!TrailSource || !TrailSource->BladeTrailEffect || !TraceStart || !TraceEnd) return;

	if (!Trail)
	{
		if (!bActive) return;

		Trail = UNiagaraFunctionLibrary::SpawnSystemAttached(TrailSource->BladeTrailEffect, TraceStart, NAME_None, FVector::ZeroVector,
			FRotator::ZeroRotator, EAttachLocation::SnapToTarget, false, false);
		if (!Trail) return;
	}

	if (!bActive)
	{
		Trail->Deactivate();
		return;
	}

	const FVector TipLocal = TraceStart->GetComponentTransform().InverseTransformPosition(TraceEnd->GetComponentLocation());
	Trail->SetRelativeLocationAndRotation(TipLocal * 0.5f, TipLocal.Rotation());
	Trail->SetVariableFloat(FName("BladeLength"), TipLocal.Size());
	Trail->SetVariableVec3(FName("BladeTipLocal"), TipLocal);
	Trail->Activate(true);
}

void ASword::RefreshComboUnlocks()
{
	if (!BaseSwordData)
	{
		BaseSwordData = SwordData;
	}
	if (!BaseSwordData) return;

	UWeaponUpgradeComponent* Upgrades = GetOwner() ? GetOwner()->FindComponentByClass<UWeaponUpgradeComponent>() : nullptr;

	if (Upgrades != BoundUpgradeComponent.Get())
	{
		if (UWeaponUpgradeComponent* PreviousUpgrades = BoundUpgradeComponent.Get())
		{
			PreviousUpgrades->OnCombosChanged.RemoveDynamic(this, &ASword::HandleCombosChanged);
		}
		if (Upgrades)
		{
			Upgrades->OnCombosChanged.AddUniqueDynamic(this, &ASword::HandleCombosChanged);
		}
		BoundUpgradeComponent = Upgrades;
	}

	if (!Upgrades)
	{
		SwordData = BaseSwordData;
		return;
	}

	USwordData* FilteredData = DuplicateObject<USwordData>(BaseSwordData, this);

	auto MakeRuntimeOnly = [](UObject* Object)
	{
		Object->ClearFlags(RF_Standalone | RF_Public);
		Object->SetFlags(RF_Transient);
	};
	MakeRuntimeOnly(FilteredData);
	ForEachObjectWithOuter(FilteredData, MakeRuntimeOnly, true);

	TSet<UComboNode*> Visited;
	FilteredData->FirstGroundAction = PruneLockedBranches(FilteredData->FirstGroundAction, *Upgrades, *FilteredData, Visited);
	FilteredData->FirstAirAction = PruneLockedBranches(FilteredData->FirstAirAction, *Upgrades, *FilteredData, Visited);
	FilteredData->LaunchAction = PruneLockedBranches(FilteredData->LaunchAction, *Upgrades, *FilteredData, Visited);
	FilteredData->SkillAction = PruneLockedBranches(FilteredData->SkillAction, *Upgrades, *FilteredData, Visited);

	SwordData = FilteredData;
}

UComboNode* ASword::PruneLockedBranches(UComboNode* Node, const UWeaponUpgradeComponent& Upgrades, const USwordData& FilteredData, TSet<UComboNode*>& Visited) const
{
	if (!Node) return nullptr;
	if (!Upgrades.IsComboNodeUnlocked(Node)) return nullptr;

	if (!Visited.Contains(Node))
	{
		Visited.Add(Node);

		if (Node->IsIn(&FilteredData))
		{
			Node->NextActionInputTap = PruneLockedBranches(Node->NextActionInputTap, Upgrades, FilteredData, Visited);
			Node->NextActionInputHold = PruneLockedBranches(Node->NextActionInputHold, Upgrades, FilteredData, Visited);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("Sword: el nodo de combo %s no pertenece a %s, no se filtran sus ramas."),
			       *GetNameSafe(Node), *GetNameSafe(GetBaseSwordData()));
		}
	}

	return Node;
}

void ASword::HandleCombosChanged()
{
	RefreshComboUnlocks();
}