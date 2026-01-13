#include "Items/Weapons/Sword.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemInterface.h"
#include "Components/BoxComponent.h"
#include "Kismet/KismetSystemLibrary.h"

#include "DataAssets/Items/Weapons/SwordData.h"
#include "GAS/VelmaraAbilityInputID.h"
#include "GAS/VelmaraGameplayAbility.h"

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

	if (!SwordData)
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
		
	if (SwordData && SwordData->WeaponTag.IsValid())
	{
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
	ItemMesh->AttachToComponent(InParent, TransformRules, SwordData->CustomInSocketName);
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

		IgnoreActors.Add(Hit.GetActor());
		
		if (const TScriptInterface<IAbilitySystemInterface> TargetASI = Hit.GetActor())
		{
			UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
			
			if (TargetASC && DamageEffectSpecHandle.IsValid())
			{
				//Set the damage type
				DamageEffectSpecHandle.Data.Get()->AddDynamicAssetTag(CurrentDamageTag);

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

				TargetASC->ExecuteGameplayCue(CurrentCueTag, CueParameters);
			}
		}
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