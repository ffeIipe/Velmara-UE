// Fill out your copyright notice in the Description page of Project Settings.


#include "Player/PlayerHeroController.h"

APlayerHeroController::APlayerHeroController()
{
	HeroTeamID = FGenericTeamId(0);
}

FGenericTeamId APlayerHeroController::GetGenericTeamId() const
{
	return HeroTeamID;
}