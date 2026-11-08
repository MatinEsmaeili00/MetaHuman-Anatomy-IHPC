// CPP_UW_IHPCHud.cpp - title bar + chevron + nav bar + analysis panel. See header.

#include "IHPC_UI/CPP_UW_IHPCHud.h"

#include "IHPC_UI/CPP_UW_IHPCAnalysisPanel.h"
#include "IHPC_UI/CPP_UW_IHPCNavTab.h"
#include "IHPC_UI/CPP_UW_IHPCTimeline.h"
#include "IHPC_UI/IHPCUILibrary.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "IHPCHud"

UCPP_UW_IHPCHud::UCPP_UW_IHPCHud(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Exact Figma colours (see header).
	ChevronCircleClosedColor = UIHPCUILibrary::FigmaColor(TEXT("1D3C55"), 0.44f);
	ChevronCircleOpenColor = UIHPCUILibrary::FigmaColor(TEXT("FFFFFF"), 0.86f);
}

TArray<UCPP_UW_IHPCNavTab*> UCPP_UW_IHPCHud::GetTabs() const
{
	return { Tab_Profile, Tab_Progress, Tab_SavedVideos, Tab_HistoryTimeline, Tab_Settings };
}

void UCPP_UW_IHPCHud::NativePreConstruct()
{
	Super::NativePreConstruct();

	// Give every tab its id and default text, so the designer cannot get them mixed up.
	if (Tab_Profile)         Tab_Profile->SetTabIdAndDefaultLabel(EIHPCNavTab::Profile, LOCTEXT("Profile", "Profile"));
	if (Tab_Progress)        Tab_Progress->SetTabIdAndDefaultLabel(EIHPCNavTab::Progress, LOCTEXT("Progress", "Progress"));
	if (Tab_SavedVideos)     Tab_SavedVideos->SetTabIdAndDefaultLabel(EIHPCNavTab::SavedVideos, LOCTEXT("SavedVideos", "Saved Videos"));
	if (Tab_HistoryTimeline) Tab_HistoryTimeline->SetTabIdAndDefaultLabel(EIHPCNavTab::HistoryTimeline, LOCTEXT("HistoryTimeline", "History & Timeline"));
	if (Tab_Settings)        Tab_Settings->SetTabIdAndDefaultLabel(EIHPCNavTab::Settings, LOCTEXT("Settings", "Settings"));

	// In the designer, preview the starting state.
	bNavBarOpen = bStartWithNavBarOpen;
	if (NavBar)
	{
		NavBar->SetVisibility(bNavBarOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	RefreshChevron();
}

void UCPP_UW_IHPCHud::NativeConstruct()
{
	Super::NativeConstruct();

	// Empty areas of the HUD must not swallow clicks: the 3D view underneath needs them
	// for orbit / pan / zoom (see CPP_PlayerController::HandleMouseMove).
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);

	if (ChevronButton)
	{
		ChevronButton->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCHud::HandleChevronClicked);
	}
	for (UCPP_UW_IHPCNavTab* Tab : GetTabs())
	{
		if (Tab)
		{
			Tab->OnTabClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCHud::HandleTabClicked);
		}
	}
	if (PanelCloseButton)
	{
		PanelCloseButton->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCHud::HandlePanelCloseClicked);
	}

	// Timeline bar: its orange markers start as the saved videos; new recordings / bookmarks
	// made on the timeline are added to the analysis panel's lists.
	if (Timeline)
	{
		if (AnalysisPanel)
		{
			Timeline->SetMarkers(AnalysisPanel->GetSavedVideos());
		}
		Timeline->OnRecordingSaved.AddUniqueDynamic(this, &UCPP_UW_IHPCHud::HandleRecordingSaved);
		Timeline->OnBookmarkAdded.AddUniqueDynamic(this, &UCPP_UW_IHPCHud::HandleBookmarkAdded);
	}

	SetNavBarOpen(bStartWithNavBarOpen);
	SelectTab(EIHPCNavTab::None);   // nothing selected, analysis panel closed
}

void UCPP_UW_IHPCHud::HandleRecordingSaved(const FIHPCTaggedMoment& Moment)
{
	if (AnalysisPanel)
	{
		AnalysisPanel->AddSavedVideo(Moment);   // new video card + new tagged point
	}
}

void UCPP_UW_IHPCHud::HandleBookmarkAdded(const FIHPCTaggedMoment& Moment)
{
	if (AnalysisPanel)
	{
		AnalysisPanel->AddTaggedPoint(Moment);  // new tagged point
	}
}

// ============================================================================
// Nav bar + chevron
// ============================================================================

void UCPP_UW_IHPCHud::HandleChevronClicked()
{
	ToggleNavBar();
}

void UCPP_UW_IHPCHud::SetNavBarOpen(bool bOpen)
{
	bNavBarOpen = bOpen;
	if (NavBar)
	{
		NavBar->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	RefreshChevron();
}

void UCPP_UW_IHPCHud::RefreshChevron()
{
	if (ChevronGroup)
	{
		// The circle sits on the bottom edge of whichever bar is lowest.
		ChevronGroup->SetRenderTranslation(FVector2D(0.f, bNavBarOpen ? NavBarHeight : 0.f));
	}
	if (ChevronCircle)
	{
		ChevronCircle->SetColorAndOpacity(bNavBarOpen ? ChevronCircleOpenColor : ChevronCircleClosedColor);
	}
	if (ChevronIcon)
	{
		if (UTexture2D* Texture = bNavBarOpen ? ChevronOpenTexture.Get() : ChevronClosedTexture.Get())
		{
			ChevronIcon->SetBrushFromTexture(Texture, /*bMatchSize*/ false);
		}
	}
}

// ============================================================================
// Tabs + panels (analysis panel for Saved Videos, empty panels for the other tabs) + close "X"
// ============================================================================

void UCPP_UW_IHPCHud::HandleTabClicked(EIHPCNavTab Tab)
{
	// Clicking the selected tab again unselects it (and closes its panel).
	SelectTab(Tab == SelectedTab ? EIHPCNavTab::None : Tab);
}

void UCPP_UW_IHPCHud::SelectTab(EIHPCNavTab Tab)
{
	SelectedTab = Tab;
	for (UCPP_UW_IHPCNavTab* NavTab : GetTabs())
	{
		if (NavTab)
		{
			NavTab->SetSelected(NavTab->TabId == Tab && Tab != EIHPCNavTab::None);
		}
	}

	RefreshPanels();
	if (GetPanelForTab(Tab))
	{
		SetNavBarOpen(false);   // Figma frame 5: panel open, nav bar closed (it would cover the panel's top)
	}

	OnTabChanged.Broadcast(Tab);
}

UWidget* UCPP_UW_IHPCHud::GetPanelForTab(EIHPCNavTab Tab) const
{
	if (Tab == EIHPCNavTab::None)
	{
		return nullptr;
	}
	if (Tab == AnalysisPanelTab)
	{
		return AnalysisPanel;
	}
	switch (Tab)
	{
	case EIHPCNavTab::Profile:         return ProfilePanel;
	case EIHPCNavTab::Progress:        return ProgressPanel;
	case EIHPCNavTab::HistoryTimeline: return HistoryTimelinePanel;
	case EIHPCNavTab::Settings:        return SettingsPanel;
	default:                           return nullptr;   // Saved Videos when it is not the AnalysisPanelTab
	}
}

void UCPP_UW_IHPCHud::RefreshPanels()
{
	UWidget* OpenPanel = GetPanelForTab(SelectedTab);
	for (UWidget* Panel : TArray<UWidget*>{ AnalysisPanel, ProfilePanel, ProgressPanel, HistoryTimelinePanel, SettingsPanel })
	{
		if (Panel)
		{
			// Visible (not just drawn): an open panel blocks clicks from reaching the 3D view behind it.
			Panel->SetVisibility(Panel == OpenPanel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}
	if (PanelCloseButton)
	{
		PanelCloseButton->SetVisibility(OpenPanel ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

void UCPP_UW_IHPCHud::HandlePanelCloseClicked()
{
	ClosePanel();
}

void UCPP_UW_IHPCHud::SetAnalysisPanelOpen(bool bOpen)
{
	if (AnalysisPanel)
	{
		AnalysisPanel->SetVisibility(bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
	if (PanelCloseButton)
	{
		PanelCloseButton->SetVisibility(GetPanelForTab(SelectedTab) || bOpen ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
	}
}

#undef LOCTEXT_NAMESPACE
