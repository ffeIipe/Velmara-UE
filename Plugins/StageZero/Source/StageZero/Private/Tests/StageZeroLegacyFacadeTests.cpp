// Copyright (c) 2025 71 Studios. All rights reserved.

#if WITH_DEV_AUTOMATION_TESTS

#include "StageZeroLoader.h"
#include "StageZeroBlueprintLibrary.h"
#include "StageZeroLoadingWidget.h"
#include "StageZeroProjectSettings.h"
#include "StageZeroPipelineCacheObserver.h"
#include "StageZeroRuntimeShaderTracker.h"
#include "StageZeroSubsystem.h"
#include "StageZeroWarmup.h"
#include "StageZeroWarmupManifest.h"
#include "StageZeroWPAnchor.h"
#include "StageZeroWPSplineWarmup.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Misc/AutomationTest.h"
#include "Sound/SoundBase.h"
#include "UObject/Package.h"
#include "UObject/StrongObjectPtr.h"
#include "UObject/UObjectGlobals.h"
#include "UObject/UnrealType.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageZeroLegacyLoaderReflectionTest,
    "StageZero.Compatibility.LegacyLoaderReflection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageZeroLegacyLoaderReflectionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UClass* LoaderClass = AStageZeroLoader::StaticClass();
    TestNotNull(TEXT("Legacy loader class remains loadable"), LoaderClass);
    if (!LoaderClass)
    {
        return false;
    }

    const TArray<FName> RequiredFunctions = {
        TEXT("SetLevelTargets"),
        TEXT("StartStageZero"),
        TEXT("SetPrimaryLevelBP"),
        TEXT("SetWarmupMapsBP"),
        TEXT("AddWarmupMapBP"),
        TEXT("ClearWarmupMapsBP"),
        TEXT("SetWorldPartitionWarmAllCells"),
        TEXT("SetMinimumDisplaySeconds"),
        TEXT("CreateLoadingWidget"),
        TEXT("GetStageZeroProgress"),
        TEXT("GetStageZeroProgressCounts"),
        TEXT("GetShaderProgress"),
        TEXT("GetTotalProgress"),
        TEXT("ForceTransitionNow")
    };
    for (const FName FunctionName : RequiredFunctions)
    {
        TestNotNull(
            *FString::Printf(TEXT("Compatibility function %s remains reflected"), *FunctionName.ToString()),
            LoaderClass->FindFunctionByName(FunctionName));
    }

    const TArray<FName> RequiredProperties = {
        TEXT("OnStageZeroStarted"), TEXT("OnStageZeroCompleted"),
        TEXT("PrimaryLevel"), TEXT("MapsToWarmUp"), TEXT("bPreloadDependencies"),
        TEXT("LoadingIconTexture"), TEXT("BackgroundSlides"), TEXT("BackgroundSlideInterval"),
        TEXT("BackgroundSlideFadeDuration"), TEXT("bLoopBackgroundSlides"), TEXT("BootTips"),
        TEXT("BootTipInterval"), TEXT("SpinnerOnlyPhaseDuration"), TEXT("bShowSpinnerDuringProgress"),
        TEXT("MaxMapsPerWarmupBatch"), TEXT("WarmupBatchDelaySeconds"),
        TEXT("bWarmUpLevelInstances"), TEXT("bWarmUpPackedLevelActors"), TEXT("bWarmUpAllMaps"),
        TEXT("bEnableWorldPartitionWarmup"), TEXT("bWarmAllWorldPartitionCells"),
        TEXT("WarmupStreamingRadius"), TEXT("WarmupStreamingPasses"),
        TEXT("WarmupStreamingPassMaxSeconds"), TEXT("WarmupStreamingMemoryBudgetMB"),
        TEXT("bAutoStartStageZero"), TEXT("bAutoCreateLoadingWidget"), TEXT("LoadingWidgetClass"),
        TEXT("UIUpdateInterval"), TEXT("bShowCompletionMessage"), TEXT("bAllowCreatePlayerIfMissing"),
        TEXT("CompletionMessage"), TEXT("bShowTransitionMessage"), TEXT("TransitionMessage"),
        TEXT("BackgroundMusic"), TEXT("MusicFadeInTime"), TEXT("MusicFadeOutTime"),
        TEXT("MinimumDisplaySeconds"), TEXT("DelayAfterComplete"), TEXT("MaxWaitTimeSeconds"),
        TEXT("bProgressOverride"), TEXT("SimulatedProgressSpeed"), TEXT("bSkipShaderCheck"),
        TEXT("bShowBuildInfo"), TEXT("bFallbackToSimulatedProgress"),
        TEXT("bShowDebugProgressOnScreen"), TEXT("bForceShaderRecompilation"),
        TEXT("bEnableGPUWarmup"), TEXT("bAutoCollectWarmupAssets")
    };
    for (const FName PropertyName : RequiredProperties)
    {
        TestNotNull(
            *FString::Printf(TEXT("Compatibility property %s remains reflected"), *PropertyName.ToString()),
            FindFProperty<FProperty>(LoaderClass, PropertyName));
    }

    auto CountNonReturnParameters = [](const UFunction* Function)
    {
        int32 Count = 0;
        if (!Function)
        {
            return Count;
        }

        for (TFieldIterator<FProperty> It(Function); It; ++It)
        {
            if (It->HasAnyPropertyFlags(CPF_Parm) && !It->HasAnyPropertyFlags(CPF_ReturnParm))
            {
                ++Count;
            }
        }
        return Count;
    };

    UFunction* StartFunction = LoaderClass->FindFunctionByName(TEXT("StartStageZero"));
    TestNotNull(TEXT("StartStageZero signature is inspectable"), StartFunction);
    if (StartFunction)
    {
        TestEqual(TEXT("StartStageZero remains parameterless"), CountNonReturnParameters(StartFunction), 0);
        TestNull(TEXT("StartStageZero remains void"), StartFunction->GetReturnProperty());
    }

    UFunction* SetTargetsFunction = LoaderClass->FindFunctionByName(TEXT("SetLevelTargets"));
    TestNotNull(TEXT("SetLevelTargets signature is inspectable"), SetTargetsFunction);
    if (SetTargetsFunction)
    {
        TestEqual(TEXT("SetLevelTargets retains two parameters"), CountNonReturnParameters(SetTargetsFunction), 2);
        TestNull(TEXT("SetLevelTargets remains void"), SetTargetsFunction->GetReturnProperty());

        const FSoftObjectProperty* PrimaryParameter = FindFProperty<FSoftObjectProperty>(SetTargetsFunction, TEXT("InPrimary"));
        TestNotNull(TEXT("SetLevelTargets InPrimary remains a soft object parameter"), PrimaryParameter);
        if (PrimaryParameter)
        {
            TestTrue(TEXT("SetLevelTargets InPrimary remains a UWorld reference"), PrimaryParameter->PropertyClass == UWorld::StaticClass());
            TestTrue(TEXT("SetLevelTargets InPrimary remains a function parameter"), PrimaryParameter->HasAnyPropertyFlags(CPF_Parm));
        }

        const FArrayProperty* WarmupMapsParameter = FindFProperty<FArrayProperty>(SetTargetsFunction, TEXT("InWarmupMaps"));
        TestNotNull(TEXT("SetLevelTargets InWarmupMaps remains an array parameter"), WarmupMapsParameter);
        if (WarmupMapsParameter)
        {
            TestTrue(TEXT("SetLevelTargets InWarmupMaps remains a function parameter"), WarmupMapsParameter->HasAnyPropertyFlags(CPF_Parm));
            const FSoftObjectProperty* WarmupMapInner = CastField<FSoftObjectProperty>(WarmupMapsParameter->Inner);
            TestNotNull(TEXT("SetLevelTargets warmup-map entries remain soft object references"), WarmupMapInner);
            if (WarmupMapInner)
            {
                TestTrue(TEXT("SetLevelTargets warmup-map entries remain UWorld references"), WarmupMapInner->PropertyClass == UWorld::StaticClass());
            }
        }
    }

    UFunction* ProgressFunction = LoaderClass->FindFunctionByName(TEXT("GetStageZeroProgress"));
    TestNotNull(TEXT("GetStageZeroProgress signature is inspectable"), ProgressFunction);
    if (ProgressFunction)
    {
        TestEqual(TEXT("GetStageZeroProgress remains parameterless"), CountNonReturnParameters(ProgressFunction), 0);
        TestNotNull(TEXT("GetStageZeroProgress still returns float"), CastField<FFloatProperty>(ProgressFunction->GetReturnProperty()));
    }

    UFunction* CountsFunction = LoaderClass->FindFunctionByName(TEXT("GetStageZeroProgressCounts"));
    TestNotNull(TEXT("GetStageZeroProgressCounts signature is inspectable"), CountsFunction);
    if (CountsFunction)
    {
        TestEqual(TEXT("GetStageZeroProgressCounts retains two output parameters"), CountNonReturnParameters(CountsFunction), 2);
        TestNull(TEXT("GetStageZeroProgressCounts remains void"), CountsFunction->GetReturnProperty());

        const TArray<FName> CountParameterNames = { TEXT("OutCompleted"), TEXT("OutTotal") };
        for (const FName ParameterName : CountParameterNames)
        {
            const FIntProperty* Parameter = FindFProperty<FIntProperty>(CountsFunction, ParameterName);
            TestNotNull(
                *FString::Printf(TEXT("%s remains an int32 output parameter"), *ParameterName.ToString()),
                Parameter);
            if (Parameter)
            {
                TestTrue(
                    *FString::Printf(TEXT("%s remains an output parameter"), *ParameterName.ToString()),
                    Parameter->HasAllPropertyFlags(CPF_Parm | CPF_OutParm));
                TestFalse(
                    *FString::Printf(TEXT("%s is not encoded as a return value"), *ParameterName.ToString()),
                    Parameter->HasAnyPropertyFlags(CPF_ReturnParm));
            }
        }
    }

    const FSoftObjectProperty* PrimaryLevelProperty = FindFProperty<FSoftObjectProperty>(LoaderClass, TEXT("PrimaryLevel"));
    TestNotNull(TEXT("PrimaryLevel remains a soft object property"), PrimaryLevelProperty);
    if (PrimaryLevelProperty)
    {
        TestTrue(TEXT("PrimaryLevel remains typed as UWorld"), PrimaryLevelProperty->PropertyClass == UWorld::StaticClass());
    }

    const FArrayProperty* MapsToWarmUpProperty = FindFProperty<FArrayProperty>(LoaderClass, TEXT("MapsToWarmUp"));
    TestNotNull(TEXT("MapsToWarmUp remains an array property"), MapsToWarmUpProperty);
    if (MapsToWarmUpProperty)
    {
        const FSoftObjectProperty* MapInner = CastField<FSoftObjectProperty>(MapsToWarmUpProperty->Inner);
        TestNotNull(TEXT("MapsToWarmUp entries remain soft object references"), MapInner);
        if (MapInner)
        {
            TestTrue(TEXT("MapsToWarmUp entries remain typed as UWorld"), MapInner->PropertyClass == UWorld::StaticClass());
        }
    }

    const FClassProperty* LoadingWidgetClassProperty = FindFProperty<FClassProperty>(LoaderClass, TEXT("LoadingWidgetClass"));
    TestNotNull(TEXT("LoadingWidgetClass remains a class property"), LoadingWidgetClassProperty);
    if (LoadingWidgetClassProperty)
    {
        TestTrue(
            TEXT("LoadingWidgetClass remains constrained to UStageZeroLoadingWidget"),
            LoadingWidgetClassProperty->MetaClass == UStageZeroLoadingWidget::StaticClass());
    }

    const FObjectPropertyBase* BackgroundMusicProperty = FindFProperty<FObjectPropertyBase>(LoaderClass, TEXT("BackgroundMusic"));
    TestNotNull(TEXT("BackgroundMusic remains an object property"), BackgroundMusicProperty);
    if (BackgroundMusicProperty)
    {
        TestTrue(TEXT("BackgroundMusic remains typed as USoundBase"), BackgroundMusicProperty->PropertyClass == USoundBase::StaticClass());
    }

    const TArray<FName> UserFacingPropertyNames = {
        TEXT("PrimaryLevel"),
        TEXT("MapsToWarmUp"),
        TEXT("LoadingWidgetClass"),
        TEXT("bAutoStartStageZero")
    };
    for (const FName PropertyName : UserFacingPropertyNames)
    {
        const FProperty* Property = FindFProperty<FProperty>(LoaderClass, PropertyName);
        TestTrue(
            *FString::Printf(TEXT("Active property %s remains editable"), *PropertyName.ToString()),
            Property && Property->HasAnyPropertyFlags(CPF_Edit));
    }

    const TArray<FName> RetiredPropertyNames = {
        TEXT("bPreloadDependencies"),
        TEXT("MaxMapsPerWarmupBatch"),
        TEXT("WarmupBatchDelaySeconds"),
        TEXT("bWarmUpLevelInstances"),
        TEXT("bWarmUpPackedLevelActors"),
        TEXT("bEnableWorldPartitionWarmup"),
        TEXT("bWarmAllWorldPartitionCells"),
        TEXT("WarmupStreamingRadius"),
        TEXT("WarmupStreamingPasses"),
        TEXT("WarmupStreamingPassMaxSeconds"),
        TEXT("WarmupStreamingMemoryBudgetMB"),
        TEXT("bSkipShaderCheck"),
        TEXT("bShowBuildInfo"),
        TEXT("bForceShaderRecompilation"),
        TEXT("bEnableGPUWarmup")
    };
    for (const FName PropertyName : RetiredPropertyNames)
    {
        const FProperty* Property = FindFProperty<FProperty>(LoaderClass, PropertyName);
        TestNotNull(
            *FString::Printf(TEXT("Retired property %s remains serialized"), *PropertyName.ToString()),
            Property);
        if (Property)
        {
            TestFalse(
                *FString::Printf(TEXT("Retired property %s stays hidden from editing"), *PropertyName.ToString()),
                Property->HasAnyPropertyFlags(CPF_Edit));
#if WITH_METADATA
            TestTrue(
                *FString::Printf(TEXT("Retired property %s remains marked deprecated"), *PropertyName.ToString()),
                Property->HasMetaData(TEXT("DeprecatedProperty")));
            TestFalse(
                *FString::Printf(TEXT("Retired property %s explains its replacement"), *PropertyName.ToString()),
                Property->GetMetaData(TEXT("DeprecationMessage")).IsEmpty());
#endif
        }
    }

    const TArray<FName> HiddenCompatibilityPropertyNames = {
        TEXT("bShowTransitionMessage"),
        TEXT("TransitionMessage")
    };
    for (const FName PropertyName : HiddenCompatibilityPropertyNames)
    {
        const FProperty* Property = FindFProperty<FProperty>(LoaderClass, PropertyName);
        TestNotNull(
            *FString::Printf(TEXT("Legacy presentation property %s remains serialized"), *PropertyName.ToString()),
            Property);
        if (Property)
        {
            TestFalse(
                *FString::Printf(TEXT("Legacy presentation property %s stays hidden from new authoring"), *PropertyName.ToString()),
                Property->HasAnyPropertyFlags(CPF_Edit));
#if WITH_METADATA
            TestTrue(
                *FString::Printf(TEXT("Legacy presentation property %s is explicitly compatibility-only"), *PropertyName.ToString()),
                Property->HasMetaData(TEXT("DeprecatedProperty")));
#endif
        }
    }

    const TArray<FName> DeprecatedFunctionNames = {
        TEXT("StartStageZero"),
        TEXT("GetShaderProgress"),
        TEXT("ForceTransitionNow")
    };
    for (const FName FunctionName : DeprecatedFunctionNames)
    {
        const UFunction* Function = LoaderClass->FindFunctionByName(FunctionName);
        TestNotNull(
            *FString::Printf(TEXT("Deprecated function %s remains reflected"), *FunctionName.ToString()),
            Function);
#if WITH_METADATA
        if (Function)
        {
            TestTrue(
                *FString::Printf(TEXT("Function %s remains marked deprecated"), *FunctionName.ToString()),
                Function->HasMetaData(TEXT("DeprecatedFunction")));
            TestFalse(
                *FString::Printf(TEXT("Function %s explains its replacement"), *FunctionName.ToString()),
                Function->GetMetaData(TEXT("DeprecationMessage")).IsEmpty());
        }
#endif
    }

    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageZeroLegacyTransitionMessageCompatibilityTest,
    "StageZero.Compatibility.LegacyTransitionMessage",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageZeroLegacyTransitionMessageCompatibilityTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UFunction* TransitionEvent = UStageZeroLoadingWidget::StaticClass()->FindFunctionByName(TEXT("OnTransitionMessage"));
    TestNotNull(TEXT("The legacy transition presentation event remains reflected"), TransitionEvent);
    if (TransitionEvent)
    {
        TestTrue(TEXT("The transition presentation hook remains a Blueprint event"), TransitionEvent->HasAnyFunctionFlags(FUNC_BlueprintEvent));
        const FStrProperty* MessageParameter = FindFProperty<FStrProperty>(TransitionEvent, TEXT("Message"));
        TestNotNull(TEXT("The transition event retains its FString message parameter"), MessageParameter);
        if (MessageParameter)
        {
            TestTrue(TEXT("The transition message remains an input parameter"), MessageParameter->HasAnyPropertyFlags(CPF_Parm));
            TestFalse(TEXT("The transition message is not a return value"), MessageParameter->HasAnyPropertyFlags(CPF_ReturnParm));
        }
    }

    TStrongObjectPtr<UGameInstance> TestGameInstance(NewObject<UGameInstance>(GetTransientPackage()));
    UStageZeroSubsystem* Subsystem = TestGameInstance.IsValid()
        ? NewObject<UStageZeroSubsystem>(TestGameInstance.Get())
        : nullptr;
    TStrongObjectPtr<UStageZeroLoadingWidget> TestWidget(NewObject<UStageZeroLoadingWidget>(GetTransientPackage()));
    TestNotNull(TEXT("A compatibility-test subsystem can be constructed"), Subsystem);
    TestNotNull(TEXT("A typed loading widget can be constructed"), TestWidget.Get());
    if (!Subsystem || !TestWidget.IsValid())
    {
        return false;
    }

    const FName PrimaryPackage(TEXT("/Game/StageZeroTests/Primary"));
    Subsystem->PrimaryLevelOverride = TSoftObjectPtr<UWorld>(
        FSoftObjectPath(TEXT("/Game/StageZeroTests/Primary.Primary")));
    Subsystem->ConfigureLegacyTransitionMessage(true, TEXT("Preparing the primary level"));
    Subsystem->ActiveLoadingWidget = TestWidget.Get();

    TestFalse(
        TEXT("A non-primary destination cannot consume the legacy transition event"),
        Subsystem->DispatchLegacyTransitionMessage(FName(TEXT("/Game/StageZeroTests/Other"))));
    TestFalse(TEXT("A rejected destination does not trip the once-only guard"), Subsystem->bLegacyTransitionMessageDispatched);
    TestFalse(TEXT("A rejected destination does not schedule widget cleanup"), Subsystem->bLegacyTransitionWidgetAwaitingTravelCleanup);

    TestTrue(
        TEXT("An enabled serialized legacy message dispatches for the configured primary destination"),
        Subsystem->DispatchLegacyTransitionMessage(PrimaryPackage));
    TestTrue(TEXT("A successful dispatch trips the once-only guard before Blueprint execution"), Subsystem->bLegacyTransitionMessageDispatched);
    TestTrue(TEXT("A successful dispatch keeps presentation alive only until travel begins"), Subsystem->bLegacyTransitionWidgetAwaitingTravelCleanup);
    TestFalse(
        TEXT("The same transition cannot dispatch the legacy event twice"),
        Subsystem->DispatchLegacyTransitionMessage(PrimaryPackage));

    Subsystem->CancelWarmup(false);
    TestFalse(TEXT("Cancelling scheduled travel clears the pending presentation state"), Subsystem->bLegacyTransitionWidgetAwaitingTravelCleanup);
    TestFalse(TEXT("Cancelling scheduled travel releases subsystem widget tracking"), Subsystem->ActiveLoadingWidget.IsValid());
    TestFalse(TEXT("Cancelling before travel re-arms the message for a later transition attempt"), Subsystem->bLegacyTransitionMessageDispatched);

    Subsystem->ActiveLoadingWidget = TestWidget.Get();
    TestTrue(
        TEXT("A later transition attempt can dispatch after the earlier attempt was cancelled"),
        Subsystem->DispatchLegacyTransitionMessage(PrimaryPackage));
    Subsystem->ClearLegacyTransitionPresentation();

    Subsystem->ResetWarmupTrackingState();
    Subsystem->ConfigureLegacyTransitionMessage(false, TEXT("Hidden but disabled"));
    Subsystem->ActiveLoadingWidget = TestWidget.Get();
    TestFalse(
        TEXT("A serialized message remains inactive when the legacy show flag was false"),
        Subsystem->DispatchLegacyTransitionMessage(PrimaryPackage));
    TestFalse(TEXT("Disabled legacy messaging never schedules presentation cleanup"), Subsystem->bLegacyTransitionWidgetAwaitingTravelCleanup);

    Subsystem->ConfigureLegacyTransitionMessage(true, FString());
    TestFalse(
        TEXT("An empty serialized message cannot dispatch"),
        Subsystem->DispatchLegacyTransitionMessage(PrimaryPackage));

    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageZeroRetiredOptionsTruthfulnessTest,
    "StageZero.Compatibility.RetiredOptionsTruthfulness",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageZeroRetiredOptionsTruthfulnessTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UClass* SettingsClass = UStageZeroProjectSettings::StaticClass();
    TestNotNull(TEXT("Project settings class remains available"), SettingsClass);
    if (!SettingsClass)
    {
        return false;
    }

    const TArray<FName> ActiveSettings = {
        TEXT("bWarmUpLevelInstances"),
        TEXT("bWarmUpPackedLevelActors"),
        TEXT("StartupMode"),
        TEXT("CompletionPolicy"),
        TEXT("bUsePipelineCacheWarmup"),
        TEXT("PipelineCacheTimeoutSeconds"),
        TEXT("bContinueOnPipelineCacheTimeout"),
        TEXT("bTreatAssetFailuresAsFatal"),
        TEXT("bUseAsyncWarmupLoading"),
        TEXT("AssetLoadTimeoutSeconds")
    };
    for (const FName PropertyName : ActiveSettings)
    {
        const FProperty* Property = FindFProperty<FProperty>(SettingsClass, PropertyName);
        TestNotNull(*FString::Printf(TEXT("Active setting %s exists"), *PropertyName.ToString()), Property);
        TestTrue(
            *FString::Printf(TEXT("Active setting %s is editable"), *PropertyName.ToString()),
            Property && Property->HasAnyPropertyFlags(CPF_Edit));
    }

    const TArray<FName> RetiredSettings = {
        TEXT("WarmupMaxItems"),
        TEXT("bEnableMaterialRenderWarmup"),
        TEXT("bLoadMapsDuringWarmup"),
        TEXT("bEnableWarmupCacheSkip"),
        TEXT("bSkipBootScreenWhenWarmupCacheValid"),
        TEXT("bRequirePipelineCacheClearForCacheSkip")
    };
    for (const FName PropertyName : RetiredSettings)
    {
        const FProperty* Property = FindFProperty<FProperty>(SettingsClass, PropertyName);
        TestNotNull(*FString::Printf(TEXT("Retired setting %s remains serialized"), *PropertyName.ToString()), Property);
        if (Property)
        {
            TestFalse(
                *FString::Printf(TEXT("Retired setting %s is hidden"), *PropertyName.ToString()),
                Property->HasAnyPropertyFlags(CPF_Edit));
#if WITH_METADATA
            TestTrue(
                *FString::Printf(TEXT("Retired setting %s is marked deprecated"), *PropertyName.ToString()),
                Property->HasMetaData(TEXT("DeprecatedProperty")));
#endif
        }
    }

#if WITH_METADATA
    TestTrue(TEXT("Legacy warmup adapter is visibly deprecated"), UStageZeroWarmup::StaticClass()->HasMetaData(TEXT("DeprecatedNode")));
#endif

    TStrongObjectPtr<UGameInstance> TestGameInstance(NewObject<UGameInstance>(GetTransientPackage()));
    TestNotNull(TEXT("A valid subsystem outer can be constructed"), TestGameInstance.Get());
    UStageZeroSubsystem* Subsystem = TestGameInstance.IsValid()
        ? NewObject<UStageZeroSubsystem>(TestGameInstance.Get())
        : nullptr;
    TestNotNull(TEXT("Subsystem compatibility surface can be constructed"), Subsystem);
    if (Subsystem)
    {
        TestFalse(TEXT("Persistent warmup cache is permanently disabled"), Subsystem->IsWarmupCacheEnabled());
        const FStageZeroWarmupCacheStatus CacheStatus = Subsystem->GetWarmupCacheStatus(true, false);
        TestFalse(TEXT("Retired cache never permits a whole-run skip"), CacheStatus.bCanSkipWarmup);
        TestFalse(TEXT("Retired cache never claims current-process PSO state"), CacheStatus.bPipelineCacheClear);
        TestFalse(TEXT("Runtime Level Instance traversal remains disabled"), Subsystem->IncludesLevelInstances(false));
        TestFalse(TEXT("Runtime Packed Level Actor traversal remains disabled"), Subsystem->IncludesPackedLevelActors(false));
    }

    const TArray<FName> RetiredCacheFunctions = {
        TEXT("StartWarmupIfNeeded"),
        TEXT("CanSkipStageZeroWarmup"),
        TEXT("GetWarmupCacheStatus")
    };
    for (const FName FunctionName : RetiredCacheFunctions)
    {
        const UFunction* Function = UStageZeroSubsystem::StaticClass()->FindFunctionByName(FunctionName);
        TestNotNull(*FString::Printf(TEXT("Retired cache function %s remains reflected"), *FunctionName.ToString()), Function);
#if WITH_METADATA
        if (Function)
        {
            TestTrue(
                *FString::Printf(TEXT("Retired cache function %s is marked deprecated"), *FunctionName.ToString()),
                Function->HasMetaData(TEXT("DeprecatedFunction")));
            TestTrue(
                *FString::Printf(TEXT("Retired cache function %s is hidden from Blueprint palettes"), *FunctionName.ToString()),
                Function->HasMetaData(TEXT("BlueprintInternalUseOnly")));
        }
#endif
    }

    const TArray<FName> ManifestAuthoringFunctions = {
        TEXT("SetWarmUpLevelInstances"),
        TEXT("GetWarmUpLevelInstances"),
        TEXT("SetWarmUpPackedLevelActors"),
        TEXT("GetWarmUpPackedLevelActors")
    };
    for (const FName FunctionName : ManifestAuthoringFunctions)
    {
        const UFunction* Function = UStageZeroBlueprintLibrary::StaticClass()->FindFunctionByName(FunctionName);
        TestNotNull(*FString::Printf(TEXT("Manifest authoring function %s remains reflected"), *FunctionName.ToString()), Function);
#if WITH_METADATA
        if (Function)
        {
            TestFalse(
                *FString::Printf(TEXT("Manifest authoring function %s is active"), *FunctionName.ToString()),
                Function->HasMetaData(TEXT("DeprecatedFunction")));
            TestFalse(
                *FString::Printf(TEXT("Manifest authoring function %s is visible to Blueprint users"), *FunctionName.ToString()),
                Function->HasMetaData(TEXT("BlueprintInternalUseOnly")));
        }
#endif
    }

    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageZeroRuntimeSettingsPolicyTest,
    "StageZero.Runtime.SettingsPolicy",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageZeroRuntimeSettingsPolicyTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UStageZeroProjectSettings* Settings = GetMutableDefault<UStageZeroProjectSettings>();
    TStrongObjectPtr<UGameInstance> TestGameInstance(NewObject<UGameInstance>(GetTransientPackage()));
    UStageZeroSubsystem* Subsystem = TestGameInstance.IsValid()
        ? NewObject<UStageZeroSubsystem>(TestGameInstance.Get())
        : nullptr;
    TestNotNull(TEXT("Mutable Stage Zero settings are available"), Settings);
    TestNotNull(TEXT("Settings-policy subsystem can be constructed"), Subsystem);
    if (!Settings || !Subsystem)
    {
        return false;
    }

    const EStageZeroCompletionPolicy PreviousCompletionPolicy = Settings->CompletionPolicy;
    const bool bPreviousContinueOnPipelineTimeout = Settings->bContinueOnPipelineCacheTimeout;
    const bool bPreviousTreatAssetFailuresAsFatal = Settings->bTreatAssetFailuresAsFatal;
    const bool bPreviousUseAsyncLoading = Settings->bUseAsyncWarmupLoading;
    const bool bPreviousUsePipelineCache = Settings->bUsePipelineCacheWarmup;

    Settings->CompletionPolicy = EStageZeroCompletionPolicy::Strict;
    Settings->bTreatAssetFailuresAsFatal = true;
    TestTrue(TEXT("Strict mode can make manifest asset failures fatal"), Subsystem->ShouldTreatAssetFailuresAsFatal());
    Settings->bTreatAssetFailuresAsFatal = false;
    TestFalse(TEXT("Strict mode honors the non-fatal asset-failure override"), Subsystem->ShouldTreatAssetFailuresAsFatal());
    Settings->CompletionPolicy = EStageZeroCompletionPolicy::PartialOk;
    Settings->bTreatAssetFailuresAsFatal = true;
    TestFalse(TEXT("Best Effort mode never converts an asset failure into a fatal result"), Subsystem->ShouldTreatAssetFailuresAsFatal());

    Settings->CompletionPolicy = EStageZeroCompletionPolicy::Strict;
    Settings->bContinueOnPipelineCacheTimeout = false;
    TestFalse(TEXT("Strict mode can block on a recorded PSO-cache timeout"), Subsystem->ShouldContinueOnPipelineCacheTimeout());
    Settings->bContinueOnPipelineCacheTimeout = true;
    TestTrue(TEXT("Strict mode honors the continue-on-PSO-timeout override"), Subsystem->ShouldContinueOnPipelineCacheTimeout());
    Settings->CompletionPolicy = EStageZeroCompletionPolicy::PartialOk;
    Settings->bContinueOnPipelineCacheTimeout = false;
    TestTrue(TEXT("Best Effort mode continues after a recorded PSO-cache timeout"), Subsystem->ShouldContinueOnPipelineCacheTimeout());

    Settings->bUseAsyncWarmupLoading = false;
    TestFalse(TEXT("Synchronous manifest loading can be selected"), Subsystem->ShouldUseAsyncWarmupLoading());
    Settings->bUseAsyncWarmupLoading = true;
    TestTrue(TEXT("Asynchronous manifest loading can be selected"), Subsystem->ShouldUseAsyncWarmupLoading());

    Settings->bUsePipelineCacheWarmup = false;
    TestFalse(TEXT("Recorded PSO-cache warmup can be disabled"), Subsystem->ShouldUsePipelineCacheWarmup());
    Settings->bUsePipelineCacheWarmup = true;
    TestTrue(TEXT("Recorded PSO-cache warmup can be enabled"), Subsystem->ShouldUsePipelineCacheWarmup());

    Settings->CompletionPolicy = PreviousCompletionPolicy;
    Settings->bContinueOnPipelineCacheTimeout = bPreviousContinueOnPipelineTimeout;
    Settings->bTreatAssetFailuresAsFatal = bPreviousTreatAssetFailuresAsFatal;
    Settings->bUseAsyncWarmupLoading = bPreviousUseAsyncLoading;
    Settings->bUsePipelineCacheWarmup = bPreviousUsePipelineCache;

    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageZeroPerMapLifecycleAccountingTest,
    "StageZero.Runtime.PerMapLifecycleAccounting",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageZeroPerMapLifecycleAccountingTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    TStrongObjectPtr<UGameInstance> TestGameInstance(NewObject<UGameInstance>(GetTransientPackage()));
    UStageZeroSubsystem* Subsystem = TestGameInstance.IsValid()
        ? NewObject<UStageZeroSubsystem>(TestGameInstance.Get())
        : nullptr;
    TestNotNull(TEXT("Per-map accounting subsystem can be constructed"), Subsystem);
    if (!Subsystem)
    {
        return false;
    }

    const FName MapAName(TEXT("/Game/StageZeroTests/Map_A"));
    const FName MapBName(TEXT("/Game/StageZeroTests/Map_B"));
    const FName EmptyMapName(TEXT("/Game/StageZeroTests/Map_Empty"));
    const FSoftObjectPath SharedPath(TEXT("/Game/StageZeroTests/T_Shared.T_Shared"));
    const FSoftObjectPath AOnlyPath(TEXT("/Game/StageZeroTests/M_AOnly.M_AOnly"));
    const FSoftObjectPath BTimedOutPath(TEXT("/Game/StageZeroTests/T_BTimedOut.T_BTimedOut"));
    const FSoftObjectPath BCancelledPath(TEXT("/Game/StageZeroTests/T_BCancelled.T_BCancelled"));
    const FSoftObjectPath InvalidPath;

    TArray<FStageZeroManifestMapCoverage> CoveragePlan;
    FStageZeroManifestMapCoverage& MapA = CoveragePlan.AddDefaulted_GetRef();
    MapA.MapName = MapAName;
    MapA.RequiredPaths.Add(SharedPath);
    MapA.RequiredPaths.Add(AOnlyPath);
    MapA.RequiredPaths.Add(InvalidPath);

    FStageZeroManifestMapCoverage& MapB = CoveragePlan.AddDefaulted_GetRef();
    MapB.MapName = MapBName;
    MapB.RequiredPaths.Add(SharedPath);
    MapB.RequiredPaths.Add(BTimedOutPath);
    MapB.RequiredPaths.Add(BCancelledPath);
    MapB.RequiredPaths.Add(InvalidPath);

    FStageZeroManifestMapCoverage& EmptyMap = CoveragePlan.AddDefaulted_GetRef();
    EmptyMap.MapName = EmptyMapName;

    TSet<FSoftObjectPath> AggregatePaths;
    AggregatePaths.Add(SharedPath);
    AggregatePaths.Add(AOnlyPath);
    AggregatePaths.Add(BTimedOutPath);
    AggregatePaths.Add(BCancelledPath);
    AggregatePaths.Add(InvalidPath);
    TestEqual(TEXT("Aggregate execution plan contains one physical entry per unique path"), AggregatePaths.Num(), 5);

    FString FailureReason;
    const bool bCoverageInitialized = Subsystem->InitializeWarmupMapCoverage(
        CoveragePlan,
        FName(TEXT("AllMaps")),
        AggregatePaths,
        FailureReason);
    TestTrue(
        TEXT("Consistent logical map coverage is accepted"),
        bCoverageInitialized);
    TestTrue(TEXT("Accepted coverage has no failure reason"), FailureReason.IsEmpty());
    TestEqual(TEXT("All authored map groups are retained"), Subsystem->ActiveWarmupMapCoverage.Num(), 3);
    TestEqual(TEXT("Total map count uses logical groups"), Subsystem->TargetMapCount, 3);
    if (!bCoverageInitialized || Subsystem->ActiveWarmupMapCoverage.Num() != 3)
    {
        return false;
    }

    const TArray<int32>* SharedMembership = Subsystem->ManifestPathMapCoverageMembership.Find(SharedPath);
    TestNotNull(TEXT("Shared path has reverse map membership"), SharedMembership);
    if (SharedMembership)
    {
        TestEqual(TEXT("Shared path still has one physical key but two logical owners"), SharedMembership->Num(), 2);
        TestTrue(TEXT("Shared path belongs to map A"), SharedMembership->Contains(0));
        TestTrue(TEXT("Shared path belongs to map B"), SharedMembership->Contains(1));
    }

    Subsystem->BroadcastWarmupMapCoverageStarts();
    TestTrue(TEXT("Map A received its logical start"), Subsystem->ActiveWarmupMapCoverage[0].bStarted);
    TestTrue(TEXT("Map B received its logical start"), Subsystem->ActiveWarmupMapCoverage[1].bStarted);
    TestTrue(TEXT("Empty map received its logical start"), Subsystem->ActiveWarmupMapCoverage[2].bStarted);
    TestTrue(TEXT("Empty map finishes immediately after starts are emitted"), Subsystem->ActiveWarmupMapCoverage[2].bFinished);
    TestEqual(TEXT("Only the empty map is initially complete"), Subsystem->CompletedMaps, 1);
    Subsystem->BroadcastWarmupMapCoverageStarts();
    TestEqual(TEXT("Repeated start dispatch cannot finish an empty map twice"), Subsystem->CompletedMaps, 1);

    Subsystem->TrackWarmupJobRequested(InvalidPath, TEXT("Texture"), true);
    Subsystem->TrackWarmupJobFinished(
        InvalidPath,
        EStageZeroWarmupJobTerminalState::Failed,
        TEXT("Invalid manifest path."));
    Subsystem->TrackWarmupJobRequested(SharedPath, TEXT("Texture"), true);
    Subsystem->TrackWarmupJobFinished(SharedPath, EStageZeroWarmupJobTerminalState::Succeeded);
    TestFalse(TEXT("Shared and invalid paths alone do not finish map A"), Subsystem->ActiveWarmupMapCoverage[0].bFinished);
    TestFalse(TEXT("Shared and invalid paths alone do not finish map B"), Subsystem->ActiveWarmupMapCoverage[1].bFinished);

    const int32 SharedTerminalCount = Subsystem->ActiveWarmupMapCoverage[0].ProcessedTerminalPaths.Num();
    Subsystem->TrackWarmupJobFinished(
        SharedPath,
        EStageZeroWarmupJobTerminalState::Failed,
        TEXT("Duplicate terminal callback must be ignored."));
    TestEqual(
        TEXT("Duplicate terminal callbacks do not duplicate map accounting"),
        Subsystem->ActiveWarmupMapCoverage[0].ProcessedTerminalPaths.Num(),
        SharedTerminalCount);
    TestEqual(TEXT("Duplicate terminal callbacks do not increment completed maps"), Subsystem->CompletedMaps, 1);

    Subsystem->TrackWarmupJobRequested(AOnlyPath, TEXT("Material"), true);
    Subsystem->TrackWarmupJobFinished(
        AOnlyPath,
        EStageZeroWarmupJobTerminalState::Failed,
        TEXT("A processed asset failure is still a terminal map item."));
    TestTrue(TEXT("Map A finishes after each unique authored path is processed"), Subsystem->ActiveWarmupMapCoverage[0].bFinished);
    TestFalse(TEXT("Map B remains pending"), Subsystem->ActiveWarmupMapCoverage[1].bFinished);
    TestEqual(TEXT("Empty map and map A are complete"), Subsystem->CompletedMaps, 2);

    Subsystem->TrackWarmupJobRequested(BTimedOutPath, TEXT("Texture"), true);
    Subsystem->TrackWarmupJobLoading(BTimedOutPath);
    Subsystem->TrackWarmupJobFinished(
        BTimedOutPath,
        EStageZeroWarmupJobTerminalState::TimedOut,
        TEXT("Timed out before the asset was processed."));
    Subsystem->TrackWarmupJobRequested(BCancelledPath, TEXT("Texture"), true);
    Subsystem->TrackWarmupJobLoading(BCancelledPath);
    Subsystem->MarkOutstandingManifestJobsCancelled(TEXT("Test cancellation."));
    TestFalse(TEXT("A timed-out required path does not finish map B"), Subsystem->ActiveWarmupMapCoverage[1].bFinished);
    TestFalse(
        TEXT("A cancelled required path is not counted as processed map coverage"),
        Subsystem->ActiveWarmupMapCoverage[1].ProcessedTerminalPaths.Contains(BCancelledPath));
    TestEqual(TEXT("Cancellation cannot blanket-finish pending maps"), Subsystem->CompletedMaps, 2);

    Subsystem->ResetWarmupTrackingState();
    const FName TimeoutMapName(TEXT("/Game/StageZeroTests/Map_Timeout"));
    const FSoftObjectPath DeadlinePath(TEXT("/Game/StageZeroTests/T_Deadline.T_Deadline"));
    TArray<FStageZeroManifestMapCoverage> TimeoutPlan;
    FStageZeroManifestMapCoverage& TimeoutMap = TimeoutPlan.AddDefaulted_GetRef();
    TimeoutMap.MapName = TimeoutMapName;
    TimeoutMap.RequiredPaths.Add(DeadlinePath);
    TSet<FSoftObjectPath> TimeoutAggregate;
    TimeoutAggregate.Add(DeadlinePath);
    const bool bTimeoutCoverageInitialized = Subsystem->InitializeWarmupMapCoverage(
        TimeoutPlan,
        TimeoutMapName,
        TimeoutAggregate,
        FailureReason);
    TestTrue(
        TEXT("Deadline coverage plan is accepted"),
        bTimeoutCoverageInitialized);
    if (!bTimeoutCoverageInitialized || Subsystem->ActiveWarmupMapCoverage.Num() != 1)
    {
        return false;
    }
    Subsystem->BroadcastWarmupMapCoverageStarts();
    Subsystem->TrackWarmupJobRequested(DeadlinePath, TEXT("Texture"), true);
    Subsystem->TrackWarmupJobLoading(DeadlinePath);
    Subsystem->ManifestTaskTotal = 1;
    Subsystem->CurrentMapTaskTotal = 1;
    Subsystem->TotalTasks = 1;
    Subsystem->MarkOutstandingManifestJobsTimedOut(TEXT("Test wall-clock deadline."));
    TestFalse(TEXT("A whole-run deadline does not finish pending map coverage"), Subsystem->ActiveWarmupMapCoverage[0].bFinished);
    TestEqual(TEXT("A whole-run deadline leaves completed maps truthful"), Subsystem->CompletedMaps, 0);

    TArray<FStageZeroManifestMapCoverage> InconsistentPlan;
    FStageZeroManifestMapCoverage& InconsistentMap = InconsistentPlan.AddDefaulted_GetRef();
    InconsistentMap.MapName = TEXT("/Game/StageZeroTests/Map_Inconsistent");
    const FSoftObjectPath MissingFromAggregatePath(TEXT("/Game/StageZeroTests/T_MissingFromAggregate.T_MissingFromAggregate"));
    InconsistentMap.RequiredPaths.Add(MissingFromAggregatePath);
    TestFalse(
        TEXT("Coverage absent from the aggregate plan is rejected atomically"),
        Subsystem->InitializeWarmupMapCoverage(InconsistentPlan, FName(TEXT("AllMaps")), TimeoutAggregate, FailureReason));
    TestTrue(TEXT("Inconsistent coverage failure is explicit"), FailureReason.Contains(TEXT("absent from the aggregate load plan")));
    TestEqual(TEXT("Rejected coverage exposes no partially initialized map state"), Subsystem->ActiveWarmupMapCoverage.Num(), 0);
    TestEqual(TEXT("Rejected coverage cannot report a completed map"), Subsystem->CompletedMaps, 0);

    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageZeroWorldPartitionManifestSelectionTest,
    "StageZero.WorldPartition.ManifestSubsetSelection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageZeroWorldPartitionManifestSelectionTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UClass* AnchorClass = AStageZeroWPAnchor::StaticClass();
    UClass* SplineClass = AStageZeroWPSplineWarmup::StaticClass();
    TestNotNull(TEXT("World Partition anchor authoring actor remains available"), AnchorClass);
    TestNotNull(TEXT("World Partition spline authoring actor remains available"), SplineClass);
    if (AnchorClass)
    {
        TestFalse(TEXT("World Partition anchor actor remains placeable"), AnchorClass->HasAnyClassFlags(CLASS_NotPlaceable));
#if WITH_METADATA
        TestFalse(TEXT("World Partition anchor actor is not deprecated"), AnchorClass->HasMetaData(TEXT("DeprecatedNode")));
#endif
    }
    if (SplineClass)
    {
        TestFalse(TEXT("World Partition spline actor remains placeable"), SplineClass->HasAnyClassFlags(CLASS_NotPlaceable));
#if WITH_METADATA
        TestFalse(TEXT("World Partition spline actor is not deprecated"), SplineClass->HasMetaData(TEXT("DeprecatedNode")));
#endif
    }

    const TArray<FName> WorldPartitionFunctions = {
        TEXT("WarmUpWorldPartitionCells"),
        TEXT("WarmUpWorldPartitionAnchors"),
        TEXT("WarmUpWorldPartitionSplineRoutes")
    };
    for (const FName FunctionName : WorldPartitionFunctions)
    {
        const UFunction* Function = UStageZeroSubsystem::StaticClass()->FindFunctionByName(FunctionName);
        TestNotNull(*FString::Printf(TEXT("World Partition function %s remains reflected"), *FunctionName.ToString()), Function);
#if WITH_METADATA
        if (Function)
        {
            TestFalse(
                *FString::Printf(TEXT("World Partition function %s is active"), *FunctionName.ToString()),
                Function->HasMetaData(TEXT("DeprecatedFunction")));
            TestFalse(
                *FString::Printf(TEXT("World Partition function %s remains visible to Blueprint users"), *FunctionName.ToString()),
                Function->HasMetaData(TEXT("BlueprintInternalUseOnly")));
        }
#endif
    }

    UClass* ManifestClass = UStageZeroWarmupManifest::StaticClass();
    const TArray<FName> ActiveManifestArrays = {
        TEXT("WPCells"),
        TEXT("WPAnchorGroups"),
        TEXT("WPSplineGroups")
    };
    for (const FName PropertyName : ActiveManifestArrays)
    {
        const FArrayProperty* Property = FindFProperty<FArrayProperty>(ManifestClass, PropertyName);
        TestNotNull(*FString::Printf(TEXT("Manifest array %s remains reflected"), *PropertyName.ToString()), Property);
#if WITH_METADATA
        if (Property)
        {
            TestFalse(
                *FString::Printf(TEXT("Manifest array %s is active data"), *PropertyName.ToString()),
                Property->HasMetaData(TEXT("DeprecatedProperty")));
        }
#endif
    }

    TStrongObjectPtr<UGameInstance> TestGameInstance(NewObject<UGameInstance>(GetTransientPackage()));
    UStageZeroSubsystem* Subsystem = TestGameInstance.IsValid()
        ? NewObject<UStageZeroSubsystem>(TestGameInstance.Get())
        : nullptr;
    TStrongObjectPtr<UStageZeroWarmupManifest> Manifest(NewObject<UStageZeroWarmupManifest>(GetTransientPackage()));
    TestNotNull(TEXT("World Partition selection subsystem can be constructed"), Subsystem);
    TestTrue(TEXT("World Partition selection manifest can be constructed"), Manifest.IsValid());
    if (!Subsystem || !Manifest.IsValid())
    {
        return false;
    }

    const FName MapPackage(TEXT("/Game/StageZeroTests/WP_Map"));
    const FName FallbackMapPackage(TEXT("/Game/StageZeroTests/WP_Fallback"));
    const FSoftObjectPath CellMaterial(TEXT("/Game/StageZeroTests/M_Cell.M_Cell"));
    const FSoftObjectPath FallbackTexture(TEXT("/Game/StageZeroTests/T_Fallback.T_Fallback"));
    const FSoftObjectPath AnchorMaterial(TEXT("/Game/StageZeroTests/M_Anchor.M_Anchor"));
    const FSoftObjectPath AnchorTexture(TEXT("/Game/StageZeroTests/T_Anchor.T_Anchor"));
    const FSoftObjectPath RouteNiagara(TEXT("/Game/StageZeroTests/NS_Route.NS_Route"));
    const FSoftObjectPath RouteMesh(TEXT("/Game/StageZeroTests/SM_Route.SM_Route"));

    FStageZeroWPCellEntry& Cell = Manifest->WPCells.AddDefaulted_GetRef();
    Cell.MapPackageName = MapPackage;
    Cell.CellId = TEXT("Cell_A");
    Cell.Materials.Add(CellMaterial);

    FStageZeroMapMatTexEntry& FallbackMap = Manifest->MapEntries.AddDefaulted_GetRef();
    FallbackMap.MapPackageName = FallbackMapPackage;
    FallbackMap.Textures.Add(FallbackTexture);

    FStageZeroWPAnchorGroup& DowntownAnchor = Manifest->WPAnchorGroups.AddDefaulted_GetRef();
    DowntownAnchor.MapPackageName = MapPackage;
    DowntownAnchor.GroupName = TEXT("Downtown");
    DowntownAnchor.AnchorId = TEXT("/Game/StageZeroTests/WP_Map.WP_Map:PersistentLevel.Anchor_Downtown");
    DowntownAnchor.Materials.Add(AnchorMaterial);

    FStageZeroWPAnchorGroup& TransitAnchor = Manifest->WPAnchorGroups.AddDefaulted_GetRef();
    TransitAnchor.MapPackageName = MapPackage;
    TransitAnchor.GroupName = TEXT("Transit");
    TransitAnchor.AnchorId = TEXT("/Game/StageZeroTests/WP_Map.WP_Map:PersistentLevel.Anchor_Transit");
    TransitAnchor.Textures.Add(AnchorTexture);

    FStageZeroWPSplineGroup& MainRoute = Manifest->WPSplineGroups.AddDefaulted_GetRef();
    MainRoute.MapPackageName = MapPackage;
    MainRoute.GroupName = TEXT("MainRoute");
    MainRoute.RouteId = TEXT("/Game/StageZeroTests/WP_Map.WP_Map:PersistentLevel.Route_Main");
    MainRoute.NiagaraSystems.Add(RouteNiagara);

    FStageZeroWPSplineGroup& ServiceRoute = Manifest->WPSplineGroups.AddDefaulted_GetRef();
    ServiceRoute.MapPackageName = MapPackage;
    ServiceRoute.GroupName = TEXT("ServiceRoute");
    ServiceRoute.RouteId = TEXT("/Game/StageZeroTests/WP_Map.WP_Map:PersistentLevel.Route_Service");
    ServiceRoute.StaticMeshes.Add(RouteMesh);

    TArray<FSoftObjectPath> Materials;
    TArray<FSoftObjectPath> Textures;
    TArray<FSoftObjectPath> Niagara;
    TArray<FSoftObjectPath> StaticMeshes;
    TArray<FSoftObjectPath> SkeletalMeshes;
    FString FailureReason;
    int32 SelectedCount = 0;
    bool bUsedMapFallback = false;

    TestTrue(
        TEXT("Cell selection uses baked cell data"),
        Subsystem->BuildAssetsFromWPCells(*Manifest, MapPackage, Materials, Textures, Niagara, StaticMeshes, SkeletalMeshes, SelectedCount, bUsedMapFallback, FailureReason));
    TestEqual(TEXT("Cell selection reports the selected cell count"), SelectedCount, 1);
    TestFalse(TEXT("Cell selection does not broaden to the map fallback when cells exist"), bUsedMapFallback);
    TestTrue(TEXT("Cell selection returns the baked cell material"), Materials.Contains(CellMaterial));

    TestTrue(
        TEXT("Unfiltered cell selection supports map-level data from an older lightweight manifest"),
        Subsystem->BuildAssetsFromWPCells(*Manifest, FallbackMapPackage, Materials, Textures, Niagara, StaticMeshes, SkeletalMeshes, SelectedCount, bUsedMapFallback, FailureReason));
    TestTrue(TEXT("Cell compatibility fallback is reported"), bUsedMapFallback);
    TestEqual(TEXT("Cell compatibility fallback does not invent cell records"), SelectedCount, 0);
    TestTrue(TEXT("Cell compatibility fallback returns the map's baked texture"), Textures.Contains(FallbackTexture));

    TestFalse(
        TEXT("Cell selection fails when neither cell data nor a map entry exists"),
        Subsystem->BuildAssetsFromWPCells(*Manifest, TEXT("/Game/StageZeroTests/WP_Missing"), Materials, Textures, Niagara, StaticMeshes, SkeletalMeshes, SelectedCount, bUsedMapFallback, FailureReason));
    TestEqual(TEXT("Failed cell selection exposes no assets"), Materials.Num() + Textures.Num() + Niagara.Num() + StaticMeshes.Num() + SkeletalMeshes.Num(), 0);
    TestFalse(TEXT("Failed cell selection does not claim a map fallback"), bUsedMapFallback);
    TestTrue(TEXT("Failed cell selection explains that manifest data is missing"), FailureReason.Contains(TEXT("No baked World Partition cell data")));

    const FName TransitAnchorId(*TransitAnchor.AnchorId);
    TestTrue(
        TEXT("Anchor selectors match both group names and stable anchor ids"),
        Subsystem->BuildAssetsFromWPAnchors(*Manifest, MapPackage, { TEXT("Downtown"), TransitAnchorId }, Materials, Textures, Niagara, StaticMeshes, SkeletalMeshes, SelectedCount, FailureReason));
    TestEqual(TEXT("Anchor selection includes both matched groups"), SelectedCount, 2);
    TestTrue(TEXT("Anchor group selection returns the baked material"), Materials.Contains(AnchorMaterial));
    TestTrue(TEXT("Anchor id selection returns the baked texture"), Textures.Contains(AnchorTexture));

    TestFalse(
        TEXT("A missing requested anchor fails the entire selection"),
        Subsystem->BuildAssetsFromWPAnchors(*Manifest, MapPackage, { TEXT("Downtown"), TEXT("MissingAnchor") }, Materials, Textures, Niagara, StaticMeshes, SkeletalMeshes, SelectedCount, FailureReason));
    TestEqual(TEXT("Failed anchor selection exposes no partial groups"), SelectedCount, 0);
    TestEqual(TEXT("Failed anchor selection exposes no partial assets"), Materials.Num() + Textures.Num() + Niagara.Num() + StaticMeshes.Num() + SkeletalMeshes.Num(), 0);
    TestTrue(TEXT("Missing anchor failure identifies the selector"), FailureReason.Contains(TEXT("MissingAnchor")));
    TestTrue(TEXT("Missing anchor failure confirms that no whole-map fallback occurred"), FailureReason.Contains(TEXT("No whole-map fallback")));

    TestTrue(
        TEXT("An empty anchor filter selects all baked groups for the map"),
        Subsystem->BuildAssetsFromWPAnchors(*Manifest, MapPackage, {}, Materials, Textures, Niagara, StaticMeshes, SkeletalMeshes, SelectedCount, FailureReason));
    TestEqual(TEXT("Unfiltered anchor selection includes every map anchor"), SelectedCount, 2);

    const FName ServiceRouteId(*ServiceRoute.RouteId);
    TestTrue(
        TEXT("Spline selectors match both group names and stable route ids"),
        Subsystem->BuildAssetsFromWPSplines(*Manifest, MapPackage, { TEXT("MainRoute"), ServiceRouteId }, Materials, Textures, Niagara, StaticMeshes, SkeletalMeshes, SelectedCount, FailureReason));
    TestEqual(TEXT("Spline selection includes both matched routes"), SelectedCount, 2);
    TestTrue(TEXT("Spline group selection returns the baked Niagara system"), Niagara.Contains(RouteNiagara));
    TestTrue(TEXT("Spline id selection returns the baked static mesh"), StaticMeshes.Contains(RouteMesh));

    TestFalse(
        TEXT("A missing requested spline route fails the entire selection"),
        Subsystem->BuildAssetsFromWPSplines(*Manifest, MapPackage, { TEXT("MainRoute"), TEXT("MissingRoute") }, Materials, Textures, Niagara, StaticMeshes, SkeletalMeshes, SelectedCount, FailureReason));
    TestEqual(TEXT("Failed spline selection exposes no partial routes"), SelectedCount, 0);
    TestEqual(TEXT("Failed spline selection exposes no partial assets"), Materials.Num() + Textures.Num() + Niagara.Num() + StaticMeshes.Num() + SkeletalMeshes.Num(), 0);
    TestTrue(TEXT("Missing spline failure identifies the selector"), FailureReason.Contains(TEXT("MissingRoute")));
    TestTrue(TEXT("Missing spline failure confirms that no whole-map fallback occurred"), FailureReason.Contains(TEXT("No whole-map fallback")));

    TestTrue(
        TEXT("An empty route filter selects all baked routes for the map"),
        Subsystem->BuildAssetsFromWPSplines(*Manifest, MapPackage, {}, Materials, Textures, Niagara, StaticMeshes, SkeletalMeshes, SelectedCount, FailureReason));
    TestEqual(TEXT("Unfiltered spline selection includes every map route"), SelectedCount, 2);

    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageZeroRetiredTrackerTruthfulnessTest,
    "StageZero.Compatibility.RetiredTrackerTruthfulness",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageZeroRetiredTrackerTruthfulnessTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    UClass* TrackerClass = UStageZeroRuntimeShaderTracker::StaticClass();
    TestNotNull(TEXT("Retired tracker class remains loadable"), TrackerClass);
    if (!TrackerClass)
    {
        return false;
    }

#if WITH_METADATA
    TestTrue(TEXT("Retired tracker class is visibly deprecated"), TrackerClass->HasMetaData(TEXT("DeprecatedNode")));
    TestFalse(
        TEXT("Retired tracker class provides migration guidance"),
        TrackerClass->GetMetaData(TEXT("DeprecationMessage")).IsEmpty());
#endif

    UStageZeroRuntimeShaderTracker* Tracker = NewObject<UStageZeroRuntimeShaderTracker>(GetTransientPackage());
    TestNotNull(TEXT("Retired tracker compatibility shell can still be constructed"), Tracker);
    if (!Tracker)
    {
        return false;
    }

    TestFalse(TEXT("Unattached tracker facade reports that no subsystem is available"), Tracker->IsAvailable());
    TestFalse(TEXT("Unattached tracker facade never reports shader readiness"), Tracker->IsComplete());
    TestEqual(TEXT("Unattached tracker facade reports zero progress"), Tracker->GetProgress(), 0.0f);

    int32 Completed = -1;
    int32 Total = -1;
    Tracker->GetJobCounts(Completed, Total);
    TestEqual(TEXT("Unattached tracker facade reports zero completed jobs"), Completed, 0);
    TestEqual(TEXT("Unattached tracker facade reports zero total jobs"), Total, 0);
    TestTrue(
        TEXT("Unattached tracker facade status is explicit"),
        Tracker->GetDebugStatus().Contains(TEXT("no Stage Zero subsystem"), ESearchCase::IgnoreCase));

    Tracker->Tick(1.0f / 60.0f);
    TestFalse(TEXT("Tick cannot create readiness without an attached authoritative subsystem"), Tracker->IsComplete());
    Tracker->Shutdown();

    return !HasAnyErrors();
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageZeroPipelineCacheIdentityAssessmentTest,
    "StageZero.PipelineCache.CacheIdentityAssessment",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageZeroPipelineCacheIdentityAssessmentTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FString ProjectKey(TEXT("StageZeroIdentityProject"));
    const FString UserKey(TEXT("StageZeroIdentityProject_usr"));
    FStageZeroPipelineCacheObservation Observation;

    FStageZeroPipelineCacheRecordObservation& Unrelated = Observation.CacheRecords.AddDefaulted_GetRef();
    Unrelated.CacheKey = TEXT("UnrelatedPluginCache");
    Unrelated.Generation = 1;
    Unrelated.bCompleted = true;

    FStageZeroPipelineCacheRecordObservation& Project = Observation.CacheRecords.AddDefaulted_GetRef();
    Project.CacheKey = ProjectKey;
    Project.Generation = 2;
    Project.OpenedPSOs = 17;
    Project.ExpectedPSOs = 17;
    Project.bActive = true;

    TSet<FString> RequiredKeys;
    RequiredKeys.Add(ProjectKey);
    FStageZeroPipelineCacheKeyAssessment Assessment = StageZeroPipelineCacheObserver::AssessSnapshotForCacheKeys(
        Observation, ProjectKey, UserKey, RequiredKeys, 1);
    TestTrue(TEXT("Expected project cache is recognized"), Assessment.bHasRelevantRecord);
    TestTrue(TEXT("Expected active cache remains pending"), Assessment.bHasActiveRelevantRecord);
    TestTrue(TEXT("New required project record is observed"), Assessment.bAllRequiredRecordsObserved);
    TestFalse(TEXT("Active required project record is not complete"), Assessment.bAllRequiredRecordsCompleted);
    TestEqual(TEXT("Unrelated completion cannot replace expected cache counts"), Assessment.LatestOpenedPSOs, 17u);

    Project.bActive = false;
    Project.bCompleted = true;
    Project.CompletedPSOs = 17;
    const int32 ProjectChunkIndex = Observation.CacheRecords.AddDefaulted();
    FStageZeroPipelineCacheRecordObservation& ProjectChunk = Observation.CacheRecords[ProjectChunkIndex];
    ProjectChunk.CacheKey = ProjectKey + TEXT("_Chunk3");
    ProjectChunk.Generation = 3;
    ProjectChunk.OpenedPSOs = 5;
    ProjectChunk.CompletedPSOs = 5;
    ProjectChunk.bCompleted = true;
    const int32 UserIndex = Observation.CacheRecords.AddDefaulted();
    FStageZeroPipelineCacheRecordObservation& User = Observation.CacheRecords[UserIndex];
    User.CacheKey = UserKey;
    User.Generation = 4;
    User.OpenedPSOs = 2;
    User.CompletedPSOs = 2;
    User.bCompleted = true;
    RequiredKeys.Add(UserKey);

    Assessment = StageZeroPipelineCacheObserver::AssessSnapshotForCacheKeys(
        Observation, ProjectKey, UserKey, RequiredKeys, 1);
    TestTrue(TEXT("Project chunk keys are part of the expected project identity"), Assessment.bHasCompletedRelevantRecord);
    TestTrue(TEXT("Every required post-baseline cache record is observed"), Assessment.bAllRequiredRecordsObserved);
    TestTrue(TEXT("Every expected project/user record is complete"), Assessment.bAllRequiredRecordsCompleted);
    TestTrue(TEXT("Unrelated cache state does not affect expected-cache completion"), Assessment.bAllRelevantRecordsCompleted);
    TestEqual(TEXT("Latest expected cache supplies diagnostics"), Assessment.LatestCompletedPSOs, 2u);

    Observation.CacheRecords[UserIndex].Generation = 1;
    Assessment = StageZeroPipelineCacheObserver::AssessSnapshotForCacheKeys(
        Observation, ProjectKey, UserKey, RequiredKeys, 1);
    TestFalse(TEXT("A stale pre-baseline record cannot satisfy a newly opened required cache"), Assessment.bAllRequiredRecordsObserved);

    Observation.CacheRecords[UserIndex].Generation = 4;
    Observation.CacheRecords[ProjectChunkIndex].bIncomplete = true;
    Observation.CacheRecords[ProjectChunkIndex].bCompleted = false;
    Assessment = StageZeroPipelineCacheObserver::AssessSnapshotForCacheKeys(
        Observation, ProjectKey, UserKey, RequiredKeys, 1);
    TestTrue(TEXT("Incomplete expected cache lifecycle is surfaced"), Assessment.bHasIncompleteRelevantRecord);
    TestFalse(TEXT("Incomplete expected cache lifecycle cannot report aggregate completion"), Assessment.bAllRelevantRecordsCompleted);

    return !HasAnyErrors();
}

#endif // WITH_DEV_AUTOMATION_TESTS
