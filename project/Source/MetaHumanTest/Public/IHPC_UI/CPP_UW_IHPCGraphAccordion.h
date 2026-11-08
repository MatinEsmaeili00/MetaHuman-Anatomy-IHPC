// CPP_UW_IHPCGraphAccordion.h
// -----------------------------------------------------------------------------
// The right-hand accordion of the analysis panel. Figma: "Component 3" (286 px wide).
//
//   [ Show graphs     v ]   <- click a header to open/close its section
//   [ Active Muscles  v ]      (only one section open at a time, like Figma Variant2)
//   [ Tagged Points   v ]
//
// "Show graphs" open (Figma Variant2):
//   Movement Graph   (live line graph)
//   Active Muscles   (live line graph)
//   (o)| Bones   (o)| Nerves   (o)| Muscles   (o)| Tendons    <- toggle switches
//                                         [download] [zoom]
//
// Toggle behaviour: when NO toggle is on, every line is shown (this is the state
// drawn in Figma: all toggles off, all lines visible). Turning toggles on filters
// the graphs down to just those lines.
//
// Download writes the graph numbers to a CSV file in <Project>/Saved/IHPC_Exports/.
// Zoom asks the analysis panel to show a large copy of the Movement Graph.
//
// Widget Blueprint: /Game/UserInterface/IHPC/Widgets/WBP_IHPC_GraphAccordion
// -----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IHPC_UI/IHPCTypes.h"
#include "CPP_UW_IHPCGraphAccordion.generated.h"

class UButton;
class UImage;
class UTexture2D;
class UVerticalBox;
class UCPP_UW_IHPCLineGraph;
class UCPP_UW_IHPCToggleSwitch;
class UCPP_UW_IHPCListRow;

/** Fired when the CSV export finishes. (FIHPCSimpleEvent / FIHPCMomentEvent live in IHPCTypes.h.) */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIHPCExportFinished, const FString&, FilePath);

UCLASS(Abstract)
class METAHUMANTEST_API UCPP_UW_IHPCGraphAccordion : public UUserWidget
{
	GENERATED_BODY()

public:
	UCPP_UW_IHPCGraphAccordion(const FObjectInitializer& ObjectInitializer);

	// ---------------------------------------------------------------- settings

	/** Which section is open when the widget first appears. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IHPC|Behaviour")
	EIHPCAccordionSection StartOpenSection = EIHPCAccordionSection::None;

	/** If true, clicking a tagged point jumps the avatar animation to that moment. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IHPC|Behaviour")
	bool bSeekTimelineOnTaggedPointClick = true;

	/** Chevron shown on an OPEN section header (Figma: up arrow). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	TObjectPtr<UTexture2D> ChevronUpTexture;

	/** Chevron shown on a CLOSED section header (Figma: down arrow). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	TObjectPtr<UTexture2D> ChevronDownTexture;

	/** Font for the Active Muscles / Tagged Points rows. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	FSlateFontInfo ListFont;

	/** Sample rows for "Active Muscles" until real data is connected (names from the project's Muscles folder). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Data")
	TArray<FText> ActiveMuscles;

	// ---------------------------------------------------------------- events

	/** The zoom (magnifier) button was clicked. */
	UPROPERTY(BlueprintAssignable, Category = "IHPC|Events")
	FIHPCSimpleEvent OnZoomRequested;

	/** The CSV export finished. Gives the full file path. */
	UPROPERTY(BlueprintAssignable, Category = "IHPC|Events")
	FIHPCExportFinished OnGraphsExported;

	/** A row in "Tagged Points" was clicked. */
	UPROPERTY(BlueprintAssignable, Category = "IHPC|Events")
	FIHPCMomentEvent OnTaggedPointClicked;

	// ---------------------------------------------------------------- functions

	/** Opens one section and closes the others. Pass None to close all. */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Accordion")
	void OpenSection(EIHPCAccordionSection Section);

	/** Opens the section, or closes it if it is already open (what a header click does). */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Accordion")
	void ToggleSection(EIHPCAccordionSection Section);

	UFUNCTION(BlueprintPure, Category = "IHPC|Accordion")
	EIHPCAccordionSection GetOpenSection() const { return OpenSectionState; }

	/** Writes both graphs to <Project>/Saved/IHPC_Exports/IHPC_Graphs_<date>_<time>.csv and returns the path. */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Accordion")
	FString ExportGraphsToCSV();

	/** Fills the "Tagged Points" list. The analysis panel passes the same data the video cards use. */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Accordion")
	void SetTaggedPoints(const TArray<FIHPCTaggedMoment>& Points);

	/** Adds one row to the end of "Tagged Points" (used for new bookmarks / recordings from the timeline). */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Accordion")
	void AddTaggedPoint(const FIHPCTaggedMoment& Point);

	/** Fills the "Active Muscles" list. */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Accordion")
	void SetActiveMuscles(const TArray<FText>& MuscleNames);

	UFUNCTION(BlueprintPure, Category = "IHPC|Accordion")
	UCPP_UW_IHPCLineGraph* GetMovementGraph() const { return MovementGraph; }

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ---- Widgets that must exist in the Widget Blueprint (names must match exactly) ----

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Header_ShowGraphs;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Header_ActiveMuscles;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> Header_TaggedPoints;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> Chevron_ShowGraphs;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> Chevron_ActiveMuscles;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UImage> Chevron_TaggedPoints;

	/** The dark panels that appear under each header when it is open. */
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> Content_ShowGraphs;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> Content_ActiveMuscles;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> Content_TaggedPoints;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCPP_UW_IHPCLineGraph> MovementGraph;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCPP_UW_IHPCLineGraph> ActiveMusclesGraph;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCPP_UW_IHPCToggleSwitch> Toggle_Bones;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCPP_UW_IHPCToggleSwitch> Toggle_Nerves;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCPP_UW_IHPCToggleSwitch> Toggle_Muscles;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCPP_UW_IHPCToggleSwitch> Toggle_Tendons;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> DownloadButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> ZoomButton;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UVerticalBox> ActiveMusclesList;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UVerticalBox> TaggedPointsList;

private:
	UFUNCTION() void HandleShowGraphsHeader();
	UFUNCTION() void HandleActiveMusclesHeader();
	UFUNCTION() void HandleTaggedPointsHeader();
	UFUNCTION() void HandleDownload();
	UFUNCTION() void HandleZoom();
	UFUNCTION() void HandleToggleChanged(UCPP_UW_IHPCToggleSwitch* Switch, bool bIsOn);
	UFUNCTION() void HandleTaggedPointRowClicked(UCPP_UW_IHPCListRow* Row);

	/** Shows/hides the graph lines according to the 4 toggles (see "Toggle behaviour" above). */
	void ApplyToggleFilter();

	/** Shows the open section, hides the others, swaps chevrons. */
	void RefreshSections();

	void RebuildActiveMusclesList();
	void RebuildTaggedPointsList();

	/** The four toggles in series order: Bones, Nerves, Muscles, Tendons. */
	TArray<UCPP_UW_IHPCToggleSwitch*> GetToggles() const;

	EIHPCAccordionSection OpenSectionState = EIHPCAccordionSection::None;

	UPROPERTY()
	TArray<FIHPCTaggedMoment> TaggedPoints;
};
