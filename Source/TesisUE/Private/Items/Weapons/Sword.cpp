#include "Items/Weapons/Sword.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/BoxComponent.h"
#include "Components/SphereComponent.h"
#include "Kismet/KismetSystemLibrary.h"
#include "UObject/UObjectHash.h"

#include "DataAssets/Items/Weapons/SwordData.h"
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
		//if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "MISSING! Data asset of: " + GetName() + " is nullptr.");
	}
}

void ASword::Equip()
{
	Super::Equip();

	// Antes de dar las habilidades, para que el combo arranque con los desbloqueos del dueño.
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

		if (IsValid(SwordData->AnimLayer))
		{
			if (const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner()))
			{
				CharacterOwner->GetMesh()->GetAnimInstance()->LinkAnimClassLayers(SwordData->AnimLayer);
			}
		}
	}
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

	if (IsValid(SwordData->AnimLayer))
	{
		if (const ACharacter* CharacterOwner = Cast<ACharacter>(GetOwner()))
		{
			CharacterOwner->GetMesh()->GetAnimInstance()->UnlinkAnimClassLayers(SwordData->AnimLayer);
		}
	}
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
	const FVector Start = BoxTraceStart->GetComponentLocation();
	const FVector End = BoxTraceEnd->GetComponentLocation();

	IgnoreActors.Add(this);
	IgnoreActors.Add(GetOwner());

	TArray<FHitResult> HitResults;

	UKismetSystemLibrary::BoxTraceMulti(
		this,
		Start,
		End,
		FVector(20.f, 20.f, 20.f),
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

		IgnoreActors.Add(Hit.GetActor());
		
		if (const TScriptInterface<IAbilitySystemInterface> TargetASI = Hit.GetActor())
		{
			UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
			
			if (TargetASC && DamageEffectSpecHandle.IsValid())
			{
				//Set the damage type
				DamageEffectSpecHandle.Data.Get()->AddDynamicAssetTag(CurrentDamageTag);
				
				//Also sending the hit result
				FGameplayEffectContextHandle Context = GetOwner()->FindComponentByClass<UAbilitySystemComponent>()->MakeEffectContext();
				Context.AddHitResult(Hit);

				DamageEffectSpecHandle.Data->SetContext(Context);
				
				//Applying the gameplay effect to the hit actor
				GetOwner()->FindComponentByClass<UAbilitySystemComponent>()->ApplyGameplayEffectSpecToTarget(
					*DamageEffectSpecHandle.Data.Get(),
					TargetASC
				);
				
				//VFX call
				FGameplayCueParameters CueParameters;
				CueParameters.Location = Hit.ImpactPoint;
				CueParameters.Normal = Hit.ImpactNormal;
				CueParameters.Instigator = GetOwner();
				CueParameters.TargetAttachComponent = OverlappedComponent;

				TargetASC->ExecuteGameplayCue(CurrentCueTag, CueParameters);

				/*if (GetGameInstance()->Implements<UEffectManagerProvider>())
				{
					IEffectManagerProvider::Execute_PlayGameplayEffect(GetGameInstance(), CurrentDamageTag, Hit.ImpactPoint);
				}*/
			}
		}

		OnActorsWeaponHit(HitResults);
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

	// Sin sistema de mejoras (ej. enemigos) se usan todos los combos del Data Asset.
	if (!Upgrades)
	{
		SwordData = BaseSwordData;
		return;
	}

	// Nunca se modifica el Data Asset: se trabaja sobre una copia propia de esta espada.
	USwordData* FilteredData = DuplicateObject<USwordData>(BaseSwordData, this);

	// La copia hereda los flags de asset (Standalone/Public): sin limpiarlos no se liberaría y retendría a la espada.
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

		// Solo se tocan nodos de la copia; uno que viva fuera del Data Asset se deja intacto.
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