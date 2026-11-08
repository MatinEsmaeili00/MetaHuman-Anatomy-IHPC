// CPP_UW_IHPCHud.h
// -----------------------------------------------------------------------------
// The full-screen IHPC HUD - the top of the screen in every Figma iPad frame.
//
//   +-------------------------------------------------------------------+
//   |              AVATAR INJURY PREVENTION TECHNOLOGY                    |  title bar (#1D3C55, 105 px)
//   +-------------------------------------------------------------------+
//   |  Profile | Progress | Saved Videos | History & Timeline | Settings  |  nav bar (#274A65, 88 px)
//   +-------------------------------------------------------------------+     (hidden until opened)
//                                (v)                                        chevron button
//
//   * Chevron button: opens / closes the nav bar (Figma frame 1 = closed, frame 3 = open).
//     Closed: dark see-through circle + white down arrow. Open: white circle + blue up arrow.
//   * Tabs: clicking a tab selects it (green highlight). Clicking it again unselects it.
//   * "Saved Videos" opens the analysis panel (Figma frame 5) and closes the nav bar,
//     because that frame shows the panel with the nav bar closed. Change AnalysisPanelTab
//     to make a different tab open the panel.
//   * Profile / Progress / History & Timeline / Settings each open their own EMPTY panel with the
//     same look as the analysis panel's background (Figma frame 5 "Rectangle 26": black 62 %,
//     radius 29, 1217 x 798). Their content is not designed yet; add it inside the panels.
//   * Close "X" (PanelCloseButton): a round button on the top-right corner of whichever panel is
//     open. Clicking it closes the panel and unselects the tab (same as clicking the tab again).
//   * ACTIONS menu (the 4th tab, Tab_HistoryTimeline, labelled "Actions" in the Widget Blueprint; its panel
//     HistoryTimelinePanel holds the ActionsList): every Button named "Action_<Name>" anywhere in the HUD
//     (Action_Running, Action_TennisForehand, Action_SoccerWarmup) plays the action <Name> on the possessed pawn's
//     UAnimationTimelineComp (its Action Clips). The playing action's button is highlighted green; clicking it again
//     goes back to walk / jog / run. Actions the pawn has no clip for are greyed out. New action = add a button
//     named Action_<Name> in the designer + a clip under <Name> in each pawn's AnimationTimelineComp > Action Clips.
//
// Clicks on empty parts of the HUD pass straight through to the 3D view, so the
// existing mouse orbit / pan / zoom keeps working.
//
// Widget Blueprint: /Game/UserInterface/IHPC/Widgets/WBP_IHPC_HUD
// Created by ACPP_PlayerController::BeginPlay (MainHUDWidgetClass, set in BP_PlayerController),
// the same way as the anatomy switcher and pawn switcher, and drawn above them (Z-order 10).
// -----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Styling/SlateTypes.h"
#include "IHPC_UI/IHPCTypes.h"
#include "CPP_UW_IHPCHud.generated.h"

class UButton;
class UImage;
class UTexture2D;
class UCPP_UW_IHPCNavTab;
class UCPP_UW_IHPCAnalysisPanel;
class UCPP_UW_IHPCTimeline;
class UCPP_UW_IHPCHud;
class UAnimationTimelineComp;
class APawn;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIHPCTabChanged, EIHPCNavTab, NewTab);

/** Fired when an Action button starts an action (ActionName) or goes back to the locomotion (None). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIHPCActionChanged, FName, ActionName);

/** One per Action_<Name> button: a button's OnClicked has no parameters, so this remembers which action it is. */
UCLASS()
class METAHUMANTEST_API UIHPCActionButtonProxy : public UObject
{
	GENERATED_BODY()

public:
	TWeakObjectPtr<UCPP_UW_IHPCHud> Hud;
	TWeakObjectPtr<UButton> Button;
	FName ActionName;
	/** The button's look from the Widget Blueprint (restored when it is not the playing action). */
	FButtonStyle DesignStyle;

	UFUNCTION()
	void HandleClicked();
};

UCLASS(Abstract)
class METAHUMANTEST_API UCPP_UW_IHPCHud : public UUserWidget
{
	GENERATED_BODY()

public:
	UCPP_UW_IHPCHud(const FObjectInitializer& ObjectInitializer);

	// ---------------------------------------------------------------- settings

	/** Which tab opens the analysis panel. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IHPC|Behaviour")
	EIHPCNavTab AnalysisPanelTab = EIHPCNavTab::SavedVideos;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IHPC|Behaviour")
	bool bStartWithNavBarOpen = false;

	/** Height of the nav bar (Figma: 88 px). The chevron moves down by this much when it opens. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IHPC|Layout")
	float NavBarHeight = 88.f;

	/** Arrow shown while the nav bar is closed (white, pointing down). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	TObjectPtr<UTexture2D> ChevronClosedTexture;

	/** Arrow shown while the nav bar is open (blue #245983, pointing up). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	TObjectPtr<UTexture2D> ChevronOpenTexture;

	/** Circle colour while closed: #1D3C55 at 44% (Figma frame 1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	FLinearColor ChevronCircleClosedColor;

	/** Circle colour while open: white at 86% (Figma frame 3). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	FLinearColor ChevronCircleOpenColor;

	// ---------------------------------------------------------------- actions menu

	/** Buttons whose name starts with this play the action named by the rest (Action_Running -> "Running"). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IHPC|Actions")
	FString ActionButtonPrefix = TEXT("Action_");

	/** Close the Actions panel after picking an action, so the avatar can be seen doing it. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Actions")
	bool bCloseActionsPanelOnSelect = true;

	/** Fill of the playing action's button (the nav bar's highlight green #B3CA7F, see-through). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Actions")
	FLinearColor ActionSelectedColor;

	// ---------------------------------------------------------------- events

	UPROPERTY(BlueprintAssignable, Category = "IHPC|Events")
	FIHPCTabChanged OnTabChanged;

	UPROPERTY(BlueprintAssignable, Category = "IHPC|Events")
	FIHPCActionChanged OnActionChanged;

	/** Same as clicking the Action_<ActionName> button: plays that action on the possessed pawn, or goes back to
	 *  walk / jog / run if it is already playing (or ActionName is None). Returns true if an action is now playing. */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Actions")
	bool SelectAction(FName ActionName);

	/** Action playing on the possessed pawn, or None while it walks / jogs / runs. */
	UFUNCTION(BlueprintPure, Category = "IHPC|Actions")
	FName GetPlayingAction() const;

	/** Re-reads the possessed pawn's actions: highlight + greyed-out buttons. */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Actions")
	void RefreshActionButtons();

	// ---------------------------------------------------------------- functions

	UFUNCTION(BlueprintCallable, Category = "IHPC|HUD")
	void SetNavBarOpen(bool bOpen);

	UFUNCTION(BlueprintCallable, Category = "IHPC|HUD")
	void ToggleNavBar() { SetNavBarOpen(!bNavBarOpen); }

	UFUNCTION(BlueprintPure, Category = "IHPC|HUD")
	bool IsNavBarOpen() const { return bNavBarOpen; }

	/** Selects a tab (None = unselect all) and opens/closes the analysis panel to match. */
	UFUNCTION(BlueprintCallable, Category = "IHPC|HUD")
	void SelectTab(EIHPCNavTab Tab);

	UFUNCTION(BlueprintPure, Category = "IHPC|HUD")
	EIHPCNavTab GetSelectedTab() const { return SelectedTab; }

	UFUNCTION(BlueprintCallable, Category = "IHPC|HUD")
	void SetAnalysisPanelOpen(bool bOpen);

	/** Closes whichever tab panel is open and unselects its tab (what the close "X" does). */
	UFUNCTION(BlueprintCallable, Category = "IHPC|HUD")
	void ClosePanel() { SelectTab(EIHPCNavTab::None); }

	/** The panel a tab opens: the analysis panel for AnalysisPanelTab, otherwise ProfilePanel /
	 *  ProgressPanel / HistoryTimelinePanel / SettingsPanel. Null for None or a missing panel. */
	UFUNCTION(BlueprintPure, Category = "IHPC|HUD")
	UWidget* GetPanelForTab(EIHPCNavTab Tab) const;

	UFUNCTION(BlueprintPure, Category = "IHPC|HUD")
	bool IsAnyPanelOpen() const { return GetPanelForTab(SelectedTab) != nullptr; }

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	/** The nav bar strip (with the tabs inside). Shown/hidden by the chevron. */
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> NavBar;

	/** Circle + arrow + button together. Moved down by NavBarHeight when the nav bar opens. */
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> ChevronGroup;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ChevronButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> ChevronCircle;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> ChevronIcon;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCPP_UW_IHPCNavTab> Tab_Profile;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCPP_UW_IHPCNavTab> Tab_Progress;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCPP_UW_IHPCNavTab> Tab_SavedVideos;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCPP_UW_IHPCNavTab> Tab_HistoryTimeline;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCPP_UW_IHPCNavTab> Tab_Settings;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCPP_UW_IHPCAnalysisPanel> AnalysisPanel;

	/** Empty panels for the other tabs, styled like Figma frame 5 "Rectangle 26". Put each tab's content inside. */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> ProfilePanel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> ProgressPanel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> HistoryTimelinePanel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> SettingsPanel;

	/** Round "X" on the top-right corner of the open panel. Shown only while a panel is open. */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> PanelCloseButton;

	/** Playback / timeline bar at the bottom (Figma "ihpc ui (1)ee 1"). Replaces the old prototype timeline. */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCPP_UW_IHPCTimeline> Timeline;

private:
	UFUNCTION() void HandleChevronClicked();
	UFUNCTION() void HandleTabClicked(EIHPCNavTab Tab);
	UFUNCTION() void HandleRecordingSaved(const FIHPCTaggedMoment& Moment);
	UFUNCTION() void HandleBookmarkAdded(const FIHPCTaggedMoment& Moment);
	UFUNCTION() void HandlePanelCloseClicked();
	UFUNCTION() void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	friend class UIHPCActionButtonProxy;
	void HandleActionClicked(FName ActionName);

	/** Finds every Action_<Name> button and binds it (once). */
	void BindActionButtons();

	/** The possessed pawn's timeline (actions live there). */
	UAnimationTimelineComp* GetAvatarTimeline() const;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UIHPCActionButtonProxy>> ActionButtons;

	/** Shows the panel of the selected tab, hides the others, shows / hides the close "X". */
	void RefreshPanels();

	/** Updates the circle colour, arrow picture and position for the current open/closed state. */
	void RefreshChevron();

	TArray<UCPP_UW_IHPCNavTab*> GetTabs() const;

	bool bNavBarOpen = false;
	EIHPCNavTab SelectedTab = EIHPCNavTab::None;
};
