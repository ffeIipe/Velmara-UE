// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "SceneEvents/Trigger.h"
#include "Item.generated.h"

class UBoxComponent;
class UPromptWidgetComponent;

enum class EItemState : uint8
{
	EIS_Hovering,
	EIS_Equipped
};

UCLASS()
class TESISUE_API AItem : public ATrigger
{
	GENERATED_BODY()
	
public:	

	AItem();

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties")
	FText ItemDisplayName = FText::FromString("Item");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties", meta = (MultiLine = true))
	FText ItemDescription = FText::FromString("Default item description.");

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Item Properties")
	UTexture2D* ItemIcon = nullptr;

	virtual void BeginPlay();

	virtual void Equip(USceneComponent* InParent, FName InSocketName, AActor* NewOwner, APawn* NewInstigator);
	
	virtual void Unequip();
	
	virtual void Use(class ACharacter* TargetCharacter);
	
	virtual void EnableVisuals(bool bEnable);
	
	virtual UPrimitiveComponent* GetCollisionComponent();

protected:
	UPROPERTY(VisibleAnywhere, Category = "Weapon Properties")
	UStaticMeshComponent* ItemMesh;

	void OnSphereBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex, bool bFromSweep,
		const FHitResult& SweepResult) override;

	void OnSphereEndOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex) override;

	EItemState ItemState = EItemState::EIS_Hovering;
	
	UPROPERTY(VisibleAnywhere)
	UPromptWidgetComponent* PromptWidget;
};