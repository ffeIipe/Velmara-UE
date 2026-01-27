#include "AI/Core/CombatDirectorSubsystem.h"

void UCombatDirectorSubsystem::RegisterEnemy(AActor* Enemy, AActor* TargetActor)
{
    if (!Enemy || !TargetActor) return;

    FCombatTargetData& Data = CombatMap.FindOrAdd(TargetActor);
    
    // Evitamos duplicados
    if (!Data.RegisteredEnemies.Contains(Enemy))
    {
        Data.RegisteredEnemies.Add(Enemy);
    }
}

void UCombatDirectorSubsystem::UnregisterEnemy(AActor* Enemy, AActor* TargetActor)
{
    if (!TargetActor) return; // Si el target murió, el mapa se limpiará eventualmente o podemos forzarlo.

    if (FCombatTargetData* Data = CombatMap.Find(TargetActor))
    {
        // 1. Quitar de la lista general
        Data->RegisteredEnemies.Remove(Enemy);

        // 2. Si tenía un token de ataque, quitárselo forzosamente para liberar el cupo
        if (Data->AttackingEnemies.Contains(Enemy))
        {
            Data->AttackingEnemies.Remove(Enemy);
        }

        // Si ya no quedan enemigos para este target, podríamos limpiar la entrada del mapa para ahorrar RAM
        if (Data->RegisteredEnemies.Num() == 0)
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

    // 1. Limpieza preventiva: Verificamos si alguien murió con el token en la mano y no avisó
    CleanInvalidEntries(Data);

    // 2. Si ya tengo token, retorno true (Idempotencia)
    if (Data.AttackingEnemies.Contains(Enemy))
    {
        return true;
    }

    // 3. Chequeo de cupo
    if (Data.AttackingEnemies.Num() < Data.MaxConcurrentAttackers)
    {
        Data.AttackingEnemies.Add(Enemy);
        return true;
    }

    return false; // Cupo lleno, sigue rodeando
}

void UCombatDirectorSubsystem::ReturnAttackToken(AActor* Enemy, AActor* TargetActor)
{
    if (!TargetActor) return;

    if (FCombatTargetData* Data = CombatMap.Find(TargetActor))
    {
        Data->AttackingEnemies.Remove(Enemy);
        // Opcional: Podríamos avisar a otros enemigos que se liberó un slot, 
        // pero el BTService lo chequeará en el próximo tick.
    }
}

void UCombatDirectorSubsystem::CleanInvalidEntries(FCombatTargetData& Data)
{
    // Elimina cualquier actor que haya sido destruido (IsStale)
    Data.AttackingEnemies.RemoveAll([](const TWeakObjectPtr<AActor>& Ptr)
    {
        return !Ptr.IsValid(); 
    });
}