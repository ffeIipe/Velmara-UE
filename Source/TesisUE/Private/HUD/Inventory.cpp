#include "HUD/Inventory.h"

void UInventory::OnClicked(int NewWeaponIndex)
{
	WeaponIndex = NewWeaponIndex;
	OnWeaponButtonClickedEvent.Broadcast(WeaponIndex);
}