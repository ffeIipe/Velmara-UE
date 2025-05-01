#include "Items/Item.h"
#include "Player/PlayerMain.h"
#include "Components/BoxComponent.h"
#include "Tutorial/PromptWidgetComponent.h"


AItem::AItem()
{
	PrimaryActorTick.bCanEverTick = false;

	ItemMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Item Mesh"));
	RootComponent = ItemMesh;

	BoxCollider->SetupAttachment(GetRootComponent());

	PromptWidget = CreateDefaultSubobject<UPromptWidgetComponent>(TEXT("PromptTrigger"));
	PromptWidget->SetupAttachment(GetRootComponent());
}

void AItem::BeginPlay()
{
	Super::BeginPlay();
	if (PromptWidget)
 PromptWidget->GetWidget()->SetVisibility(ESlateVisibility::Hidden);
}

void AItem::Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator)
{
	PromptWidget->GetWidget()->SetVisibility(ESlateVisibility::Hidden);
}

void AItem::Unequip()
{
	//PromptWidget2->GetWidget()->SetVisibility(ESlateVisibility::Visible);
}

void AItem::Use(ACharacter* TargetCharacter)
{
}

void AItem::EnableVisuals(bool bEnable)
{
	SetActorHiddenInGame(!bEnable);
	SetActorEnableCollision(bEnable);
	PromptWidget->GetWidget()->SetVisibility(ESlateVisibility::Hidden);
}

UPrimitiveComponent* AItem::GetCollisionComponent()
{
	return nullptr;
}

void AItem::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	if (Player)
	{
		PromptWidget->GetWidget()->SetVisibility(ESlateVisibility::Visible);
		Player->SetOverlappingItem(this);
	}
}

void AItem::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnSphereEndOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);

	if (Player)
	{
		PromptWidget->GetWidget()->SetVisibility(ESlateVisibility::Hidden);
		Player->SetOverlappingItem(nullptr);
	}
}
