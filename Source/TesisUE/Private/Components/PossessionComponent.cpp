#include "Components/PossessionComponent.h"

//Components
#include "Components/CharacterStateComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/InventoryComponent.h"
#include "Components/CombatComponent.h"

//Entities
#include "Entities/Entity.h"
#include "Kismet/KismetSystemLibrary.h"
#include <Enemy/Enemy.h>

//Misc
#include "Items/Item.h"
#include <Kismet/GameplayStatics.h>

UPossessionComponent::UPossessionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;

}

void UPossessionComponent::BeginPlay()
{
	Super::BeginPlay();

	EntityOwner = Cast<AEntity>(GetOwner());
	PlayerControllerRef = Cast<APlayerController>(EntityOwner->GetController());
}


AEntity* UPossessionComponent::GetPossessionVictim()
{
	if (!PlayerControllerRef) return nullptr;

	FRotator CameraRotation;
	FVector Start = EntityOwner->GetActorLocation() + EntityOwner->GetActorForwardVector() * 100.f;
	PlayerControllerRef->GetPlayerViewPoint(Start, CameraRotation);
	FVector Direction = CameraRotation.Vector();

	FVector End = Start + Direction * PossessDistance;

	TArray<AActor*> ActorsToIgnore;
	ActorsToIgnore.Add(GetOwner());

	FHitResult ResultHit;

	bool bHit = UKismetSystemLibrary::SphereTraceSingle(
		GetWorld(),
		Start,
		End,
		PossessRadius,
		ETraceTypeQuery::TraceTypeQuery4,
		false,
		ActorsToIgnore,
		EDrawDebugTrace::ForDuration,
		ResultHit,
		true
	);

	if (!bHit) return nullptr;

	AEntity* EnemyTraced = Cast<AEntity>(ResultHit.GetActor());
	if (!EnemyTraced) return nullptr;

	UPossessionComponent* EntityPossessionComponent = EnemyTraced->GetPossessionComponent();
	if (!EntityPossessionComponent) return nullptr;

	if (EntityPossessionComponent->CanBePossessed())
	{
		return EnemyTraced;
	}
	else return nullptr;
}

void UPossessionComponent::Possess()
{
	if (EntityOwner->GetCharacterStateComponent()->GetCurrentCharacterState().Form == ECharacterForm::ECF_Spectral)
	{
		AEntity* EntityTraced = GetPossessionVictim();
		
		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Yellow, FString("1"));

		if (!PlayerControllerRef || !EntityTraced) return;

		GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Orange, FString("2"));

		if (EntityTraced->GetCharacterStateComponent()->GetCurrentCharacterState().Action == ECharacterActions::ECA_Dead) return;

		if (EntityOwner->GetAttributeComponent()->RequiresEnergy(10.f))
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("3"));

			PlayerControllerRef->Possess(EntityTraced);

			EntityPossessed = EntityTraced;
			EntityPossessed->GetPossessionComponent()->ReceivePossession(EntityOwner, EntityPossessed, EntityOwner->GetAttributeComponent()->GetEnergy());

			if (EntityOwner->GetInventoryComponent()->GetEquippedItem())
			{
				EntityOwner->GetInventoryComponent()->GetEquippedItem()->SetActorHiddenInGame(true);
			}

			EntityOwner->GetCharacterStateComponent()->SetCharacterForm(ECharacterForm::ECF_Possessing);
			EntityOwner->SetActorHiddenInGame(true);
			EntityOwner->SetActorEnableCollision(false);
			EntityOwner->GetMesh()->bPauseAnims = true;
		}
		else
		{
			if (OnCannotPossess.IsBound())
			{
				OnCannotPossess.Broadcast();
			}
		}

	}
	else EntityOwner->GetCombatComponent()->Input_Execute();
}

void UPossessionComponent::ReceivePossession(AEntity* NewOwner, AEntity* TargetEntity, float OwnerEnergy)
{
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, FString("Receiving possession..."));

	if (!IsValid(NewOwner) || !IsValid(TargetEntity)) return;

	PossessionOwner = NewOwner;
	EntityOwner->GetAttributeComponent()->SetEnergy(OwnerEnergy);
	EntityOwner->bUseControllerRotationYaw = false;
	PossessionOwner->AttachFollowCamera(EntityOwner->GetSpringArmComponent());

	if (OnPossessed.IsBound())
	{
		OnPossessed.Broadcast();
	}

	//NewOwner->PlayerControllerRef->PlayerMainHUD->TogglePaladinUI(true);

	if (EntityOwner->GetAttributeComponent())
	{
		EntityOwner->GetAttributeComponent()->StartDecreaseEnergy();
	}
}

void UPossessionComponent::ReleasePossession()
{
	GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Blue, FString("Releasing possession..."));
	PlayerControllerRef->Possess(EntityOwner);

	EntityOwner->SetActorLocationAndRotation(EntityPossessed->GetActorLocation() + FVector(100.f, 0.f, 0.f), EntityPossessed->GetActorRotation());
	EntityOwner->SetActorHiddenInGame(false);
	EntityOwner->SetActorEnableCollision(true);
	EntityOwner->GetMesh()->bPauseAnims = false;
	EntityOwner->AttachFollowCamera(EntityOwner->GetSpringArmComponent());

	EntityPossessed->bUseControllerRotationYaw = true;
	EntityPossessed->GetPossessionComponent()->PossessionOwner = nullptr;

	if (EntityPossessed->GetPossessionComponent()->OnReleasePossession.IsBound())
	{
		EntityPossessed->GetPossessionComponent()->OnReleasePossession.Broadcast();
	}

	EntityPossessed = nullptr;
}

void UPossessionComponent::ReleaseAndExecute()
{
	if (EntityOwner && EntityOwner->GetAttributeComponent()->RequiresEnergy(ReleaseAndExecuteEnergyTax))
	{
		EntityOwner->GetAttributeComponent()->IncreaseHealth(15.f);
		EntityPossessed->GetAttributeComponent()->IncreaseEnergy(-ReleaseAndExecuteEnergyTax);

		EntityPossessed->StopAnimMontage();

		if (EntityPossessed->GetPossessionComponent()->OnReleaseAndExecute.IsBound())
		{
			EntityPossessed->GetPossessionComponent()->OnReleaseAndExecute.Broadcast();
		}

		EntityPossessed->GetPossessionComponent()->PossessionOwner = nullptr;
		EntityPossessed = nullptr;
	}
	else
	{
		if (OnCannotRelease.IsBound())
		{
			OnCannotRelease.Broadcast();
		}
	}
}
