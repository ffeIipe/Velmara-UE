#include "Features/WeaponUpgradeSystem/Data/WeaponUpgradeTypes.h"

#include "DataAssets/Items/Weapons/SwordData.h"
#include "GAS/ComboNode.h"

#define LOCTEXT_NAMESPACE "WeaponUpgrade"

namespace WeaponUpgradeUtils
{
	namespace
	{
		struct FComboRootEntry
		{
			EComboRoot Root;
			const UComboNode* Node;
		};

		void GetComboRoots(const UWeaponData* Weapon, TArray<FComboRootEntry>& OutRoots)
		{
			if (const USwordData* SwordData = Cast<USwordData>(Weapon))
			{
				OutRoots.Add({EComboRoot::Ground, SwordData->FirstGroundAction});
				OutRoots.Add({EComboRoot::Air, SwordData->FirstAirAction});
				OutRoots.Add({EComboRoot::Launch, SwordData->LaunchAction});
				OutRoots.Add({EComboRoot::Skill, SwordData->SkillAction});
			}
		}

		int32 CountBranchHits(const UComboNode* Node, const FGameplayTag& UnlockTag, TSet<const UComboNode*>& Visited)
		{
			if (!Node || Visited.Contains(Node)) return 0;

			// Un nodo con otro desbloqueo ya pertenece a otra rama.
			if (Node->RequiredUnlockTag.IsValid() && Node->RequiredUnlockTag != UnlockTag) return 0;

			Visited.Add(Node);
			return 1 + CountBranchHits(Node->NextActionInputTap, UnlockTag, Visited)
			         + CountBranchHits(Node->NextActionInputHold, UnlockTag, Visited);
		}

		bool SearchNode(const UComboNode* Node, const FGameplayTag& UnlockTag, const FGameplayTag& ParentLock,
		                TArray<EComboInput>& Inputs, TSet<const UComboNode*>& Visited, FComboInputPath& OutPath)
		{
			if (!Node || Visited.Contains(Node)) return false;
			Visited.Add(Node);

			if (Node->RequiredUnlockTag == UnlockTag)
			{
				OutPath.bFound = true;
				OutPath.Inputs = Inputs;
				OutPath.ParentUnlockTag = ParentLock;

				TSet<const UComboNode*> BranchVisited;
				OutPath.HitCount = CountBranchHits(Node, UnlockTag, BranchVisited);
				return true;
			}

			const FGameplayTag ChildParentLock = Node->RequiredUnlockTag.IsValid() ? Node->RequiredUnlockTag : ParentLock;

			Inputs.Add(EComboInput::Tap);
			if (SearchNode(Node->NextActionInputTap, UnlockTag, ChildParentLock, Inputs, Visited, OutPath)) return true;
			Inputs.Pop();

			Inputs.Add(EComboInput::Hold);
			if (SearchNode(Node->NextActionInputHold, UnlockTag, ChildParentLock, Inputs, Visited, OutPath)) return true;
			Inputs.Pop();

			return false;
		}

		void CollectFromNode(const UComboNode* Node, TArray<FGameplayTag>& OutTags, TSet<const UComboNode*>& Visited)
		{
			if (!Node || Visited.Contains(Node)) return;
			Visited.Add(Node);

			if (Node->RequiredUnlockTag.IsValid())
			{
				OutTags.AddUnique(Node->RequiredUnlockTag);
			}

			CollectFromNode(Node->NextActionInputTap, OutTags, Visited);
			CollectFromNode(Node->NextActionInputHold, OutTags, Visited);
		}
	}

	FComboInputPath FindComboPath(const UWeaponData* Weapon, const FGameplayTag& UnlockTag)
	{
		FComboInputPath Path;
		if (!UnlockTag.IsValid()) return Path;

		TArray<FComboRootEntry> Roots;
		GetComboRoots(Weapon, Roots);

		TSet<const UComboNode*> Visited;
		for (const FComboRootEntry& Entry : Roots)
		{
			// Suelo y aire arrancan con el input de ataque; lanzamiento y habilidad tienen su propio disparador.
			TArray<EComboInput> Inputs;
			if (Entry.Root == EComboRoot::Ground || Entry.Root == EComboRoot::Air)
			{
				Inputs.Add(EComboInput::Tap);
			}

			if (SearchNode(Entry.Node, UnlockTag, FGameplayTag(), Inputs, Visited, Path))
			{
				Path.Root = Entry.Root;
				return Path;
			}
		}

		return Path;
	}

	void CollectUnlockTags(const UWeaponData* Weapon, TArray<FGameplayTag>& OutTags)
	{
		TArray<FComboRootEntry> Roots;
		GetComboRoots(Weapon, Roots);

		TSet<const UComboNode*> Visited;
		for (const FComboRootEntry& Entry : Roots)
		{
			CollectFromNode(Entry.Node, OutTags, Visited);
		}
	}

	FText DescribeComboInput(const FComboInputPath& Path)
	{
		if (!Path.bFound)
		{
			return LOCTEXT("ComboNotInTree", "No está asignado a ningún nodo del árbol de combos");
		}

		FText RootText;
		switch (Path.Root)
		{
		case EComboRoot::Ground: RootText = LOCTEXT("RootGround", "Suelo"); break;
		case EComboRoot::Air:    RootText = LOCTEXT("RootAir", "Aire"); break;
		case EComboRoot::Launch: RootText = LOCTEXT("RootLaunch", "Lanzamiento"); break;
		case EComboRoot::Skill:  RootText = LOCTEXT("RootSkill", "Habilidad"); break;
		default: break;
		}

		if (Path.Inputs.IsEmpty()) return RootText;

		TArray<FText> InputTexts;
		for (const EComboInput Input : Path.Inputs)
		{
			InputTexts.Add(Input == EComboInput::Hold ? LOCTEXT("InputHold", "Mantener") : LOCTEXT("InputTap", "Pulsar"));
		}

		return FText::Format(LOCTEXT("ComboInputFormat", "{0}: {1}"), RootText, FText::Join(FText::FromString(TEXT(" · ")), InputTexts));
	}

	FText GetWeaponDisplayName(const UWeaponData* Weapon)
	{
		if (!Weapon) return FText::GetEmpty();
		if (!Weapon->DisplayName.IsEmpty()) return Weapon->DisplayName;

		FString Name = Weapon->GetName();
		Name.RemoveFromStart(TEXT("DA_"));
		Name.ReplaceInline(TEXT("_"), TEXT(" "));
		return FText::FromString(Name);
	}
}

#undef LOCTEXT_NAMESPACE
