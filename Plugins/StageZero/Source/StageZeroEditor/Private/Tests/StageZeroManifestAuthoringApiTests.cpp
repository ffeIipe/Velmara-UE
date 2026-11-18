#if WITH_DEV_AUTOMATION_TESTS

#include "StageZeroManifestAuthoring.h"

#include "Misc/AutomationTest.h"

#include <type_traits>

namespace
{
using FStageZeroManifestBuildFunction = UStageZeroWarmupManifest* (*)(
    const FStageZeroManifestBuildOptions&,
    FStageZeroManifestAuthoringResult*,
    FStageZeroManifestAuthoringProgressCallback);

static_assert(
    std::is_same_v<decltype(&FStageZeroManifestAuthoring::BuildManifest), FStageZeroManifestBuildFunction>,
    "The public manifest-authoring entry point must retain its documented typed result and per-call callback contract.");
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(
    FStageZeroManifestAuthoringApiDefaultsTest,
    "StageZero.Editor.ManifestAuthoring.PublicApiDefaults",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)

bool FStageZeroManifestAuthoringApiDefaultsTest::RunTest(const FString& Parameters)
{
    (void)Parameters;

    const FStageZeroManifestAuthoringProgress Progress;
    TestEqual(TEXT("Progress starts with no active map"), Progress.MapIndex, 0);
    TestEqual(TEXT("Progress starts with no total maps"), Progress.MapTotal, 0);
    TestTrue(TEXT("Progress starts without a map package"), Progress.MapPackage.IsNone());

    const FStageZeroManifestAuthoringResult Result;
    TestFalse(TEXT("A default result does not claim success"), Result.bSucceeded);
    TestFalse(TEXT("A default result does not claim a saved manifest"), Result.bManifestSaved);
    TestEqual(TEXT("A default result has no requested maps"), Result.NumRequestedMaps, 0);
    TestTrue(TEXT("A default result has no failure text"), Result.FailureReason.IsEmpty());
    return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
