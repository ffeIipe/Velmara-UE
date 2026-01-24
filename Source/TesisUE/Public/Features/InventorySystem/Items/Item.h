#pragma once

#include "CoreMinimal.h"
#include "Features/InventorySystem/Interfaces/Pickable.h"
#include "Features/SaveSystem/Interfaces/SaveInterface.h"
#include "Tutorial/InputPromptTrigger.h"

#include "Item.generated.h"

class UBoxComponent;
class UPromptWidgetComponent;

USTRUCT(BlueprintType)
struct FUIItemProperties
{
	GENERATED_BODY()
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties")
	FText ItemDisplayName;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (MultiLine = true))
	FText ItemDescription;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties")
	UTexture2D* ItemIcon;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnItemUsed, ACharacter*, UserCharacter);

UCLASS()
class TESISUE_API AItem : public AActor, public IPickable, public ISaveInterface
{
	GENERATED_BODY()

public:	
	AItem();
	
	UPROPERTY(BlueprintAssignable)
	FOnItemUsed OnUsed;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, SaveGame)
	bool bWasUsed = false;

	virtual void BeginPlay() override;
	
	virtual void OnEnteredInventory_Implementation(AActor* NewOwner) override;

	virtual void OnRemovedFromInventory_Implementation() override;

	virtual void EnableVisuals();

	virtual void DisableVisuals();

	virtual bool ShouldConsumeOnUse() const override { return false; }
	
	virtual void OnSaveGame_Implementation(FEntitySaveData& OutData) override;
	
	virtual void OnLoadGame_Implementation(const FEntitySaveData& InData) override;

	virtual FName GetUniqueSaveID_Implementation() override { return UniqueSaveID; };
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FName UniqueSaveID;
	
protected:
	virtual void DisableCollision();

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FUIItemProperties UIItemProperties;

	UPROPERTY(VisibleAnywhere, BlueprintReadWrite, Category = "Item Properties")
	UStaticMeshComponent* ItemMesh;
	
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly)
	USphereComponent* SphereCollider;
};