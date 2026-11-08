// CPP_UW_IHPCAnalysisPanel.h
// -----------------------------------------------------------------------------
// The big dark panel. Figma: the panel on frame "iPad Pro 12.9" - 5" (1217 x 798).
//
//   +---------------------------------------------------------------------+
//   |  +-------------------------------------------+   [ Show graphs   v ] |
//   |  |  viewer area (870 x 559)                  |   [ Active Muscles v ] |
//   |  |  (empty - the MetaHuman picture in Figma  |   [ Tagged Points  v ] |
//   |  |   is NOT recreated; the zoomed graph      |                        |
//   |  |   appears here)                  [inspect]|                        |
//   |  +-------------------------------------------+                        |
//   |  +-------------------------------------------+                        |
//   |  |  videos row                               |                        |
//   |  +-------------------------------------------+                        |
//   +---------------------------------------------------------------------+
//
// It wires its children together:
//   * zoom button in the accordion -> shows a large Movement Graph in the viewer area
//   * inspect button (bottom-right of the viewer) -> closes the large graph
//     (and fires OnInspectClicked so a 3D close-up can be hooked up later)
//   * the "Tagged Points" list shows the same moments as the video cards
//
// Widget Blueprint: /Game/UserInterface/IHPC/Widgets/WBP_IHPC_AnalysisPanel
// -----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IHPC_UI/IHPCTypes.h"
#include "CPP_UW_IHPCAnalysisPanel.generated.h"

class UButton;
class UCPP_UW_IHPCGraphAccordion;
class UCPP_UW_IHPCVideosRow;
class UCPP_UW_IHPCLineGraph;

UCLASS(Abstract)
class METAHUMANTEST_API UCPP_UW_IHPCAnalysisPanel : public UUserWidget
{
	GENERATED_BODY()

public:
	/** The inspect button (magnifier with </> in the viewer corner) was clicked while no zoomed graph was open. */
	UPROPERTY(BlueprintAssignable, Category = "IHPC|Events")
	FIHPCSimpleEvent OnInspectClicked;

	/** Shows / hides the large Movement Graph in the viewer area. */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Panel")
	void SetZoomedGraphVisible(bool bVisible);

	UFUNCTION(BlueprintPure, Category = "IHPC|Panel")
	bool IsZoomedGraphVisible() const { return bZoomedGraphVisible; }

	/** Adds a new clip: a new card in the videos row AND a new row in Tagged Points. */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Panel")
	void AddSavedVideo(const FIHPCTaggedMoment& Moment);

	/** Adds a row to Tagged Points only (e.g. a bookmark). */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Panel")
	void AddTaggedPoint(const FIHPCTaggedMoment& Moment);

	/** The moments shown as video cards. */
	UFUNCTION(BlueprintPure, Category = "IHPC|Panel")
	TArray<FIHPCTaggedMoment> GetSavedVideos() const;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCPP_UW_IHPCGraphAccordion> GraphAccordion;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCPP_UW_IHPCVideosRow> VideosRow;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> InspectButton;

	/** Holder for the large graph (hidden until zoom is clicked). */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> ZoomedGraphContainer;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UCPP_UW_IHPCLineGraph> ZoomedGraph;

private:
	UFUNCTION() void HandleZoomRequested();
	UFUNCTION() void HandleInspectClicked();

	bool bZoomedGraphVisible = false;
};
