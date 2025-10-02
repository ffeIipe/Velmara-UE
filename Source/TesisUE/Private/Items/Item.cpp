#include "Items/Item.h"
#include "Components/BoxComponent.h"
#include "Tutorial/PromptWidgetComponent.h"
#include "Interfaces/AttributeProvider.h" //do not delete this
#include "Components/Items/ItemMementoComponent.h"


AItem::AItem()
{
	PrimaryActorTick.bCanEverTick = false;

	ItemMementoComponent = CreateDefaultSubobject<UItemMementoComponent>(TEXT("ItemMemento"));
	
	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Item Mesh"));
	RootComponent = ItemMesh;
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	ItemMesh->SetCollisionResponseToAllChannels(ECR_Ignore);
	ItemMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Block);

	BoxCollider->SetupAttachment(GetRootComponent());
	BoxCollider->SetCollisionResponseToAllChannels(ECR_Ignore);
	BoxCollider->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
}

void AItem::BeginPlay()
{
	Super::BeginPlay();
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
	
	/*
	DisableCollision();*/
}

void AItem::EnableVisuals()
{
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);

	/*if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Enable visuals");*/
}

void AItem::DisableVisuals()
{
	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Disable visuals");
	
	SetActorHiddenInGame(true);
	SetActorEnableCollision(false);
}

UPrimitiveComponent* AItem::GetCollisionComponent()
{
	return nullptr;
}

void AItem::DisableCollision()
{
	Super::DisableCollision();

	ItemMesh->SetCollisionResponseToChannel(ECC_GameTraceChannel5, ECR_Ignore);
	ItemMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	
	if (PromptWidgetComponent)
	{
		PromptWidgetComponent->DisablePromptWidget();
	}
}