// Fill out your copyright notice in the Description page of Project Settings.


#include "SpectralMode/SpectralObjectInteractable.h"
#include "SpectralMode/SpectralReceiver.h"
#include "Components/SphereComponent.h"
#include "Player/PlayerMain.h"
#include "Blueprint/UserWidget.h"
#include "Components/TextBlock.h"


void ASpectralObjectInteractable::BeginPlay()
{
	Super::BeginPlay();

	Super::BeginPlay();

	if (InteractionSphere)
	{
		InteractionSphere->OnComponentBeginOverlap.AddDynamic(this, &ASpectralObjectInteractable::OnSphereBeginOverlap);
		InteractionSphere->OnComponentEndOverlap.AddDynamic(this, &ASpectralObjectInteractable::OnSphereEndOverlap);
	}
}

void ASpectralObjectInteractable::SetSpectralVisibility(bool bIsVisible)
{
	Super::SetSpectralVisibility(bIsVisible);
}

void ASpectralObjectInteractable::OnSphereBeginOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{	
	Super::OnSphereBeginOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex, bFromSweep, SweepResult);

	APlayerMain* Player = Cast<APlayerMain>(OtherActor);
	if (Player && InteractionWidgetClass)
	{

		if (!InteractionWidget)
		{
			InteractionWidget = CreateWidget<UUserWidget>(GetWorld(), InteractionWidgetClass);
			if (InteractionWidget)
			{
				InteractionWidget->AddToViewport();


				UTextBlock* InteractText = Cast<UTextBlock>(InteractionWidget->GetWidgetFromName(TEXT("InteractText")));
				if (InteractText)
				{
					InteractText->SetVisibility(ESlateVisibility::Visible);
				}
			}
		}
	}
}

void ASpectralObjectInteractable::OnSphereEndOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex)
{
	Super::OnSphereEndOverlap(OverlappedComponent, OtherActor, OtherComp, OtherBodyIndex);

	APlayerMain* Player = Cast<APlayerMain>(OtherActor);
	if (Player && InteractionWidget)
	{
		UTextBlock* InteractText = Cast<UTextBlock>(InteractionWidget->GetWidgetFromName(TEXT("InteractText")));
		if (InteractText)
		{
			InteractText->SetVisibility(ESlateVisibility::Collapsed);
		}

		InteractionWidget->RemoveFromParent();
		InteractionWidget = nullptr;
	}
}

void ASpectralObjectInteractable::SpectralInteract()
{
	if (ObjectToInteract)
	{
		ObjectToInteract->Open();
	}
}
