#include "Enemy/Paladin.h"
#include "Player/PlayerMain.h"

#include "GameFramework/DamageType.h"
#include "DamageTypes/MeleeDamage.h"

#include "GameFramework/CharacterMovementComponent.h"
#include "Components/AttributeComponent.h"
#include "Components/CharacterStateComponent.h"
#include "Components/CapsuleComponent.h"

#include "Kismet/GameplayStatics.h"

#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include <Kismet/KismetMathLibrary.h>

#include "DataAssets/EntityData.h"
#include "Interfaces/Weapon/WeaponInterface.h"

#include "DataAssets/MontagesData.h"


APaladin::APaladin()
{
	WeaponToEquip = CreateDefaultSubobject<UChildActorComponent>(TEXT("Weapon"));
	WeaponToEquip->SetupAttachment(GetMesh());
	
	GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
	GetMesh()->SetCollisionResponseToChannel(ECC_GameTraceChannel2, ECR_Ignore);
	
	GetAttributeComponent()->AttachShield(GetMesh(), FName("LeftHandSocket"));

	OnShieldTakeDamage.AddDynamic(this, &APaladin::ShieldHit);
}

void APaladin::BeginPlay()
{
	Super::BeginPlay();

	if (!BBComponent)
	{
		if (AIController)
		{
			BBComponent = Cast<UBlackboardComponent>(AIController->GetBlackboardComponent());
		}
		else
		{
			AIController = Cast<AAIController>(GetController());
			BBComponent = Cast<UBlackboardComponent>(AIController->GetBlackboardComponent());
		}
	}

	if (!WeaponToEquip->GetChildActor())
	{
		WeaponToEquip->CreateChildActor();
	}

	
	TArray<AActor*> AttachedActors;
	GetAttachedActors(AttachedActors);
	
	for (AActor* AttachedActor : AttachedActors)
	{
		if (const TScriptInterface<IWeaponInterface> WeaponInterface = AttachedActor)
		{
			GetInventoryComponent()->TryAddWeapon(WeaponInterface);
		}
		else if (GEngine)
		{
			GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Red, "Not valid weapon attached to actor.");
		}
	}
}

void APaladin::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	if (EntityData)
	{
		if (EntityData->InitialWeapon)
		{
			WeaponToEquip->SetChildActorClass(EntityData->InitialWeapon);
		}
	}
}

void APaladin::ActivateEnemy(const FVector& Location, const FRotator& Rotation)
{
	Super::ActivateEnemy(Location, Rotation);

	if (CharacterStateComponent)
	{
		CharacterStateComponent->SetWeaponState(ECharacterWeaponStates::ECWS_EquippedWeapon);
	}

	SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);
}

void APaladin::DeactivateEnemy()
{
	SetWeaponCollisionEnabled(ECollisionEnabled::NoCollision);

	Super::DeactivateEnemy();
}

bool APaladin::IsLaunchable()
{
	return !GetAttributeComponent()->IsShielded(); //returns false if it has shield equipped or not detached yet.
}

void APaladin::LaunchUp()
{
	Super::LaunchUp();
	
	if (bIsLaunched) return;

	if (GEngine) GEngine->AddOnScreenDebugMessage(INDEX_NONE, 3.f, FColor::Green, FString(this->GetName()));
	bIsLaunched = true;
	DisableAI();
	Execute_PlayAnimMontage(this, MontagesData->Montages.HitReactMontage, 1.f, FName("FromAir"));
	GetCharacterMovement()->SetMovementMode(MOVE_Flying);
}

void APaladin::Die(UAnimMontage* DeathAnim, const FName Section)
{
	Super::Die(DeathAnim, Section);
	
	NotifyDamageTakenToBlackboard(LastDamageCauser);
}

void APaladin::ShieldHit()
{
	if (MontagesData->Montages.HitReactMontage)
	{
		Execute_StopAnimMontage(this, GetCurrentMontage());
		Execute_PlayAnimMontage(this, MontagesData->Montages.HitReactMontage, 1.f, FName("ShieldHit"));
	}
}

void APaladin::CrashDown()
{
	GetCharacterMovement()->SetMovementMode(MOVE_Falling);
	Execute_PlayAnimMontage(this, MontagesData->Montages.HitReactMontage, 1.f, FName("KnockDown"));
	LaunchCharacter(FVector(0.f, 0.f, -100000.f), true, true);
}

void APaladin::HitInAir()
{
	const float PlayerLocationHeight = UGameplayStatics::GetPlayerPawn(GetWorld(), 0)->GetActorLocation().Z;
	SetActorLocation(FVector(GetTargetActorLocation().X, GetTargetActorLocation().Y, PlayerLocationHeight));
	Execute_PlayAnimMontage(this, MontagesData->Montages.HitReactMontage, 1.f, FName("FromAir"));
	DisableAI();
}

void APaladin::ReactToDamage(const EMeleeDamageTypes DamageType, const FVector& ImpactPoint)
{
	switch (DamageType)
	{
	case EMeleeDamageTypes::EMDT_CrashDown:
		CrashDown();
		break;

	case EMeleeDamageTypes::EMDT_InAir:
		HitInAir();
		break;

	case EMeleeDamageTypes::EMDT_Finisher:
		FinishedDamage();
		break;

	case EMeleeDamageTypes::EMDT_Slash:
		Slash();
		break;

	case EMeleeDamageTypes::EMDT_Puncture:
		Execute_PlayAnimMontage(this, MontagesData->Montages.HitReactMontage, 1.f, FName("PunctureReact"));
		break;

	case EMeleeDamageTypes::EMDT_Impact:
		Execute_PlayAnimMontage(this, MontagesData->Montages.HitReactMontage, 1.f, FName("ImpactReact"));
		break;

	/*case EMainDamageTypes::EMDT_Pistol:*/
		
		
	case EMeleeDamageTypes::EMDT_None:
		GetDirectionalReact(ImpactPoint);
		break;

	default:
		GetDirectionalReact(ImpactPoint);
		break;
	}
}

void APaladin::Slash()
{
	const FRotator DamageCauserLocation = UKismetMathLibrary::FindLookAtRotation(GetTargetActorLocation(), LastDamageCauser->GetTargetActorLocation());

	SetActorRotation(FRotator(0.f, DamageCauserLocation.Yaw, 0.f));

	Execute_StopAnimMontage(this, GetCurrentMontage());
	Execute_PlayAnimMontage(this, MontagesData->Montages.HitReactMontage, 1.f, FName("FromFrontBig"));
}
