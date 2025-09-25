#include "Items/Item.h"
#include "Components/BoxComponent.h"
#include "Player/PlayerMain.h"
#include "Tutorial/PromptWidgetComponent.h"

#include "Components/Items/ItemMementoComponent.h"


AItem::AItem()
{
	PrimaryActorTick.bCanEverTick = false;

	ItemMementoComponent = CreateDefaultSubobject<UItemMementoComponent>(TEXT("ItemMemento"));
	
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Item Mesh"));
	RootComponent = ItemMesh;
	ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);

	BoxCollider->SetupAttachment(GetRootComponent());
	BoxCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	BoxCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	PromptWidget = CreateDefaultSubobject<UPromptWidgetComponent>(TEXT("PromptTrigger"));
	PromptWidget->SetupAttachment(GetRootComponent());

	/*if (UniqueSaveID == NAME_None)
	{
		const FString GuidString = FGuid::NewGuid().ToString();
		UniqueSaveID = FName(*GuidString);
	}*/
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
}

void AItem::Pick(AActor* NewOwner)
{
	SetOwner(NewOwner);
	SetInstigator(Cast<APawn>(NewOwner));
	AttributeProvider = NewOwner;
	
	if (!bWasUsed)
	{
		bWasUsed = true;
		ItemMementoComponent->CaptureItemState();
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