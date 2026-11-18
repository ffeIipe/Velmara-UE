#include "Modules/ModuleManager.h"
#include "StageZeroManifestTab.h"
#include "Widgets/Docking/SDockTab.h"
#include "ToolMenus.h"
#include "Styling/AppStyle.h"
#include "Interfaces/IPluginManager.h"
#include "Styling/SlateStyle.h"
#include "Styling/SlateStyleRegistry.h"
#include "PropertyEditorModule.h"
#include "StageZeroWarmupManifestDetails.h"
#include "StageZeroWarmupManifest.h"
#include "StageZeroWarmupUtils.h"
#include "StageZeroManifestBuilder.h"
#include "HAL/IConsoleManager.h"

static const FName StageZeroManifestTabName("StageZeroManifestTab");

class FStageZeroEditorModule : public IModuleInterface
{
public:
    virtual void StartupModule() override
    {
        // The editor module must load during commandlet discovery, but commandlets do not
        // have an interactive Slate/ToolMenus surface to register against.
        if (IsRunningCommandlet())
        {
            return;
        }

        bEditorUIRegistered = true;
        RegisterStyle();
        RegisterEditorConsoleCommands();
        FGlobalTabmanager::Get()->RegisterNomadTabSpawner(StageZeroManifestTabName,
            FOnSpawnTab::CreateRaw(this, &FStageZeroEditorModule::OnSpawnManifestTab))
            .SetDisplayName(FText::FromString(TEXT("Stage Zero Manifest")))
            .SetTooltipText(FText::FromString(TEXT("Generate and inspect the Stage Zero warmup manifest.")))
            .SetMenuType(ETabSpawnerMenuType::Enabled)
            .SetIcon(FSlateIcon(StyleSetName, IconBrushName));

        RegisterMenus();

        FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
        PropertyModule.RegisterCustomClassLayout(
            TEXT("StageZeroWarmupManifest"),
            FOnGetDetailCustomizationInstance::CreateStatic(&FStageZeroWarmupManifestDetails::MakeInstance));
        PropertyModule.NotifyCustomizationModuleChanged();
    }

    virtual void ShutdownModule() override
    {
        if (!bEditorUIRegistered)
        {
            return;
        }

        if (BuildManifestConsoleCommand)
        {
            IConsoleManager::Get().UnregisterConsoleObject(BuildManifestConsoleCommand, false);
            BuildManifestConsoleCommand = nullptr;
        }

        if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
        {
            FPropertyEditorModule& PropertyModule = FModuleManager::GetModuleChecked<FPropertyEditorModule>("PropertyEditor");
            PropertyModule.UnregisterCustomClassLayout(TEXT("StageZeroWarmupManifest"));
            PropertyModule.NotifyCustomizationModuleChanged();
        }
        if (IconStyle.IsValid())
        {
            FSlateStyleRegistry::UnRegisterSlateStyle(*IconStyle);
            IconStyle.Reset();
        }
        UToolMenus::UnRegisterStartupCallback(StartupCallbackHandle);
        UToolMenus::UnregisterOwner(this);
        FGlobalTabmanager::Get()->UnregisterNomadTabSpawner(StageZeroManifestTabName);
    }

    TSharedRef<SDockTab> OnSpawnManifestTab(const FSpawnTabArgs& Args)
    {
        return SNew(SDockTab)
            .TabRole(ETabRole::NomadTab)
            [
                SNew(SStageZeroManifestTab)
            ];
    }

    void RegisterMenus()
    {
        StartupCallbackHandle = UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateRaw(this, &FStageZeroEditorModule::RegisterToolMenus));
    }

    void RegisterToolMenus()
    {
        UToolMenus* Menus = UToolMenus::Get();
        if (!Menus)
        {
            return;
        }

        FToolMenuOwnerScoped OwnerScoped(this);

        if (!AddManifestToolbarEntry(Menus, FName(TEXT("LevelEditor.LevelEditorToolBar.User"))) &&
            !AddManifestToolbarEntry(Menus, FName(TEXT("LevelEditor.LevelEditorToolBar.AssetsToolBar"))))
        {
            AddManifestToolbarEntry(Menus, FName(TEXT("LevelEditor.LevelToolbar")));
        }
    }

    bool AddManifestToolbarEntry(UToolMenus* Menus, const FName ToolbarMenuName)
    {
        if (!Menus || !Menus->IsMenuRegistered(ToolbarMenuName))
        {
            return false;
        }

        UToolMenu* ToolbarMenu = Menus->ExtendMenu(ToolbarMenuName);
        if (!ToolbarMenu)
        {
            return false;
        }

        FToolMenuSection& Section = ToolbarMenu->AddSection("StageZero", FText::FromString(TEXT("Stage Zero")));
        FToolMenuEntry Entry = FToolMenuEntry::InitToolBarButton(
            "StageZeroManifest",
            FUIAction(FExecuteAction::CreateRaw(this, &FStageZeroEditorModule::OpenManifestTab)),
            FText::FromString(TEXT("Stage Zero")),
            FText::FromString(TEXT("Open Stage Zero Manifest Utility")),
            FSlateIcon(StyleSetName, IconBrushName));
        Entry.SetCommandList(nullptr);
        Section.AddEntry(Entry);
        return true;
    }

    void OpenManifestTab()
    {
        FGlobalTabmanager::Get()->TryInvokeTab(StageZeroManifestTabName);
    }

    void RegisterEditorConsoleCommands()
    {
        BuildManifestConsoleCommand = IConsoleManager::Get().RegisterConsoleCommand(
            TEXT("StageZero.BuildMatTexManifest"),
            TEXT("Rebuild the Stage Zero asset preload manifest (editor only)."),
            FConsoleCommandDelegate::CreateRaw(this, &FStageZeroEditorModule::BuildManifestFromConsole),
            ECVF_Default);
    }

    void BuildManifestFromConsole()
    {
        FStageZeroManifestBuildResult Result;
        const FStageZeroManifestBuildOptions Options = FStageZeroManifestBuilder::LoadManifestBuildOptions();
        if (UStageZeroWarmupManifest* Manifest = FStageZeroManifestBuilder::BuildManifestWithOptions(Options, &Result))
        {
            UE_LOG(LogTemp, Display, TEXT("StageZero manifest rebuilt: Maps=%d Materials=%d Textures=%d StaticMeshes=%d SkeletalMeshes=%d"),
                Manifest->MapEntries.Num(),
                Manifest->AllMaterials.Num(),
                Manifest->AllTextures.Num(),
                Manifest->AllStaticMeshes.Num(),
                Manifest->AllSkeletalMeshes.Num());
        }
        else
        {
            UE_LOG(LogTemp, Error, TEXT("StageZero manifest rebuild failed: %s"), Result.FailureReason.IsEmpty() ? TEXT("unknown reason") : *Result.FailureReason);
        }
    }

private:
    void RegisterStyle()
    {
        if (IconStyle.IsValid())
        {
            return;
        }

        const TSharedPtr<IPlugin> Plugin = IPluginManager::Get().FindPlugin(TEXT("StageZero"));
        if (!Plugin.IsValid())
        {
            return;
        }

        FString ContentDir = Plugin->GetBaseDir() / TEXT("Resources");
        IconStyle = MakeShared<FSlateStyleSet>(StyleSetName);
        IconStyle->SetContentRoot(ContentDir);
        IconStyle->SetCoreContentRoot(ContentDir);
        IconStyle->Set(IconBrushName, new FSlateImageBrush(FPaths::Combine(ContentDir, TEXT("manifesticon.png")), FVector2D(32.0f, 32.0f)));
        FSlateStyleRegistry::RegisterSlateStyle(*IconStyle);
    }

    FDelegateHandle StartupCallbackHandle;
    IConsoleObject* BuildManifestConsoleCommand = nullptr;
    TSharedPtr<FSlateStyleSet> IconStyle;
    bool bEditorUIRegistered = false;
    static constexpr const TCHAR* StyleSetName = TEXT("StageZeroEditorStyle");
    static constexpr const TCHAR* IconBrushName = TEXT("StageZeroManifest.Icon");
};

IMPLEMENT_MODULE(FStageZeroEditorModule, StageZeroEditor);
