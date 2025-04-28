#include "Components/InventoryComponent.h"
#include "HUD/Inventory.h"
#include "Items/Weapons/Sword.h" 
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "GameFramework/Character.h" 
#include "Blueprint/UserWidget.h"
#include "Blueprint/WidgetTree.h"

UInventoryComponent::UInventoryComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
	EquippedItem = nullptr;
	CurrentItemIndex = -1;
}

void UInventoryComponent::BeginPlay()
{
	//Super::BeginPlay();

	PlayerControllerRef = UGameplayStatics::GetPlayerController(GetWorld(), 0);

	// InventoryWidgetClass es TSubclassOf<UInventory>
    if (ensure(InventoryWidgetClass) && PlayerControllerRef && PlayerControllerRef->IsLocalController())
    {
        // Creamos el widget principal. Sabemos que será de tipo UInventory* o derivado.
        InventoryWidget = CreateWidget<UInventory>(PlayerControllerRef, InventoryWidgetClass);

        // Verificamos que se creó y que tiene un WidgetTree para poder buscar dentro
        if (InventoryWidget && InventoryWidget->WidgetTree)
        {
            InventoryWidget->AddToViewport();
            InventoryWidget->SetVisibility(ESlateVisibility::Hidden);

            // --- LÓGICA PARA ENCONTRAR Y BINDEAR SLOTS HIJOS ---

            TArray<UWidget*> FoundWidgets;
            // Obtiene TODOS los widgets hijos dentro del árbol del InventoryWidget principal
            InventoryWidget->WidgetTree->GetAllWidgets(FoundWidgets);

            UE_LOG(LogTemp, Log, TEXT("InventoryComponent: Found %d total widgets in InventoryWidget tree ('%s'). Searching for slots inheriting from UInventory..."),
                   FoundWidgets.Num(), *InventoryWidget->GetName());

            int BoundSlots = 0;
            for (UWidget* Widget : FoundWidgets)
            {
                // Intentar convertir el widget encontrado a UInventory (la clase C++ padre)
                UInventory* PotentialSlotWidget = Cast<UInventory>(Widget);

                // Comprobar si el cast tuvo éxito Y MUY IMPORTANTE:
                // Si NO es el mismo widget principal (InventoryWidget)
                if (PotentialSlotWidget && PotentialSlotWidget != InventoryWidget)
                {
                    // ¡Encontrado! Es un widget hijo que hereda de UInventory (debería ser tu WBP_Inventory_Item)
                    // Conecta nuestra función HandleSlotClicked al delegado OnWeaponButtonClickedEvent de ESTE slot específico.
                    PotentialSlotWidget->OnWeaponButtonClickedEvent.AddDynamic(this, &UInventoryComponent::HandleSlotClicked);
                    BoundSlots++;
                    // Accedemos a WeaponIndex porque UInventory lo tiene definido
                    UE_LOG(LogTemp, Log, TEXT("InventoryComponent:   -> Successfully bound to delegate of SlotWidget: %s (Inherits UInventory, Index: %d)"),
                           *PotentialSlotWidget->GetName(), PotentialSlotWidget->WeaponIndex);
                }
            }

            if(BoundSlots == 0)
            {
                 UE_LOG(LogTemp, Warning, TEXT("InventoryComponent: Could not find or bind to any child slot widgets inheriting from UInventory inside %s! Check WBP_Inventory structure and inheritance."), *InventoryWidget->GetName());
            } else {
                 UE_LOG(LogTemp, Log, TEXT("InventoryComponent: Finished binding. Total slots bound: %d"), BoundSlots);
            }

            // --- FIN LÓGICA MODIFICADA ---
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("InventoryComponent: Failed to create Inventory Widget ('%s') or its WidgetTree is null!"), *GetNameSafe(InventoryWidgetClass.Get()));
        }
    }
    else
    {
        UE_LOG(LogTemp, Warning, TEXT("InventoryComponent: InventoryWidgetClass not set or not a local player controller. Cannot create Inventory UI."));
    }
}

// Mostrar Inventario
void UInventoryComponent::ShowInventory()
{
	if (InventoryWidget)
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Visible);
		bIsInventoryOpen = true;
		UpdateInventoryUI();

		if (PlayerControllerRef)
		{
			FInputModeGameAndUI InputMode;
			InputMode.SetWidgetToFocus(InventoryWidget->TakeWidget());
			InputMode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
			PlayerControllerRef->SetInputMode(InputMode);
			PlayerControllerRef->bShowMouseCursor = true;
		}
		GetWorld()->GetWorldSettings()->SetTimeDilation(0.1f);
	}
}

void UInventoryComponent::HideInventory()
{
	if (InventoryWidget)
	{
		InventoryWidget->SetVisibility(ESlateVisibility::Hidden);
		bIsInventoryOpen = false;

		if (PlayerControllerRef)
		{
			FInputModeGameOnly InputMode;
			PlayerControllerRef->SetInputMode(InputMode);
			PlayerControllerRef->bShowMouseCursor = false;
		}

		GetWorld()->GetWorldSettings()->SetTimeDilation(1.0f);
	}
}

bool UInventoryComponent::AddItem(ASword* ItemToAdd)
{
	if (!ItemToAdd)
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryComponent::AddItem: Attempted to add a null item."));
		return false;
	}

	if (InventoryItems.Num() >= MaxInventorySize)
	{
		UE_LOG(LogTemp, Log, TEXT("InventoryComponent::AddItem: Inventory is full. Cannot add %s."), *ItemToAdd->GetName());
		return false;
	}

	InventoryItems.Add(ItemToAdd);
	UE_LOG(LogTemp, Log, TEXT("InventoryComponent::AddItem: Added %s to inventory. Total items: %d"), *ItemToAdd->GetName(), InventoryItems.Num());

	ItemToAdd->SetActorHiddenInGame(true);
	ItemToAdd->SetActorEnableCollision(false);
	ItemToAdd->SetActorTickEnabled(false);

	if (!EquippedItem)
	{
		SwitchItem(InventoryItems.Num() - 1);
	}

	UpdateInventoryUI();
	return true;
}

void UInventoryComponent::SwitchItem(int ItemIndex)
{
	if (!InventoryItems.IsValidIndex(ItemIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryComponent::SwitchItem: Invalid index %d provided."), ItemIndex);
		return;
	}

	ASword* NewItem = InventoryItems[ItemIndex];
	if (!NewItem)
	{
		UE_LOG(LogTemp, Error, TEXT("InventoryComponent::SwitchItem: Item at index %d is null!"), ItemIndex);
		return;
	}

	if (NewItem == EquippedItem)
	{
		UE_LOG(LogTemp, Log, TEXT("InventoryComponent::SwitchItem: Item at index %d is already equipped."), ItemIndex);
		// Opcional: Desequipar si se selecciona el mismo item?
		// if (EquippedItem) EquippedItem->Enable(false);
		// EquippedItem = nullptr;
		// CurrentItemIndex = -1;
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("InventoryComponent::SwitchItem: Switching to item at index %d: %s"), ItemIndex, *NewItem->GetName());

	if (EquippedItem)
	{
		UE_LOG(LogTemp, Log, TEXT("InventoryComponent::SwitchItem: Unequipping previous item: %s"), *EquippedItem->GetName());
		EquippedItem->Enable(false); // Asume que Enable(false) desequipa
	}

	EquippedItem = NewItem;
	CurrentItemIndex = ItemIndex;
	
	ACharacter* OwnerCharacter = Cast<ACharacter>(GetOwner());

	//EquippedItem->Equip(OwnerCharacter->GetMesh(), FName("RightHandSocket"), OwnerCharacter, OwnerCharacter);
	EquippedItem->Enable(true);

	UpdateInventoryUI();
}

void UInventoryComponent::DropItem(int ItemIndex)
{
	if (!InventoryItems.IsValidIndex(ItemIndex))
	{
		UE_LOG(LogTemp, Warning, TEXT("InventoryComponent::DropItem: Invalid index %d."), ItemIndex);
		return;
	}

	ASword* ItemToDrop = InventoryItems[ItemIndex];
	if (!ItemToDrop)
	{
		UE_LOG(LogTemp, Error, TEXT("InventoryComponent::DropItem: Item at index %d is null!"), ItemIndex);
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("InventoryComponent::DropItem: Dropping item at index %d: %s"), ItemIndex, *ItemToDrop->GetName());

	// 1. Desequipar si es el item actual
	if (ItemToDrop == EquippedItem)
	{
		EquippedItem->Enable(false);
		EquippedItem = nullptr;
		CurrentItemIndex = -1;
		UE_LOG(LogTemp, Log, TEXT("InventoryComponent::DropItem: Unequipped the dropped item."));
		// Opcional: Intentar equipar otro item si quedan?
		// if (InventoryItems.Num() > 1) { SwitchItem(0); } // Equipar el primero si queda más de uno (el que se va a borrar)
	}

	// 2. Eliminar del array
	InventoryItems.RemoveAt(ItemIndex);
	if (CurrentItemIndex > ItemIndex)
	{
		CurrentItemIndex--;
	}
	else if (InventoryItems.Num() == 0)
	{
		CurrentItemIndex = -1;
	}
	else if (ItemToDrop == EquippedItem && InventoryItems.Num() > 0)
	{
		SwitchItem(0);
	}

	AActor* Owner = GetOwner();
	if (Owner)
	{
		FVector DropLocation = Owner->GetActorLocation() + Owner->GetActorForwardVector() * 150.0f + FVector(0, 0, 50.0f); // Delante y un poco arriba
		FRotator DropRotation = Owner->GetActorRotation();
		FActorSpawnParameters SpawnParams;
		SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

		ItemToDrop->SetActorHiddenInGame(false);
		ItemToDrop->SetActorEnableCollision(true);
		ItemToDrop->SetActorTickEnabled(true);
		ItemToDrop->SetActorLocationAndRotation(DropLocation, DropRotation);
		// Opcionalmente, añadirle impulso físico
		// UPrimitiveComponent* RootComp = Cast<UPrimitiveComponent>(ItemToDrop->GetRootComponent());
		// if(RootComp && RootComp->IsSimulatingPhysics())
		// {
		//     RootComp->AddImpulse(Owner->GetActorForwardVector() * 500.0f, NAME_None, true);
		// }


		// Opción B: Spawnear una nueva copia (más seguro si el estado del item cambia mucho)
		/*
		AItem* NewDroppedItem = GetWorld()->SpawnActor<AItem>(ItemToDrop->GetClass(), DropLocation, DropRotation, SpawnParams);
		if (NewDroppedItem)
		{
			UE_LOG(LogTemp, Log, TEXT("InventoryComponent::DropItem: Spawned dropped item %s in world."), *NewDroppedItem->GetName());
			// Copiar estado relevante si es necesario
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("InventoryComponent::DropItem: Failed to spawn dropped item!"));
		}
		// Si spawnaste una copia, necesitas destruir el ItemToDrop original
		// ItemToDrop->Destroy();
		*/
	}

	UpdateInventoryUI();
}

const TArray<ASword*>& UInventoryComponent::GetInventoryItems() const
{
	return InventoryItems;
}

void UInventoryComponent::HandleSlotClicked(int ClickedIndex)
{
	// Log de ALTO NIVEL para verlo fácil en la consola
	UE_LOG(LogTemp, Error, TEXT("============= HANDLE SLOT CLICKED - Received Index: %d ============="), ClickedIndex);
	SwitchItem(ClickedIndex);
	HideInventory();
}