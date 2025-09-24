// Fill out your copyright notice in the Description page of Project Settings.


#include "Components/FieldCreationComponent.h"

#include "Field/FieldSystemComponent.h"
#include "Field/FieldSystemObjects.h"
#include "Field/FieldSystemTypes.h"


UFieldCreationComponent::UFieldCreationComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

	FieldSystemComp = CreateDefaultSubobject<UFieldSystemComponent>(TEXT("Field System"));
	MetaDataFilterComp = CreateDefaultSubobject<UFieldSystemMetaDataFilter>(TEXT("Field Meta Data Filter"));
	RadialFalloffComp = CreateDefaultSubobject<URadialFalloff>(TEXT("Radial Falloff"));
	RadialVectorComp = CreateDefaultSubobject<URadialVector>(TEXT("Radial Vector"));

	MetaDataFilterComp->ObjectType = Field_Object_Destruction;
	MetaDataFilterComp->FilterType = Field_Filter_Dynamic;
}

void UFieldCreationComponent::CreateFields(const float FieldMagnitude, const float MinRange, const float Radius, const FVector& Location, const bool bIsTick) const
{
	const TObjectPtr<UFieldNodeBase> RadialFalloff = RadialFalloffComp->SetRadialFalloff(FieldMagnitude, MinRange, 1.f, .0f, Radius, Location, Field_FallOff_None);
	const TObjectPtr<UFieldNodeBase> RadialVector = RadialVectorComp->SetRadialVector(FieldMagnitude, Location);
	
	FieldSystemComp->ApplyPhysicsField(true, Field_ExternalClusterStrain, nullptr, RadialFalloff);
	FieldSystemComp->ApplyPhysicsField(true, Field_LinearForce, MetaDataFilterComp, RadialVector);
}

