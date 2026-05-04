#include "GAS/AbilityTasks/UAbilityTask_PlayCurve.h"
#include "Math/UnrealMathUtility.h"

UAbilityTask_PlayCurve::UAbilityTask_PlayCurve()
{
    bTickingTask = true;
    bReverse = false;
    DurationMultiplier = 1.0f;
    UpdateInterval = 1.0f;
    TimeSinceLastUpdate = 0.0f;
}

UAbilityTask_PlayCurve* UAbilityTask_PlayCurve::PlayCurve(UGameplayAbility* OwningAbility, UCurveFloat* CurveAsset, float DurationMultiplier, float UpdateInterval, bool bReverse)
{
    UAbilityTask_PlayCurve* MyTask = NewAbilityTask<UAbilityTask_PlayCurve>(OwningAbility);
    MyTask->CurveFloat = CurveAsset;
    MyTask->DurationMultiplier = DurationMultiplier;
    MyTask->UpdateInterval = UpdateInterval;
    MyTask->bReverse = bReverse;
    return MyTask;
}

void UAbilityTask_PlayCurve::Activate()
{
    Super::Activate();
    
    if (CurveFloat)
    {
       float MinTime;
       CurveFloat->GetTimeRange(MinTime, MaxTime);
    }
    else
    {
       MaxTime = 1.0f;
    }
    
    if (bReverse)
    {
       CurrentTime = MaxTime;
    }
    else
    {
       CurrentTime = 0.0f;
    }

    // Forzamos que el primer tick emita el evento OnUpdate inmediatamente
    TimeSinceLastUpdate = UpdateInterval;
}

void UAbilityTask_PlayCurve::TickTask(float DeltaTime)
{
    Super::TickTask(DeltaTime);

    if (ShouldBroadcastAbilityTaskDelegates())
    {
       float TimeStep = DeltaTime * DurationMultiplier;

       if (bReverse)
       {
          CurrentTime -= TimeStep;
       }
       else
       {
          CurrentTime += TimeStep;
       }

       // Acumulamos el tiempo real de este frame
       TimeSinceLastUpdate += DeltaTime;

       bool bShouldFinish = false;

       if (bReverse)
       {
          if (CurrentTime <= 0.0f)
          {
             bShouldFinish = true;
             CurrentTime = 0.0f; 
          }
       }
       else
       {
          if (CurrentTime >= MaxTime)
          {
             bShouldFinish = true;
             CurrentTime = MaxTime; // Clampeamos al final de la curva por seguridad
          }
       }

       // Condición clave: ¿Pasó el intervalo de tiempo O terminó la curva?
       if (TimeSinceLastUpdate >= UpdateInterval || bShouldFinish)
       {
           float CurrentValue = 0.0f;
           if (CurveFloat)
           {
              CurrentValue = CurveFloat->GetFloatValue(CurrentTime);
           }
           else
           {
              CurrentValue = FMath::Clamp(CurrentTime / MaxTime, 0.0f, 1.0f);
           }

           OnUpdate.Broadcast(CurrentValue, CurrentTime);
           
           // Reseteamos el acumulador conservando el remanente (evita desincronizaciones)
           TimeSinceLastUpdate = FMath::Fmod(TimeSinceLastUpdate, UpdateInterval); 
       }

       if (bShouldFinish)
       {
          // Evaluamos el valor final justo antes de terminar para OnFinished
          float FinalValue = 0.0f;
          if (CurveFloat)
          {
              FinalValue = CurveFloat->GetFloatValue(CurrentTime);
          }
          
          OnFinished.Broadcast(FinalValue, CurrentTime);
          EndTask();
       }
    }
}

void UAbilityTask_PlayCurve::OnDestroy(bool bInOwnerFinished)
{
    Super::OnDestroy(bInOwnerFinished);
}