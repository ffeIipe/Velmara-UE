#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimNotifies/AnimNotify.h"
#include "AnimNotify_BossActionEvent.generated.h"

/*
 * Marca el frame exacto del evento de una acción del boss (aparecen los invocados, se rompe el techo).
 * Si el montaje lo tiene, reemplaza al EventTime normalizado configurado en el componente.
 */
UCLASS(meta = (DisplayName = "Boss Action Event"))
class TESISUE_API UAnimNotify_BossActionEvent : public UAnimNotify
{
	GENERATED_BODY()

public:
	virtual void Notify(USkeletalMeshComponent* MeshComp, UAnimSequenceBase* Animation, const FAnimNotifyEventReference& EventReference) override;

	virtual FString GetNotifyName_Implementation() const override { return TEXT("Boss Action Event"); }
};
