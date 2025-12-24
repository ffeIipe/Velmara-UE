#include "Items/Item.h"
#include "Components/BoxComponent.h"
#include "Tutorial/PromptWidgetComponent.h"
#include "Serialization/ObjectAndNameAsStringProxyArchive.h"


AItem::AItem()
{
	PrimaryActorTick.bCanEverTick = false;
	
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
	bWasUsed = true;
}

void AItem::EnableVisuals()
{
	SetActorHiddenInGame(false);
	SetActorEnableCollision(true);
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

void AItem::OnSaveGame_Implementation(FEntitySaveData& OutData)
{
	FMemoryWriter MemWriter(OutData.ByteData);
	FObjectAndNameAsStringProxyArchive Ar(MemWriter, true);
	Ar.ArIsSaveGame = true;
	this->Serialize(Ar);
}

void AItem::OnLoadGame_Implementation(const FEntitySaveData& InData)
{
	FMemoryReader MemReader(InData.ByteData);
	FObjectAndNameAsStringProxyArchive Ar(MemReader, true);
	Ar.ArIsSaveGame = true;
	this->Serialize(Ar);

	if (bWasUsed)
	{
		Destroy();
	}
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
