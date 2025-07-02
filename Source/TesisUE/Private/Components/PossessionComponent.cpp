#include "Components/PossessionComponent.h"

//Components
#include "Components/CharacterStateComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/InventoryComponent.h"

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
}

void UPossessionComponent::Possess()
{
	//if (EntityReference->GetCharacterStateComponent()->GetCurrentCharacterState().Form == ECharacterForm::ECF_Spectral && !IsEquipping())
	{
		AEnemy* EnemyToPossess = GetPossessionVictim();
		APlayerController* PlayerControllerRef = Cast<APlayerController>(EntityOwner->GetController());

		if (!PlayerControllerRef || !EnemyToPossess) return;

		if (EnemyToPossess->GetCharacterStateComponent()->GetCurrentCharacterState().Action == ECharacterActions::ECA_Dead) return;

		if (EnemyToPossess->GetAttributeComponent()->RequiresEnergy(10.f))
		{
			EnemyToPossess->DisableAI();
			PlayerControllerRef->Possess(EnemyToPossess);
			EnemyToPossess = EnemyPossessed;

			EnemyPossessed->EnableInput(PlayerControllerRef);
			EnemyPossessed->AutoPossessPlayer = EAutoReceiveInput::Player0;
			EnemyPossessed->GetPossessionComponent()->ReceivePossession(EntityOwner, EnemyPossessed, EntityOwner->GetAttributeComponent()->GetEnergy());

			EntityOwner->AttachFollowCamera(EnemyToPossess->GetSpringArmComponent());

			EntityOwner->GetCharacterStateComponent()->SetCharacterForm(ECharacterForm::ECF_Possessing);
			EntityOwner->SetActorHiddenInGame(true);

			if (EntityOwner->GetInventoryComponent()->GetEquippedItem())
			{
				EntityOwner->GetInventoryComponent()->GetEquippedItem()->SetActorHiddenInGame(true);
			}

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
	//else CombatComponent->Execute();
}

AEnemy* UPossessionComponent::GetPossessionVictim()
{
	FVector Start;
	FVector End;

	Start = GetOwner()->GetActorLocation();
	End = Start + EntityOwner->GetLastMovementInputVector() * PossessDistance;

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

	AEnemy* EnemyTraced = Cast<AEnemy>(ResultHit.GetActor());
	if (!EnemyTraced) return nullptr;

	UPossessionComponent* EntityPossessionComponent = EnemyTraced->GetPossessionComponent();
	if (!EntityPossessionComponent) return nullptr;

	if (EntityPossessionComponent->CanBePossessed())
	{
		return EnemyTraced;
	}
	else return nullptr;
}

void UPossessionComponent::ReceivePossession(AEntity* NewOwner, AEnemy* TargetEnemy, float OwnerEnergy)
{
	if (!IsValid(NewOwner) || !IsValid(TargetEnemy)) return;

	TargetEnemy->GetPossessionComponent()->PossessionOwner = NewOwner;
	TargetEnemy->GetAttributeComponent()->SetEnergy(OwnerEnergy);
	TargetEnemy->bUseControllerRotationYaw = false;

	if (OnPossessed.IsBound())
	{
		OnPossessed.Broadcast();
	}

	//NewOwner->PlayerControllerRef->PlayerMainHUD->TogglePaladinUI(true);

	if (TargetEnemy->GetAttributeComponent())
	{
		TargetEnemy->GetAttributeComponent()->StartDecreaseEnergy();
	}
}

void UPossessionComponent::ReleasePossession()
{
	EnemyPossessed->bUseControllerRotationYaw = true;

	//GetDefaultParameters(); // gettear y settear parametros por defecto... se puede mejorar con algo mas lindo como un struct que gettee y settee... 

	//PlayerControllerRef->PlayerMainHUD->TogglePaladinUI(false);

	EnemyPossessed->DisableAI();
	EnemyPossessed->EnableAI();

	EnemyPossessed->GetPossessionComponent()->PossessionOwner = nullptr;
}

void UPossessionComponent::ReleaseAndExecute()
{
	if (EntityOwner && EntityOwner->GetAttributeComponent()->RequiresEnergy(ReleaseAndExecuteEnergyTax))
	{
		EntityOwner->GetAttributeComponent()->IncreaseHealth(15.f);
		EnemyPossessed->GetAttributeComponent()->IncreaseEnergy(-ReleaseAndExecuteEnergyTax);

		EnemyPossessed->DisableAI();
		EnemyPossessed->StopAnimMontage();
		//EnemyPossessed->PlayAnimMontage(DeathMontage, 1.f, FName("UnpossessDeath"));
		EnemyPossessed->GetLastDamageCauser();
		EnemyPossessed->Die();

		if (OnReleaseAndExecute.IsBound())
		{
			OnReleaseAndExecute.Broadcast();
		}

		EnemyPossessed->GetPossessionComponent()->PossessionOwner = nullptr;
	}
	else
	{
		if (OnCannotRelease.IsBound())
		{
			OnCannotRelease.Broadcast();
		}
	}
}
