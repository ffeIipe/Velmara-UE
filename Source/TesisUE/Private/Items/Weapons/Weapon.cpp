#include "Items/Weapons/Weapon.h"

#include "AbilitySystemInterface.h"
#include "AbilitySystemComponent.h"

#include "GameFramework/Character.h"
#include "GAS/VelmaraGameplayTags.h"

void AWeapon::SetDamageType_Implementation(const TSubclassOf<UBaseDamageType> DamageType)
{
	IWeaponInterface::SetDamageType_Implementation(DamageType);

	if (DamageType == nullptr)
	{
		DamageTypeClass = UBaseDamageType::StaticClass();
	}
	
	DamageTypeClass = DamageType;
}

void AWeapon::OnHit(AActor* Actor, const float Damage)
{
	const IAbilitySystemInterface* TargetASI = Cast<IAbilitySystemInterface>(Actor);
	if (!TargetASI) return;

	UAbilitySystemComponent* TargetASC = TargetASI->GetAbilitySystemComponent();
	if (!TargetASC) return;

	if (const IAbilitySystemInterface* SourceASI = Cast<IAbilitySystemInterface>(GetOwner()))
	{
		const UAbilitySystemComponent* SourceASC = SourceASI->GetAbilitySystemComponent();
	}

	FGameplayEffectContextHandle Context = TargetASC->MakeEffectContext();
	Context.AddSourceObject(this);
	Context.AddInstigator(GetInstigator(), this);

	FGameplayEffectSpecHandle SpecHandle = TargetASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);
	if (SpecHandle.IsValid())
	{
		const float FinalDamage =- Damage;

		const FGameplayTag DamageTag = FVelmaraGameplayTags::Get().Damage;
		SpecHandle.Data.Get()->SetSetByCallerMagnitude(DamageTag, FinalDamage);
		TargetASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
	}
}

void AWeapon::Pick(AActor* NewOwner)
{
	USkeletalMeshComponent* Mesh = Cast<ACharacter>(NewOwner)->GetMesh();
	APawn* NewInstigator = Cast<APawn>(NewOwner);

	Super::Pick(NewOwner);

	if (Mesh)
	{
		AttachMeshToSocket(Mesh);
	}
	
	if (NewInstigator)
	{
		SetInstigator(NewInstigator);
	}
	
	ItemState = EItemState::EIS_Equipped;
	DisableCollision();
}
