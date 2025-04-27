#include "Components/InventoryComponent.h"
#include "HUD/Inventory.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Items/Weapons/Sword.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UInventoryComponent::ShowInventory()
{
	if (InventoryWidget)
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Visible);
		bIsInventoryOpen = true;
	}
	
	PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0); //crash
	if (PlayerController && GEngine && GEngine->GameViewport)
	{
		FVector2D ViewportSize;
		
		GEngine->GameViewport->GetViewportSize(ViewportSize);

		float CenterX = ViewportSize.X / 2.0f;
		float CenterY = ViewportSize.Y / 2.0f;

		PlayerController->bShowMouseCursor = true;
		PlayerController->SetMouseLocation(CenterX, CenterY);
	}

	GetWorld()->GetWorldSettings()->SetTimeDilation(.2f);
}

void UInventoryComponent::HideInventory()
{
	if (InventoryWidget)
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Hidden);
		bIsInventoryOpen = false;
	}

	PlayerController = UGameplayStatics::GetPlayerController(GetWorld(), 0);
	if (PlayerController)
	{
		PlayerController->bShowMouseCursor = false;
	}
	
	GetWorld()->GetWorldSettings()->SetTimeDilation(1.f);
}

void UInventoryComponent::SwitchWeapon(int NewWeaponIndex)
{
	if(InventoryWeapons.IsValidIndex(NewWeaponIndex))
	CurrentIndex = NewWeaponIndex;
	EquippedWeapon = InventoryWeapons[NewWeaponIndex];
	EquippedWeapon->Enable(true);
}

void UInventoryComponent::BeginPlay()
{
	Super::BeginPlay();

	if (InventoryWidgetClass)
	{
		InventoryWidget = CreateWidget<UInventory>(GetWorld(), InventoryWidgetClass);
		if (InventoryWidget)
		{
			InventoryWidget->AddToViewport();
			InventoryWidget->SetVisibility(ESlateVisibility::Hidden);
			InventoryWidget->OnWeaponButtonClickedEvent.AddDynamic(this, &UInventoryComponent::SwitchWeapon);
		}
	}
}