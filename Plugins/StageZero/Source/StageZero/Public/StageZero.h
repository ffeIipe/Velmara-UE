// Copyright (c) 2025 71 Studios for games Development. All rights reserved. 

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

#if WITH_EDITOR
class ITargetPlatform;
#endif

class FStageZeroModule : public IModuleInterface
{
public:

	/** IModuleInterface implementation */
	virtual void StartupModule() override;
	virtual void ShutdownModule() override;

private:
#if WITH_EDITOR
	void RegisterCookPackageDelegate();
	void AddStageZeroPackagesToCook(
		TConstArrayView<const ITargetPlatform*> TargetPlatforms,
		TArray<FName>& PackagesToCook,
		TArray<FName>& PackagesToNeverCook);

	FDelegateHandle ModifyCookDelegateHandle;
#endif
};
