#include "AI/Core/CombatDirectorSubsystem.h"
#include "Engine/Engine.h"

static TAutoConsoleVariable<bool> CVarDebugCombatDirector(
    TEXT("AI.DebugCombatDirector"), false, TEXT("Muestra en pantalla los turnos de ataque que reparte el CombatDirector."));

static void DirectorDebug(const FColor Color, const FString& Message)
{
    if (GEngine && CVarDebugCombatDirector.GetValueOnGameThread())
    {
        GEngine->AddOnScreenDebugMessage(INDEX_NONE, 2.f, Color, Message);
    }
}

void UCombatDirectorSubsystem::RegisterEnemy(AActor* Enemy, AActor* TargetActor)
{
    if (!Enemy || !TargetActor) return;

    FCombatTargetData& Data = CombatMap.FindOrAdd(TargetActor);

    if (!Data.RegisteredEnemies.Contains(Enemy))
    {
        Data.RegisteredEnemies.Add(Enemy);
    }
}

void UCombatDirectorSubsystem::UnregisterEnemy(AActor* Enemy, AActor* TargetActor)
{
    if (!TargetActor) return;

    if (FCombatTargetData* Data = CombatMap.Find(TargetActor))
    {
        Data->RegisteredEnemies.Remove(Enemy);

        if (Data->AttackingEnemies.Remove(Enemy) > 0)
        {
            DirectorDebug(FColor::Orange, FString::Printf(TEXT("DIRECTOR: %s Unregistered & Forced Token Return. Slots: %d / %d"),
                *GetNameSafe(Enemy), Data->AttackingEnemies.Num(), Data->MaxConcurrentAttackers));
        }

        if (Data->RegisteredEnemies.Num() == 0 && Data->AttackingEnemies.Num() == 0)
        {
            CombatMap.Remove(TargetActor);
        }
    }
}

TArray<AActor*> UCombatDirectorSubsystem::GetAlliesForTarget(AActor* TargetActor) const
{
    TArray<AActor*> Result;
    if (!TargetActor) return Result;

    if (const FCombatTargetData* Data = CombatMap.Find(TargetActor))
    {
        for (const TWeakObjectPtr<AActor>& WeakEnemy : Data->RegisteredEnemies)
        {
            if (AActor* LiveEnemy = WeakEnemy.Get())
            {
                Result.Add(LiveEnemy);
            }
        }
    }
    return Result;
}

bool UCombatDirectorSubsystem::RequestAttackToken(AActor* Enemy, AActor* TargetActor)
{
    if (!Enemy || !TargetActor) return false;

    FCombatTargetData& Data = CombatMap.FindOrAdd(TargetActor);

    Data.AttackingEnemies.RemoveAll([](const TWeakObjectPtr<AActor>& Ptr) { return !Ptr.IsValid(); });

    if (Data.AttackingEnemies.Contains(Enemy)) return true;

    const double Now = Enemy->GetWorld()->GetTimeSeconds();
    if (const double* Until = Data.CooldownUntil.Find(Enemy); Until && Now < *Until) return false;

    if (Data.AttackingEnemies.Num() < Data.MaxConcurrentAttackers)
    {
        Data.AttackingEnemies.Add(Enemy);
        DirectorDebug(FColor::Green, FString::Printf(TEXT("DIRECTOR: Token GRANTED to %s. Slots Used: %d / %d"),
            *Enemy->GetName(), Data.AttackingEnemies.Num(), Data.MaxConcurrentAttackers));
        return true;
    }

    return false;
}

void UCombatDirectorSubsystem::ReturnAttackToken(AActor* Enemy, AActor* TargetActor)
{
    if (!TargetActor) return;

    if (FCombatTargetData* Data = CombatMap.Find(TargetActor))
    {
        if (Data->AttackingEnemies.Remove(Enemy) > 0)
        {
            DirectorDebug(FColor::Cyan, FString::Printf(TEXT("DIRECTOR: %s Returned Token. Slots Used: %d / %d"),
                *GetNameSafe(Enemy), Data->AttackingEnemies.Num(), Data->MaxConcurrentAttackers));
        }
    }
}

void UCombatDirectorSubsystem::ReturnAttackTokenWithCooldown(AActor* Enemy, AActor* TargetActor, const float Cooldown)
{
    ReturnAttackToken(Enemy, TargetActor);

    if (Enemy && TargetActor && Cooldown > 0.f)
    {
        CombatMap.FindOrAdd(TargetActor).CooldownUntil.Add(Enemy, Enemy->GetWorld()->GetTimeSeconds() + Cooldown);
    }
}

void UCombatDirectorSubsystem::ReturnAllAttackTokens(AActor* Enemy)
{
    for (TPair<TWeakObjectPtr<AActor>, FCombatTargetData>& Pair : CombatMap)
    {
        if (Pair.Value.AttackingEnemies.Remove(Enemy) > 0)
        {
            DirectorDebug(FColor::Cyan, FString::Printf(TEXT("DIRECTOR: %s Released Token."), *GetNameSafe(Enemy)));
        }
    }
}

bool UCombatDirectorSubsystem::HasAttackToken(AActor* Enemy, AActor* TargetActor) const
{
    const FCombatTargetData* Data = TargetActor ? CombatMap.Find(TargetActor) : nullptr;
    return Data && Data->AttackingEnemies.Contains(Enemy);
}
