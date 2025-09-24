#include "Items/Item.h"
#include "Player/PlayerMain.h"
#include "Components/BoxComponent.h"
#include "Tutorial/PromptWidgetComponent.h"
#include <SceneEvents/NewGameStateBase.h>


AItem::AItem()
{
	PrimaryActorTick.bCanEverTick = false;

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Item Mesh"));
	RootComponent = ItemMesh;
	ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);

	BoxCollider->SetupAttachment(GetRootComponent());
	BoxCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	BoxCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PromptWidget = CreateDefaultSubobject<UPromptWidgetComponent>(TEXT("PromptTrigger"));
	PromptWidget->SetupAttachment(GetRootComponent());
}

void AItem::BeginPlay()
{
	Super::BeginPlay();

	if (PromptWidget)
	{
		if (PromptWidget->GetWidget())
		{
			PromptWidget->GetWidget()->SetVisibility(ESlateVisibility::Hidden);
		}
	}
	
	if (const UWorld* World = GetWorld())
	{
		if (ANewGameStateBase* GameState = World->GetGameState<ANewGameStateBase>())
		{
			GameState->RequestInteractedItemStateReconciliation(this);
		}
	}
}

void AItem::Pick(AActor* NewOwner)
{
	SetOwner(NewOwner);
	SetInstigator(Cast<APawn>(NewOwner));
	AttributeProvider = NewOwner;
	
	if (!bWasUsed)
	{
		bWasUsed = true;
		if (const UWorld* World = GetWorld())
		{
			if (ANewGameStateBase* GameState = World->GetGameState<ANewGameStateBase>())
			{
				FInteractedItemSaveData SaveData;
				SaveData.UniqueSaveID = GetUniqueSaveID();
				SaveData.bWasOpened = bWasUsed;
				GameState->UpdateInteractedItemState(SaveData);
			}
		}
	}
}

void AItem::EnableVisuals(const bool bEnable)
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
	if (SavedData && SavedData->bWasOpened)
	{
		if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("Item has been used... So restoring..."));
		/*Pick(SavedData.OldOwner);*/
		bWasUsed = true;
		
		Destroy();
	}
}

void AItem::DisableCollision()
{
	Super::DisableCollision();

	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);

	if (PromptWidget)
	{
		PromptWidget->GetWidget()->SetVisibility(ESlateVisibility::Hidden);
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