#include "Features/WeaponUpgradeSystem/Core/BloodAltar.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Features/WeaponUpgradeSystem/Core/WeaponUpgradeComponent.h"

#define LOCTEXT_NAMESPACE "WeaponUpgrade"

ABloodAltar::ABloodAltar()
{
	PrimaryActorTick.bCanEverTick = false;

	AltarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AltarMesh"));
	AltarMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	// Canal "Item": es el que traza GA_Interact (igual que AItem).
	AltarMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Block);
	SetRootComponent(AltarMesh);

	InteractionText = LOCTEXT("AltarInteraction", "Ofrendar Sangre");
}

bool ABloodAltar::ExecuteInteraction_Implementation(AActor* Interactor)
{
	UWeaponUpgradeComponent* WeaponUpgrades = Interactor ? Interactor->FindComponentByClass<UWeaponUpgradeComponent>() : nullptr;
	if (!WeaponUpgrades)
	{
		UE_LOG(LogTemp, Warning, TEXT("BloodAltar: %s no tiene UWeaponUpgradeComponent."), *GetNameSafe(Interactor));
		return false;
	}

	WeaponUpgrades->ToggleMenu();
	OnAltarUsed(Interactor, WeaponUpgrades->IsMenuOpen());

	return true;
}

#undef LOCTEXT_NAMESPACE
