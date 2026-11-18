// Copyright (c) 2025 71 Studios. All rights reserved.

#include "StageZeroWarmupManifestDetails.h"

#include "DetailCategoryBuilder.h"
#include "DetailLayoutBuilder.h"
#include "DetailWidgetRow.h"
#include "PropertyHandle.h"
#include "StageZeroWarmupManifest.h"
#include "Algo/Accumulate.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Styling/AppStyle.h"
#include "EditorStyleSet.h"
#include "Runtime/Launch/Resources/Version.h"

namespace
{
static FSlateFontInfo StageZeroEditorFontStyle(const ANSICHAR* FontName)
{
#if ENGINE_MAJOR_VERSION == 5 && ENGINE_MINOR_VERSION <= 0
    return FEditorStyle::GetFontStyle(FName(FontName));
#else
    return FAppStyle::GetFontStyle(FontName);
#endif
}
}

TSharedRef<IDetailCustomization> FStageZeroWarmupManifestDetails::MakeInstance()
{
    return MakeShareable(new FStageZeroWarmupManifestDetails());
}

void FStageZeroWarmupManifestDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
    TArray<TWeakObjectPtr<UObject>> ObjectsBeingCustomized;
    DetailBuilder.GetObjectsBeingCustomized(ObjectsBeingCustomized);

    UStageZeroWarmupManifest* Manifest = nullptr;
    if (ObjectsBeingCustomized.Num() == 1)
    {
        Manifest = Cast<UStageZeroWarmupManifest>(ObjectsBeingCustomized[0].Get());
    }

    HideHeavyProperties(DetailBuilder);
    AddSummary(DetailBuilder, Manifest);
}

void FStageZeroWarmupManifestDetails::HideHeavyProperties(IDetailLayoutBuilder& DetailBuilder)
{
    auto SafeHide = [&DetailBuilder](const FName PropertyName)
    {
        TSharedRef<IPropertyHandle> PropertyHandle = DetailBuilder.GetProperty(PropertyName, UStageZeroWarmupManifest::StaticClass());
        if (PropertyHandle->IsValidHandle())
        {
            DetailBuilder.HideProperty(PropertyHandle);
        }
    };

    SafeHide(GET_MEMBER_NAME_CHECKED(UStageZeroWarmupManifest, MapEntries));
    SafeHide(GET_MEMBER_NAME_CHECKED(UStageZeroWarmupManifest, WPCells));
    SafeHide(GET_MEMBER_NAME_CHECKED(UStageZeroWarmupManifest, WPAnchorGroups));
    SafeHide(GET_MEMBER_NAME_CHECKED(UStageZeroWarmupManifest, WPSplineGroups));
    SafeHide(GET_MEMBER_NAME_CHECKED(UStageZeroWarmupManifest, AllMaterials));
    SafeHide(GET_MEMBER_NAME_CHECKED(UStageZeroWarmupManifest, AllTextures));
    SafeHide(GET_MEMBER_NAME_CHECKED(UStageZeroWarmupManifest, AllNiagaraSystems));
    SafeHide(GET_MEMBER_NAME_CHECKED(UStageZeroWarmupManifest, AllStaticMeshes));
    SafeHide(GET_MEMBER_NAME_CHECKED(UStageZeroWarmupManifest, AllSkeletalMeshes));
}

void FStageZeroWarmupManifestDetails::AddSummary(IDetailLayoutBuilder& DetailBuilder, UStageZeroWarmupManifest* Manifest)
{
    IDetailCategoryBuilder& SummaryCategory = DetailBuilder.EditCategory(TEXT("Stage Zero Manifest"), FText::FromString(TEXT("Stage Zero Manifest")), ECategoryPriority::Important);

    if (!Manifest)
    {
        SummaryCategory.AddCustomRow(FText::FromString(TEXT("ManifestSummary")))
        .WholeRowContent()
        [
            SNew(STextBlock)
            .Text(FText::FromString(TEXT("Select a single manifest asset to view its summary.")))
        ];
        return;
    }

    const int32 MapCount = Manifest->MapEntries.Num();
    const int32 WPCellCount = Manifest->WPCells.Num();
    const int32 AnchorGroupCount = Manifest->WPAnchorGroups.Num();
    const int32 SplineGroupCount = Manifest->WPSplineGroups.Num();
    const int32 MaterialCount = Manifest->AllMaterials.Num();
    const int32 TextureCount = Manifest->AllTextures.Num();
    const int32 NiagaraCount = Manifest->AllNiagaraSystems.Num();
    const int32 StaticMeshCount = Manifest->AllStaticMeshes.Num();
    const int32 SkeletalMeshCount = Manifest->AllSkeletalMeshes.Num();
    auto CountUnique = [](auto&& Range, auto Selector)
    {
        TSet<FSoftObjectPath> Paths;
        for (const auto& Item : Range)
        {
            const TArray<FSoftObjectPath>& Source = Selector(Item);
            Paths.Append(Source);
        }
        return Paths.Num();
    };

    const int32 MapTextures = CountUnique(Manifest->MapEntries, [](const FStageZeroMapMatTexEntry& Entry) -> const TArray<FSoftObjectPath>& { return Entry.Textures; });
    const int32 MapStaticMeshes = CountUnique(Manifest->MapEntries, [](const FStageZeroMapMatTexEntry& Entry) -> const TArray<FSoftObjectPath>& { return Entry.StaticMeshes; });
    const int32 MapSkeletalMeshes = CountUnique(Manifest->MapEntries, [](const FStageZeroMapMatTexEntry& Entry) -> const TArray<FSoftObjectPath>& { return Entry.SkeletalMeshes; });
    const int32 AvgMaterialsPerMap = MapCount > 0 ? MaterialCount / MapCount : 0;
    const int32 AvgTexturesPerMap = MapCount > 0 ? TextureCount / MapCount : 0;

    auto MakeHeaderText = [](const TCHAR* InLabel)
    {
        return SNew(STextBlock)
            .Text(FText::FromString(InLabel))
            .Font(StageZeroEditorFontStyle("BoldFont"));
    };

    SummaryCategory.AddCustomRow(FText::FromString(TEXT("ManifestOverview")))
    .WholeRowContent()
    [
        SNew(SVerticalBox)
        + SVerticalBox::Slot().AutoHeight()
        [
            SNew(SBorder)
            .Padding(FMargin(8.0f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    MakeHeaderText(TEXT("Overview"))
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("Maps: %d"), MapCount)))
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("World Partition Cells: %d"), WPCellCount)))
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("Anchor Groups: %d, Spline Groups: %d"), AnchorGroupCount, SplineGroupCount)))
                ]
            ]
        ]
        + SVerticalBox::Slot().AutoHeight()
        [
            SNew(SBorder)
            .Padding(FMargin(8.0f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    MakeHeaderText(TEXT("Content Stats"))
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("Materials: %d, Textures: %d, Niagara Systems: %d"), MaterialCount, TextureCount, NiagaraCount)))
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("Average per map — Materials: %d, Textures: %d"), AvgMaterialsPerMap, AvgTexturesPerMap)))
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("Textures in Map Entries: %d"), MapTextures)))
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock).Text(FText::FromString(FString::Printf(TEXT("Meshes in Map Entries: Static=%d, Skeletal=%d, Total Static=%d, Total Skeletal=%d"), MapStaticMeshes, MapSkeletalMeshes, StaticMeshCount, SkeletalMeshCount)))
                ]
            ]
        ]
        + SVerticalBox::Slot().AutoHeight()
        [
            SNew(SBorder)
            .Padding(FMargin(8.0f))
            [
                SNew(SVerticalBox)
                + SVerticalBox::Slot().AutoHeight()
                [
                    MakeHeaderText(TEXT("Maps Snapshot"))
                ]
                + SVerticalBox::Slot().AutoHeight()
                [
                    SNew(STextBlock)
                    .Text(FText::FromString(
                        [&]()
                        {
                            const int32 MaxPreview = 5;
                            const int32 PreviewCount = FMath::Min(MaxPreview, MapCount);
                            FString PreviewText;
                            for (int32 Index = 0; Index < PreviewCount; ++Index)
                            {
                                PreviewText += FString::Printf(TEXT("%d) %s\n"), Index + 1, *Manifest->MapEntries[Index].MapPackageName.ToString());
                            }
                            if (MapCount > MaxPreview)
                            {
                                PreviewText += FString::Printf(TEXT("+ %d more map(s)..."), MapCount - MaxPreview);
                            }
                            if (PreviewText.IsEmpty())
                            {
                                PreviewText = TEXT("No maps in manifest.");
                            }
                            return PreviewText;
                        }()))
                ]
            ]
        ]
    ];
}
