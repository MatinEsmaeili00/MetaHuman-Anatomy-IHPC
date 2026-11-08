// CPP_UW_IHPCHud.cpp - title bar + chevron + nav bar + analysis panel. See header.

#include "IHPC_UI/CPP_UW_IHPCHud.h"

#include "IHPC_UI/CPP_UW_IHPCAnalysisPanel.h"
#include "IHPC_UI/CPP_UW_IHPCNavTab.h"
#include "IHPC_UI/CPP_UW_IHPCTimeline.h"
#include "IHPC_UI/IHPCUILibrary.h"

#include "AnimationTimelineComp.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

#define LOCTEXT_NAMESPACE "IHPCHud"

namespace
{
	// The nav bar separators (TabSeparator_1, _2, ...) are 1 Slate unit wide (Figma: 1 px). When the
	// UI is scaled down (any viewport shorter than 1080 px, e.g. Play-In-Editor), that is less than
	// one screen pixel, and pixel snapping rounds a separator to 1 px or to nothing depending on
	// exactly where it lands - which shifts whenever a hovered tab's bold font widens the centred row.
	// Make them exactly one screen pixel wide at those scales so they always draw.
	void KeepTabSeparatorsOnePixelWide(UUserWidget* Hud)
	{
		const float Scale = UWidgetLayoutLibrary::GetViewportScale(Hud);
		const float Width = (Scale > 0.f && Scale < 1.f) ? 1.f / Scale : 1.f;
		for (int32 Index = 1; ; ++Index)
		{
			UImage* Separator = Cast<UImage>(Hud->GetWidgetFromName(FName(*FString::Printf(TEXT("TabSeparator_%d"), Index))));
			if (!Separator)
			{
				break;
			}
			Separator->SetDesiredSizeOverride(FVector2D(Width, Separator->GetBrush().GetImageSize().Y));
		}
	}
}

UCPP_UW_IHPCHud::UCPP_UW_IHPCHud(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Exact Figma colours (see header).
	ChevronCircleClosedColor = UIHPCUILibrary::FigmaColor(TEXT("1D3C55"), 0.44f);
	ChevronCircleOpenColor = UIHPCUILibrary::FigmaColor(TEXT("FFFFFF"), 0.86f);
	ActionSelectedColor = UIHPCUILibrary::FigmaColor(TEXT("B3CA7F"), 0.45f);
}

void UIHPCActionButtonProxy::HandleClicked()
{
	if (UCPP_UW_IHPCHud* Owner = Hud.Get())
	{
		Owner->HandleActionClicked(ActionName);
	}
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

	// Actions menu: Action_<Name> buttons drive the possessed pawn's timeline; re-check them when the pawn changes.
	BindActionButtons();
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &UCPP_UW_IHPCHud::HandlePossessedPawnChanged);
	}

	SetNavBarOpen(bStartWithNavBarOpen);
	SelectTab(EIHPCNavTab::None);   // nothing selected, analysis panel closed
}

void UCPP_UW_IHPCHud::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &UCPP_UW_IHPCHud::HandlePossessedPawnChanged);
	}
	Super::NativeDestruct();
}

// ============================================================================
// Actions menu (Action_<Name> buttons -> UAnimationTimelineComp::PlayAction / StopAction)
// ============================================================================

void UCPP_UW_IHPCHud::BindActionButtons()
{
	if (!WidgetTree || ActionButtonPrefix.IsEmpty())
	{
		return;
	}
	WidgetTree->ForEachWidget([this](UWidget* Widget)
	{
		UButton* Button = Cast<UButton>(Widget);
		const FString Name = Widget ? Widget->GetName() : FString();
		if (!Button || !Name.StartsWith(ActionButtonPrefix) || Name.Len() <= ActionButtonPrefix.Len())
		{
			return;
		}
		if (ActionButtons.ContainsByPredicate([Button](const UIHPCActionButtonProxy* P) { return P && P->Button.Get() == Button; }))
		{
			return;   // already bound (NativeConstruct runs again when the HUD is re-added to the viewport)
		}
		UIHPCActionButtonProxy* Proxy = NewObject<UIHPCActionButtonProxy>(this);
		Proxy->Hud = this;
		Proxy->Button = Button;
		Proxy->ActionName = FName(*Name.RightChop(ActionButtonPrefix.Len()));
		Proxy->DesignStyle = Button->GetStyle();
		Button->OnClicked.AddUniqueDynamic(Proxy, &UIHPCActionButtonProxy::HandleClicked);
		ActionButtons.Add(Proxy);
	});
	RefreshActionButtons();
}

UAnimationTimelineComp* UCPP_UW_IHPCHud::GetAvatarTimeline() const
{
	return UIHPCUILibrary::FindAvatarTimeline(this);
}

FName UCPP_UW_IHPCHud::GetPlayingAction() const
{
	const UAnimationTimelineComp* AvatarTimeline = GetAvatarTimeline();
	return AvatarTimeline ? AvatarTimeline->GetCurrentAction() : NAME_None;
}

void UCPP_UW_IHPCHud::HandleActionClicked(FName ActionName)
{
	const bool bPlaying = SelectAction(ActionName);
	if (bCloseActionsPanelOnSelect && bPlaying)
	{
		ClosePanel();
	}
}

bool UCPP_UW_IHPCHud::SelectAction(FName ActionName)
{
	UAnimationTimelineComp* AvatarTimeline = GetAvatarTimeline();
	if (!AvatarTimeline)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IHPC] Actions: the possessed pawn has no AnimationTimelineComp, '%s' can't play."), *ActionName.ToString());
		return false;
	}

	bool bPlaying = false;
	if (ActionName.IsNone() || AvatarTimeline->GetCurrentAction() == ActionName)
	{
		AvatarTimeline->StopAction();          // second click on the playing action: back to walk / jog / run
	}
	else
	{
		bPlaying = AvatarTimeline->PlayAction(ActionName);
	}

	RefreshActionButtons();
	OnActionChanged.Broadcast(AvatarTimeline->GetCurrentAction());
	return bPlaying;
}

void UCPP_UW_IHPCHud::RefreshActionButtons()
{
	const UAnimationTimelineComp* AvatarTimeline = GetAvatarTimeline();
	const FName Playing = AvatarTimeline ? AvatarTimeline->GetCurrentAction() : NAME_None;

	for (const UIHPCActionButtonProxy* Proxy : ActionButtons)
	{
		UButton* Button = Proxy ? Proxy->Button.Get() : nullptr;
		if (!Button)
		{
			continue;
		}
		// Greyed out when this pawn has no clip for the action (e.g. the plain skeleton model).
		Button->SetIsEnabled(AvatarTimeline && AvatarTimeline->HasAction(Proxy->ActionName));

		FButtonStyle Style = Proxy->DesignStyle;
		if (Proxy->ActionName == Playing)
		{
			// Same rounded box as designed, filled with the highlight green (a bit brighter on hover / press).
			auto Tint = [this](FSlateBrush& Brush, float Boost)
			{
				FLinearColor Color = ActionSelectedColor;
				Color.A = FMath::Clamp(Color.A + Boost, 0.f, 1.f);
				Brush.TintColor = FSlateColor(Color);
			};
			Tint(Style.Normal, 0.f);
			Tint(Style.Hovered, 0.1f);
			Tint(Style.Pressed, 0.2f);
		}
		Button->SetStyle(Style);
	}
}

void UCPP_UW_IHPCHud::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	RefreshActionButtons();
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
	if (bOpen)
	{
		// Re-checked on every open, so it follows the window size the bar is shown at.
		KeepTabSeparatorsOnePixelWide(this);
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
		RefreshActionButtons(); // the Actions panel shows what the (possibly switched) pawn is doing now
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
