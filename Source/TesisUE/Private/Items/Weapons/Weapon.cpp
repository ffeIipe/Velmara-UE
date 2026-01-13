#include "Items/Weapons/Weapon.h"
#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"

void AWeapon::SetDamageType_Implementation(const FGameplayTag& DamageTag, const FGameplayTag& CueTag)
{
	if (DamageTag == FGameplayTag::EmptyTag)
	{
		CurrentDamageTag = FGameplayTag::RequestGameplayTag("Damage.Default");
		CurrentCueTag = FGameplayTag::RequestGameplayTag("GameplayCue.Damage.Default");
	}
	else
	{
		CurrentCueTag = CueTag;
		CurrentDamageTag = DamageTag; 	
	}
}

void AWeapon::SetDamageEffectSpec(const FGameplayEffectSpecHandle& InSpecHandle)
{
	DamageEffectSpecHandle = InSpecHandle;
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
