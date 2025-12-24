#pragma once

#include "CoreMinimal.h"
#include "Interfaces/Pickable.h"
#include "Interfaces/SaveInterface.h"
#include "Tutorial/InputPromptTrigger.h"
#include "Item.generated.h"

class UBoxComponent;
class UPromptWidgetComponent;

enum class EItemState : uint8
{
	EIS_Hovering,
	EIS_Equipped
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemUsed, ACharacter*, UserCharacter);

UCLASS()
class TESISUE_API AItem : public AInputPromptTrigger, public IPickable, public ISaveInterface
{
	GENERATED_BODY()

public:	
	AItem();
	
	UPROPERTY(BlueprintAssignable)
	FOnItemUsed OnUsed;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	bool bWasUsed = false;

	virtual void BeginPlay() override;
	
	virtual void Pick(AActor* NewOwner) override;

	UFUNCTION()
	virtual void EnableVisuals();

	UFUNCTION()
	virtual void DisableVisuals();
	
	virtual UPrimitiveComponent* GetCollisionComponent();

	virtual void OnSaveGame_Implementation(FEntitySaveData& OutData) override;
	
	virtual void OnLoadGame_Implementation(const FEntitySaveData& InData) override;

	virtual FName GetUniqueSaveID_Implementation() override { return UniqueSaveID; };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName UniqueSaveID;
	
protected:
	virtual void DisableCollision() override;
	
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

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Item Properties")
	UStaticMeshComponent* ItemMesh;
	
	EItemState ItemState = EItemState::EIS_Hovering;
	
	/*UPROPERTY(VisibleAnywhere)
	UPromptWidgetComponent* PromptWidget;*/
};