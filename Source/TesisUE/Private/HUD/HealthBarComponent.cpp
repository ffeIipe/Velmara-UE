// Fill out your copyright notice in the Description page of Project Settings.


#include "HUD/HealthBarComponent.h"
#include "HUD/HealthBar.h"
#include "Components/ProgressBar.h"

void UHealthBarComponent::BeginPlay()
{
	Super::BeginPlay();

	PlayerMain = GetWorld()->GetFirstPlayerController()->GetPawn();
}

void UHealthBarComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	SetHealthOpacity(DeltaTime);
}

void UHealthBarComponent::SetHealthPercent(float Percent)
{
	if (HealthBarWidget == nullptr)
	{
		HealthBarWidget = Cast<UHealthBar>(GetUserWidgetObject());
	}

	if (HealthBarWidget && HealthBarWidget->HealthBar)
	{
		HealthBarWidget->HealthBar->SetPercent(Percent);
	}
}

void UHealthBarComponent::SetHealthBarActive(bool Param)
{
	if (Param)
	{
		HealthBarWidget->HealthBar->Visibility = ESlateVisibility::Visible;
	}
	else HealthBarWidget->HealthBar->Visibility = ESlateVisibility::Hidden;
}

void UHealthBarComponent::SetHealthOpacity(float DeltaTime)
{
	if (PlayerMain && HealthBarWidget && HealthBarWidget->HealthBar)
	{
		float Distance = FVector::Dist(PlayerMain->GetActorLocation(), GetOwner()->GetActorLocation());

		TargetOpacity = FMath::Clamp(1.f - ((Distance - MinDistance) / (MaxDistance - MinDistance)), 0.f, 1.f);

		CurrentOpacity = FMath::Lerp(CurrentOpacity, TargetOpacity, DeltaTime * 5.f);

		HealthBarWidget->HealthBar->SetRenderOpacity(CurrentOpacity);
	}
}