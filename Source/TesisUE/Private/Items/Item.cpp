#include "Items/Item.h"
#include "Player/PlayerMain.h"
#include "Components/BoxComponent.h"
#include "Tutorial/PromptWidgetComponent.h"
#include <SceneEvents/NewGameStateBase.h>
#include <Kismet/GameplayStatics.h>


AItem::AItem()
{
	PrimaryActorTick.bCanEverTick = false;

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Item Mesh"));
	RootComponent = ItemMesh;

	BoxCollider->SetupAttachment(GetRootComponent());
	BoxCollider->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);

	PromptWidget = CreateDefaultSubobject<UPromptWidgetComponent>(TEXT("PromptTrigger"));
	PromptWidget->SetupAttachment(GetRootComponent());

	//if (UniqueSaveID == NAME_None)
	//{
	//	UniqueSaveID = FName(*FGuid::NewGuid().ToString());
	//}
}

void AItem::BeginPlay()
{
	Super::BeginPlay();

	if (PromptWidget && PromptWidget->GetWidget())
	{
		PromptWidget->GetWidget()->SetVisibility(ESlateVisibility::Hidden);
	}
	
	if (UWorld* World = GetWorld())
	{
		if (ANewGameStateBase* GameState = World->GetGameState<ANewGameStateBase>())
		{
			GameState->RequestInteractedItemStateReconciliation(this);
		}
	}
}

void AItem::Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator)
{
	DisableCollision();
	PromptWidget->GetWidget()->SetVisibility(ESlateVisibility::Hidden);
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
	if (SavedData && SavedData->bWasOpened)
	{
		Use(UGameplayStatics::GetPlayerCharacter(GetWorld(), 0));
		bWasOpened = true;
		Destroy();
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