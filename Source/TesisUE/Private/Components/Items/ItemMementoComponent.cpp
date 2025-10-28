// Fill out your copyright notice in the Description page of Project Settings.


#include "ItemMementoComponent.h"

#include "Items/Item.h"


// Sets default values for this component's properties
UItemMementoComponent::UItemMementoComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	// ...
	
}

FItemMementoState UItemMementoComponent::CaptureItemState()
{
	const AItem* Item = Cast<AItem>(GetOwner());
	
	InternalItemState.UniqueSaveID = Item->GetUniqueSaveID();
	InternalItemState.bWasOpened = Item->bWasUsed;

	return InternalItemState;
}

void UItemMementoComponent::ApplyExternalState(const FItemMementoState ItemStateToApply)
{
	AItem* Item = Cast<AItem>(GetOwner());

	Item->bWasUsed = ItemStateToApply.bWasOpened;

	if (Item->bWasUsed)
	{
		Item->Destroy();
	}
}

void UItemMementoComponent::BeginPlay()
{
	Super::BeginPlay();
}
