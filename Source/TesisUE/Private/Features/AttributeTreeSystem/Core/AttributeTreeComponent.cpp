#include "Features/AttributeTreeSystem/Core/AttributeTreeComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffect.h"
#include "TimerManager.h"
#include "Features/AttributeTreeSystem/UI/AttributeTreeMenuWidget.h"
#include "GAS/VelmaraAttributeSet.h"

#define LOCTEXT_NAMESPACE "AttributeTree"

UAttributeTreeComponent::UAttributeTreeComponent()
{
	MenuWidgetClass = UAttributeTreeMenuWidget::StaticClass();

	AddDefaultUpgrade(AttributeTreeTags::Stat_MaxHealth,
		LOCTEXT("MaxHealthName", "Vida máxima"),
		LOCTEXT("MaxHealthDesc", "Aumenta la vida máxima del personaje."),
		UVelmaraAttributeSet::GetMaxHealthAttribute(), {0.f, 20.f, 45.f, 75.f, 110.f},
		EAttributeUpgradeDisplay::Flat, UVelmaraAttributeSet::GetHealthAttribute());

	AddDefaultUpgrade(AttributeTreeTags::Stat_Armor,
		LOCTEXT("ArmorName", "Armadura"),
		LOCTEXT("ArmorDesc", "Reduce el daño que recibís en cada golpe."),
		UVelmaraAttributeSet::GetArmorAttribute(), {0.f, 10.f, 25.f, 45.f, 70.f},
		EAttributeUpgradeDisplay::Flat);

	AddDefaultUpgrade(AttributeTreeTags::Stat_BaseDamage,
		LOCTEXT("BaseDamageName", "Daño base"),
		LOCTEXT("BaseDamageDesc", "Suma daño a cada golpe que asestás."),
		UVelmaraAttributeSet::GetBaseDamageAttribute(), {0.f, 2.f, 4.f, 7.f, 10.f},
		EAttributeUpgradeDisplay::Flat);

	AddDefaultUpgrade(AttributeTreeTags::Stat_AttackSpeed,
		LOCTEXT("AttackSpeedName", "Velocidad de ataque"),
		LOCTEXT("AttackSpeedDesc", "Acelera las animaciones de ataque."),
		UVelmaraAttributeSet::GetAttackSpeedAttribute(), {0.f, 0.05f, 0.10f, 0.15f, 0.25f},
		EAttributeUpgradeDisplay::Percent);
}

void UAttributeTreeComponent::AddDefaultUpgrade(const FGameplayTag& Tag, const FText& Name, const FText& Description,
                                                const FGameplayAttribute& Attribute, const TArray<float>& TotalBonuses,
                                                const EAttributeUpgradeDisplay DisplayFormat,
                                                const FGameplayAttribute& LinkedCurrentAttribute)
{
	// Costos por nivel: I (inicial), II, III, IV, V.
	static const int32 DefaultCosts[] = {0, 100, 250, 450, 700};

	FAttributeUpgradeDefinition& Definition = Upgrades.AddDefaulted_GetRef();
	Definition.UpgradeTag = Tag;
	Definition.DisplayName = Name;
	Definition.Description = Description;
	Definition.Attribute = Attribute;
	Definition.LinkedCurrentAttribute = LinkedCurrentAttribute;
	Definition.DisplayFormat = DisplayFormat;

	for (int32 Index = 0; Index < TotalBonuses.Num(); ++Index)
	{
		FAttributeUpgradeLevel& Level = Definition.Levels.AddDefaulted_GetRef();
		Level.Cost = DefaultCosts[FMath::Min(Index, static_cast<int32>(UE_ARRAY_COUNT(DefaultCosts)) - 1)];
		Level.TotalBonus = TotalBonuses[Index];
	}
}

void UAttributeTreeComponent::BeginPlay()
{
	Super::BeginPlay();

	// El Entity inicializa el ASC y sus stats por defecto en su propio BeginPlay (después del de sus componentes),
	// así que esperamos un tick para aplicar los niveles ya comprados/cargados.
	GetWorld()->GetTimerManager().SetTimerForNextTick(this, &UAttributeTreeComponent::ReapplyAllUpgrades);
}

/* MEJORAS */

EAttributeUpgradeResult UAttributeTreeComponent::TryUpgrade(const FGameplayTag UpgradeTag)
{
	const FAttributeUpgradeDefinition* Definition = FindDefinition(UpgradeTag);
	if (!Definition) return EAttributeUpgradeResult::InvalidUpgrade;

	const int32 CurrentLevel = GetUpgradeLevel(UpgradeTag);
	if (CurrentLevel >= Definition->GetMaxLevel()) return EAttributeUpgradeResult::MaxLevelReached;

	// Validamos el ASC antes de cobrar para no perder moneda si no se puede aplicar el bonus.
	const UAbilitySystemComponent* AbilitySystem = GetAbilitySystem();
	if (!AbilitySystem) return EAttributeUpgradeResult::NoAbilitySystem;
	if (!AbilitySystem->HasAttributeSetForAttribute(Definition->Attribute)) return EAttributeUpgradeResult::InvalidUpgrade;

	const int32 NewLevel = CurrentLevel + 1;
	const int32 Cost = Definition->GetCostForLevel(NewLevel);

	switch (TryPay(Cost))
	{
	case EUpgradePaymentResult::NotEnoughCurrency:
		return EAttributeUpgradeResult::NotEnoughCurrency;
	case EUpgradePaymentResult::NoCurrencySystem:
		return EAttributeUpgradeResult::NoCurrencySystem;
	default:
		break;
	}

	UpgradeLevels.Add(UpgradeTag, NewLevel);
	ApplyUpgradeEffect(*Definition);

	OnAttributeUpgraded.Broadcast(UpgradeTag, NewLevel);
	NotifyCurrencyChanged();

	return EAttributeUpgradeResult::Success;
}

void UAttributeTreeComponent::SetUpgradeLevel(const FGameplayTag UpgradeTag, const int32 NewLevel)
{
	const FAttributeUpgradeDefinition* Definition = FindDefinition(UpgradeTag);
	if (!Definition) return;

	const int32 ClampedLevel = FMath::Clamp(NewLevel, 1, Definition->GetMaxLevel());
	UpgradeLevels.Add(UpgradeTag, ClampedLevel);
	ApplyUpgradeEffect(*Definition);

	OnAttributeUpgraded.Broadcast(UpgradeTag, ClampedLevel);
}

void UAttributeTreeComponent::ReapplyAllUpgrades()
{
	for (const FAttributeUpgradeDefinition& Definition : Upgrades)
	{
		ApplyUpgradeEffect(Definition);
	}
}

void UAttributeTreeComponent::RestoreUpgradeLevels(const TMap<FGameplayTag, int32>& SavedLevels)
{
	UpgradeLevels.Reset();

	for (const TPair<FGameplayTag, int32>& SavedLevel : SavedLevels)
	{
		// Las ramas que ya no existen en la configuración se descartan.
		if (const FAttributeUpgradeDefinition* Definition = FindDefinition(SavedLevel.Key))
		{
			UpgradeLevels.Add(SavedLevel.Key, FMath::Clamp(SavedLevel.Value, 1, Definition->GetMaxLevel()));
		}
	}

	// Si la carga llega antes del BeginPlay (ej. al abrir otro nivel), el ASC todavía no está inicializado:
	// BeginPlay ya programa ReapplyAllUpgrades para el próximo tick, así que alcanza con dejar los niveles cargados.
	if (HasBegunPlay())
	{
		ReapplyAllUpgrades();
	}

	for (const FAttributeUpgradeDefinition& Definition : Upgrades)
	{
		OnAttributeUpgraded.Broadcast(Definition.UpgradeTag, GetUpgradeLevel(Definition.UpgradeTag));
	}
}

void UAttributeTreeComponent::ApplyUpgradeEffect(const FAttributeUpgradeDefinition& Definition)
{
	UAbilitySystemComponent* AbilitySystem = GetAbilitySystem();
	if (!AbilitySystem || !Definition.Attribute.IsValid()) return;

	if (!AbilitySystem->HasAttributeSetForAttribute(Definition.Attribute))
	{
		UE_LOG(LogTemp, Warning, TEXT("AttributeTree: %s no tiene un AttributeSet con el atributo %s."),
		       *GetNameSafe(GetOwner()), *Definition.Attribute.GetName());
		return;
	}

	const FGameplayTag& Tag = Definition.UpgradeTag;
	const float PreviousBonus = AppliedBonuses.FindRef(Tag);
	const float NewBonus = Definition.GetTotalBonusAtLevel(GetUpgradeLevel(Tag));

	// Reemplazamos el GE anterior de la rama por uno con el bonus total del nivel actual.
	if (const FActiveGameplayEffectHandle* ExistingHandle = ActiveUpgradeEffectHandles.Find(Tag))
	{
		AbilitySystem->RemoveActiveGameplayEffect(*ExistingHandle);
	}
	ActiveUpgradeEffectHandles.Remove(Tag);
	UpgradeEffects.Remove(Tag);

	if (!FMath::IsNearlyZero(NewBonus))
	{
		UGameplayEffect* Effect = NewObject<UGameplayEffect>(this, NAME_None, RF_Transient);
		Effect->DurationPolicy = EGameplayEffectDurationType::Infinite;

		FGameplayModifierInfo& Modifier = Effect->Modifiers.AddDefaulted_GetRef();
		Modifier.Attribute = Definition.Attribute;
		Modifier.ModifierOp = EGameplayModOp::AddBase;
		Modifier.ModifierMagnitude = FGameplayEffectModifierMagnitude(FScalableFloat(NewBonus));

		const FActiveGameplayEffectHandle Handle = AbilitySystem->ApplyGameplayEffectToSelf(Effect, 1.f, AbilitySystem->MakeEffectContext());
		if (Handle.IsValid())
		{
			ActiveUpgradeEffectHandles.Add(Tag, Handle);
			UpgradeEffects.Add(Tag, Effect);
		}
	}

	AppliedBonuses.Add(Tag, NewBonus);

	// Si sube un máximo (ej. MaxHealth), el valor actual (Health) recibe la misma diferencia.
	const float Delta = NewBonus - PreviousBonus;
	if (Delta > 0.f && Definition.LinkedCurrentAttribute.IsValid() && AbilitySystem->HasAttributeSetForAttribute(Definition.LinkedCurrentAttribute))
	{
		AbilitySystem->ApplyModToAttribute(Definition.LinkedCurrentAttribute, EGameplayModOp::AddBase, Delta);
	}
}

/* CONSULTAS */

int32 UAttributeTreeComponent::GetUpgradeLevel(const FGameplayTag UpgradeTag) const
{
	const FAttributeUpgradeDefinition* Definition = FindDefinition(UpgradeTag);
	if (!Definition) return 0;

	const int32* FoundLevel = UpgradeLevels.Find(UpgradeTag);
	return FMath::Clamp(FoundLevel ? *FoundLevel : 1, 1, Definition->GetMaxLevel());
}

int32 UAttributeTreeComponent::GetMaxLevel(const FGameplayTag UpgradeTag) const
{
	const FAttributeUpgradeDefinition* Definition = FindDefinition(UpgradeTag);
	return Definition ? Definition->GetMaxLevel() : 0;
}

bool UAttributeTreeComponent::IsMaxLevel(const FGameplayTag UpgradeTag) const
{
	const FAttributeUpgradeDefinition* Definition = FindDefinition(UpgradeTag);
	return Definition && GetUpgradeLevel(UpgradeTag) >= Definition->GetMaxLevel();
}

int32 UAttributeTreeComponent::GetNextLevelCost(const FGameplayTag UpgradeTag) const
{
	const FAttributeUpgradeDefinition* Definition = FindDefinition(UpgradeTag);
	if (!Definition || IsMaxLevel(UpgradeTag)) return -1;

	return Definition->GetCostForLevel(GetUpgradeLevel(UpgradeTag) + 1);
}

bool UAttributeTreeComponent::CanAffordUpgrade(const FGameplayTag UpgradeTag) const
{
	const int32 Cost = GetNextLevelCost(UpgradeTag);
	return Cost >= 0 && CanPay(Cost);
}

bool UAttributeTreeComponent::GetUpgradeDefinition(const FGameplayTag UpgradeTag, FAttributeUpgradeDefinition& OutDefinition) const
{
	if (const FAttributeUpgradeDefinition* Definition = FindDefinition(UpgradeTag))
	{
		OutDefinition = *Definition;
		return true;
	}
	return false;
}

float UAttributeTreeComponent::GetCurrentAttributeValue(const FGameplayTag UpgradeTag) const
{
	const FAttributeUpgradeDefinition* Definition = FindDefinition(UpgradeTag);
	const UAbilitySystemComponent* AbilitySystem = GetAbilitySystem();

	if (!Definition || !AbilitySystem || !AbilitySystem->HasAttributeSetForAttribute(Definition->Attribute)) return 0.f;

	return AbilitySystem->GetNumericAttribute(Definition->Attribute);
}

const FAttributeUpgradeDefinition* UAttributeTreeComponent::FindDefinition(const FGameplayTag& UpgradeTag) const
{
	return Upgrades.FindByPredicate([&UpgradeTag](const FAttributeUpgradeDefinition& Definition)
	{
		return Definition.UpgradeTag == UpgradeTag;
	});
}

UAbilitySystemComponent* UAttributeTreeComponent::GetAbilitySystem() const
{
	return UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner());
}

#undef LOCTEXT_NAMESPACE
