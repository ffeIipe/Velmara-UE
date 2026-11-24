#include "Features/WeaponUpgradeSystem/Core/BloodAltar.h"

#include "Components/StaticMeshComponent.h"
#include "Engine/CollisionProfile.h"
#include "Features/AttributeTreeSystem/Core/AttributeTreeComponent.h"
#include "Features/WeaponUpgradeSystem/Core/WeaponUpgradeComponent.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "UI/Widgets/Menus/VelmaraBloodAltarMenuWidget.h"

#define LOCTEXT_NAMESPACE "WeaponUpgrade"

ABloodAltar::ABloodAltar()
{
	PrimaryActorTick.bCanEverTick = false;

	AltarMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("AltarMesh"));
	AltarMesh->SetCollisionProfileName(UCollisionProfile::BlockAll_ProfileName);
	AltarMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Block);
	SetRootComponent(AltarMesh);

	InteractionText = LOCTEXT("AltarInteraction", "Ofrendar Sangre");
}

bool ABloodAltar::ExecuteInteraction_Implementation(AActor* Interactor)
{
	UWeaponUpgradeComponent* WeaponUpgrades = Interactor ? Interactor->FindComponentByClass<UWeaponUpgradeComponent>() : nullptr;
	UAttributeTreeComponent* AttributeTree = Interactor ? Interactor->FindComponentByClass<UAttributeTreeComponent>() : nullptr;

	if (!WeaponUpgrades && !AttributeTree)
	{
		UE_LOG(LogTemp, Warning, TEXT("BloodAltar: %s no tiene componentes de mejora."), *GetNameSafe(Interactor));
		return false;
	}

	if (const APawn* InteractorPawn = Cast<APawn>(Interactor))
	{
		if (APlayerController* PC = InteractorPawn->GetController<APlayerController>())
		{
			if (UVelmaraBloodAltarMenuWidget::GetActiveAltar(PC))
			{
				UVelmaraBloodAltarMenuWidget::CloseActiveAltar(PC);
				OnAltarUsed(Interactor, false);
				return true;
			}

			const EVelmaraBloodAltarTab InitialTab = AttributeTree
				? EVelmaraBloodAltarTab::Attributes
				: EVelmaraBloodAltarTab::WeaponUpgrades;

			UVelmaraBloodAltarMenuWidget* Opened = UVelmaraBloodAltarMenuWidget::OpenBloodAltar(PC, Interactor, InitialTab);
			if (Opened)
			{
				OnAltarUsed(Interactor, true);
				return true;
			}
		}
	}

	if (WeaponUpgrades)
	{
		WeaponUpgrades->ToggleMenu();
		OnAltarUsed(Interactor, WeaponUpgrades->IsMenuOpen());
		return true;
	}

	AttributeTree->ToggleMenu();
	OnAltarUsed(Interactor, AttributeTree->IsMenuOpen());
	return true;
}

#undef LOCTEXT_NAMESPACE
