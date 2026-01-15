#include "Player/PlayerMain.h"

#include "AbilitySystemComponent.h"
#include "SceneEvents/VelmaraGameMode.h"
#include "SceneEvents/VelmaraGameStateBase.h"
#include "SceneEvents/VelmaraGameInstance.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Components/InputComponent.h"
#include "Curves/CurveFloat.h"

#include "Camera/CameraActor.h"

#include "Kismet/GameplayStatics.h"

#include "UObject/ConstructorHelpers.h"

APlayerMain::APlayerMain()
{
	PrimaryActorTick.bCanEverTick = false;

	AutoPossessPlayer = EAutoReceiveInput::Player0;
	AutoPossessAI = EAutoPossessAI::Disabled;
}

void APlayerMain::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
	}
}

void APlayerMain::PerformDeath()
{
	Super::PerformDeath();

	if (PlayerControllerRef)
	{
		DisableInput(PlayerControllerRef);
	}

	FTimerHandle TimerHandle_LoadCheckpoint;
	GetWorldTimerManager().SetTimer(TimerHandle_LoadCheckpoint, this, &APlayerMain::LoadLastCheckpoint, 2.0f, false);
}

void APlayerMain::Revive()
{
	if (!IsAlive())
	{
		StopAnimMontage(GetCurrentMontage());

		if (PlayerControllerRef)
		{
			EnableInput(PlayerControllerRef);
		}

		GetCharacterMovement()->SetMovementMode(MOVE_Walking);
		//Execute_GetCharacterStateComponent(this)->SetAction(ECharacterActionsStates::ECAS_Nothing);
	}
}

void APlayerMain::ResetFollowCamera()
{
	if (FollowCamera && PlayerControllerRef)
	{
		//Execute_GetCharacterStateComponent(this)->SetAction(ECharacterActionsStates::ECAS_Nothing);
		FollowCamera->AttachToComponent(GetSpringArmComponent(), FAttachmentTransformRules::SnapToTargetIncludingScale, FName("SpringEndpoint"));
		PlayerControllerRef->EnableInput(PlayerControllerRef);
		bCanReceiveDamage = true;
		Cast<AVelmaraGameMode>(UGameplayStatics::GetGameMode(GetWorld()))->SetEnemiesAIEnabled(true);
	}
}

void APlayerMain::LoadLastCheckpoint() const
{
	if (UVelmaraGameInstance* VelmaraGameInstance = Cast<UVelmaraGameInstance>(GetWorld()->GetGameInstance()))
	{
		VelmaraGameInstance->LoadGame(VelmaraGameInstance->ActiveSaveSlotIndex);
	}
}