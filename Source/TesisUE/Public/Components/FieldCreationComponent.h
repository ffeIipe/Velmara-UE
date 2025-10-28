// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Field/FieldSystemTypes.h"
#include "FieldCreationComponent.generated.h"


class UFieldSystemComponent;
class URadialVector;
class UFieldSystemMetaDataFilter;
class URadialFalloff;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class TESISUE_API UFieldCreationComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UFieldCreationComponent();

	UFUNCTION(BlueprintCallable)
	void CreateFields(const float FieldMagnitude, float MinRange, float Radius, const FVector& Location, bool bIsTick) const;

protected:
	UPROPERTY()
	TObjectPtr<UFieldSystemComponent> FieldSystemComp;
	UPROPERTY()
	TObjectPtr<URadialFalloff> RadialFalloffComp;
	UPROPERTY()
	TObjectPtr<URadialVector> RadialVectorComp;
	UPROPERTY()
	TObjectPtr<UFieldSystemMetaDataFilter> MetaDataFilterComp;
};
