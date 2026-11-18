#pragma once

#include "CoreMinimal.h"

struct FStageZeroManifestBuildResult;

namespace StageZeroManifestLog
{
/** Parse the last complete, schema-versioned commandlet summary from a log. */
bool ParseSummary(const FString& LogContents, FStageZeroManifestBuildResult& OutResult);
}
