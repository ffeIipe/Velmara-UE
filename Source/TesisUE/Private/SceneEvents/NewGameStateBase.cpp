// Fill out your copyright notice in the Description page of Project Settings.


#include "SceneEvents/NewGameStateBase.h"
#include "Components/MementoComponent.h"

void ANewGameStateBase::RegisterMementoEntity(AActor* Entity)
{
	if (Entity && Entity->GetComponentByClass<UMementoComponent>() && !MementoEntities.Contains(Entity))
	{
		MementoEntities.Add(Entity);
	}
}

TArray<AActor*> ANewGameStateBase::GetMementoEntities() const
{
	return MementoEntities;
}

void ANewGameStateBase::SaveAllMementoStates()
{
	for (AActor* Entity : MementoEntities)
	{
		if (UMementoComponent* MementoComp = Entity->GetComponentByClass<UMementoComponent>()) //fatal error
		{
			MementoComp->SaveState();
		}
	}
}

void ANewGameStateBase::LoadAllMementoStates()
{
	for (AActor* Entity : MementoEntities)
	{
		if (UMementoComponent* MementoComp = Entity->GetComponentByClass<UMementoComponent>())
		{
			MementoComp->LoadState();
		}
	}
}
