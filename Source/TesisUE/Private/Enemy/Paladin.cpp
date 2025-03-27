// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/Paladin.h"

APaladin::APaladin()
{
	SwordMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SwordMesh"));
	SwordMesh->SetupAttachment(GetMesh(), TEXT("RightHandSocket"));
}
