// Fill out your copyright notice in the Description page of Project Settings.


#include "Enemy/Paladin.h"


void APaladin::GetHit_Implementation(const FVector& ImpactPoint)
{
	Super::GetHit_Implementation(ImpactPoint);

}

float APaladin::TakeDamage(float DamageAmount, FDamageEvent const& DamageEvent, AController* EventInstigator, AActor* DamageCauser)
{
	Super::TakeDamage(DamageAmount, DamageEvent, EventInstigator, DamageCauser);


	return DamageAmount;
}

void APaladin::Disarm()
{
	//TODO: logic to get off his shield if it has attached to it
}
