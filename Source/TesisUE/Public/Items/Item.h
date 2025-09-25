// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SceneEvents/Trigger.h"
#include "Interfaces/Pickable.h"
#include "Item.generated.h"

class UItemMementoComponent;
class IAttributeProvider;
class UBoxComponent;
class UPromptWidgetComponent;

enum class EItemState : uint8
{
	EIS_Hovering,
	EIS_Equipped
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemUsed, ACharacter*, UserCharacter);

UCLASS()
class TESISUE_API AItem : public ATrigger, public IPickable
{
	GENERATED_BODY()

public:	
	AItem();

	UPROPERTY(BlueprintAssignable)
	FOnItemUsed OnUsed;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	bool bWasUsed = false;

	virtual void BeginPlay() override;
	
	virtual void Pick(AActor* NewOwner) override;
	
	virtual void EnableVisuals(bool bEnable);
	
	virtual UPrimitiveComponent* GetCollisionComponent();

	/*virtual void ApplySavedState(const struct FInteractedItemSaveData* SavedData);
	*/

	FName GetUniqueSaveID() const { return UniqueSaveID; }
	
	UItemMementoComponent* GetItemMementoComponent() const { return ItemMementoComponent; }

protected:
	virtual void DisableCollision() override;
	
	UPROPERTY(EditInstanceOnly, BlueprintReadWrite, Category = "Save System")
	FName UniqueSaveID;

	struct FItemStats
	{
		FName Name;
		FImage Image;
	};

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties")
	FText ItemDisplayName = FText::FromString("Item");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (MultiLine = true))
	FText ItemDescription = FText::FromString("Default item description.");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties")
	UTexture2D* ItemIcon = nullptr;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Weapon Properties")
	UStaticMeshComponent* ItemMesh;
	
	EItemState ItemState = EItemState::EIS_Hovering;
	
	UPROPERTY(VisibleAnywhere)
	UPromptWidgetComponent* PromptWidget;

	// UPROPERTY(BlueprintReadWrite)
	// ACharacter* User;

	virtual void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult) override;

	virtual void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex) override;

	TScriptInterface<IAttributeProvider> AttributeProvider;

	UPROPERTY()
	UItemMementoComponent* ItemMementoComponent;
};