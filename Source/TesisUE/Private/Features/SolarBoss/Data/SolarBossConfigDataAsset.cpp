#include "Features/SolarBoss/Data/SolarBossConfigDataAsset.h"

#include "Features/BossSystem/Actors/BossLightShaft.h"

USolarBossConfigDataAsset::USolarBossConfigDataAsset()
{
	LightShaftClass = ABossLightShaft::StaticClass();
	DamageSetByCallerTag = FGameplayTag::RequestGameplayTag(FName("Damage"), false);

	FSolarBossCycleConfig First;
	First.HealthThreshold = 0.75f;
	First.LightShaftCount = 2;
	First.MinionCountToSpawn = 2;
	Cycles.Add(First);

	FSolarBossCycleConfig Second;
	Second.HealthThreshold = 0.5f;
	Second.LightShaftCount = 3;
	Second.MinionCountToSpawn = 3;
	Cycles.Add(Second);

	FSolarBossCycleConfig Third;
	Third.HealthThreshold = 0.25f;
	Third.bFullCeilingCollapse = true;
	Third.LightShaftCount = 0;
	Third.MinionCountToSpawn = 4;
	Cycles.Add(Third);
}
