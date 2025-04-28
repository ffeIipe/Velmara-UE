#include "HUD/Inventory.h"

void UInventory::OnClicked(int NewWeaponIndex)
{
	OnWeaponButtonClickedEvent.Broadcast(WeaponIndex);
}