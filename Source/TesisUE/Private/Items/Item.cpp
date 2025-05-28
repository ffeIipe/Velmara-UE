#include "Items/Item.h"
#include "Player/PlayerMain.h"
#include "Components/BoxComponent.h"
#include "Tutorial/PromptWidgetComponent.h"
#include <SceneEvents/NewGameStateBase.h>


AItem::AItem()
{
	PrimaryActorTick.bCanEverTick = false;

	if (UniqueSaveID == NAME_None)
	{
		UniqueSaveID = FName(*FGuid::NewGuid().ToString());
	}

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Item Mesh"));
	RootComponent = ItemMesh;

	BoxCollider->SetupAttachment(GetRootComponent());
	BoxCollider->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	PromptWidget = CreateDefaultSubobject<UPromptWidgetComponent>(TEXT("PromptTrigger"));
	PromptWidget->SetupAttachment(GetRootComponent());
}

void AItem::BeginPlay()
{
	Super::BeginPlay();

	UE_LOG(LogTemp, Log, TEXT("Item '%s' (ID: %s): BeginPlay called."), *GetName(), *GetUniqueSaveID().ToString());

	if (PromptWidget)
		PromptWidget->GetWidget()->SetVisibility(ESlateVisibility::Hidden);

	if (UWorld* World = GetWorld())
	{
		if (ANewGameStateBase* GameState = World->GetGameState<ANewGameStateBase>())
		{
			UE_LOG(LogTemp, Log, TEXT("Item '%s' (ID: %s): Requesting State Reconciliation from GameState."), *GetName(), *GetUniqueSaveID().ToString());
			GameState->RequestInteractedItemStateReconciliation(this);
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("Item '%s' (ID: %s): Could not get ANewGameStateBase!"), *GetName(), *GetUniqueSaveID().ToString());
		}
	}
}


void AItem::Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator)
{
	UE_LOG(LogTemp, Log, TEXT("Item '%s' (ID: %s): Equip called. Current bWasOpened state is: %s"), *GetName(), *GetUniqueSaveID().ToString(), bWasOpened ? TEXT("true") : TEXT("false"));

	DisableCollision();
	PromptWidget->GetWidget()->SetVisibility(ESlateVisibility::Hidden);
	if (!bWasOpened)
	{
		UE_LOG(LogTemp, Warning, TEXT("Item '%s' (ID: %s): Condition '!bWasOpened' is TRUE. Updating state to opened."), *GetName(), *GetUniqueSaveID().ToString());
		bWasOpened = true;
		if (UWorld* World = GetWorld())
		{
			if (ANewGameStateBase* GameState = World->GetGameState<ANewGameStateBase>())
			{
				FInteractedItemSaveData SaveData;
				SaveData.UniqueSaveID = GetUniqueSaveID();
				SaveData.bWasOpened = bWasOpened;
				UE_LOG(LogTemp, Warning, TEXT("Item '%s' (ID: %s): Calling GameState->UpdateInteractedItemState."), *GetName(), *GetUniqueSaveID().ToString());
				GameState->UpdateInteractedItemState(SaveData);
			}
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Item '%s' (ID: %s): Condition '!bWasOpened' is FALSE. No state update needed."), *GetName(), *GetUniqueSaveID().ToString());
	}
}


void AItem::Unequip()
{
	//PromptWidget2->GetWidget()->SetVisibility(ESlateVisibility::Visible);
}

void AItem::Use(ACharacter* TargetCharacter)
{
	if (!bWasOpened)
	{
		bWasOpened = true;
		if (UWorld* World = GetWorld())
		{
			if (ANewGameStateBase* GameState = World->GetGameState<ANewGameStateBase>())
			{
				FInteractedItemSaveData SaveData;
				SaveData.UniqueSaveID = GetUniqueSaveID();
				SaveData.bWasOpened = bWasOpened;
				GameState->UpdateInteractedItemState(SaveData);
			}
		}
		// NO llamamos a Destroy() aquí, dejamos que la clase hija decida cuándo hacerlo.
	}
}

void AItem::EnableVisuals(bool bEnable)
{
	SetActorHiddenInGame(!bEnable);
	SetActorEnableCollision(bEnable);
	PromptWidget->EnablePromptWidget(false);
}

UPrimitiveComponent* AItem::GetCollisionComponent()
{
	return nullptr;
}

void AItem::ApplySavedState(const FInteractedItemSaveData* SavedData)
{
	UE_LOG(LogTemp, Log, TEXT("Item '%s' (ID: %s): ApplySavedState called."), *GetName(), *GetUniqueSaveID().ToString());
	if (SavedData && SavedData->bWasOpened)
	{
		UE_LOG(LogTemp, Warning, TEXT("Item '%s' (ID: %s): SavedData indicates bWasOpened is TRUE. Destroying actor."), *GetName(), *GetUniqueSaveID().ToString());
		bWasOpened = true;
		Destroy();
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("Item '%s' (ID: %s): SavedData is null or bWasOpened is FALSE. Actor will NOT be destroyed."), *GetName(), *GetUniqueSaveID().ToString());
	}
}
void AItem::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (Player)
	{
		PromptWidget->EnablePromptWidget(true);
	}
}

void AItem::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
    Super::OnSphereEndOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);

    APlayerMain* CurrentOverlappingPlayer = Cast<APlayerMain>(OtherActor);

    if (IsValid(Player) && Player == CurrentOverlappingPlayer)
    {
        if (PromptWidget && PromptWidget->GetWidget())
        {
			PromptWidget->EnablePromptWidget(false);
        }
    }
    else if (CurrentOverlappingPlayer)
    {
		if (PromptWidget && PromptWidget->GetWidget())
		{
			PromptWidget->EnablePromptWidget(false);
		}
    }
}