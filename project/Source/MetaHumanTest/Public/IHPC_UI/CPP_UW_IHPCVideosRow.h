// CPP_UW_IHPCVideosRow.h
// -----------------------------------------------------------------------------
// The horizontally scrolling row of saved-video cards. Figma: "videos row" (858 x 160).
//
//   ( ====                                       )   <- thin scroll indicator (Figma: 842x15 track,
//   [card] [card] [card] [card] [card] ...  ->        62x9 light-blue thumb). Drag it to scroll.
//
// Figma variants: Default = scrolled to the start, Variant2 = scrolled to the end.
// Mouse wheel / dragging the indicator scrolls the cards.
// Clicking a card selects it and jumps the avatar animation to that moment.
//
// Widget Blueprint: /Game/UserInterface/IHPC/Widgets/WBP_IHPC_VideosRow
//   Needs a ScrollBox "CardScrollBox" (horizontal) and a Slider "ScrollSlider".
// -----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IHPC_UI/IHPCTypes.h"
#include "CPP_UW_IHPCVideosRow.generated.h"

class UScrollBox;
class USlider;
class UCPP_UW_IHPCVideoCard;

UCLASS(Abstract)
class METAHUMANTEST_API UCPP_UW_IHPCVideosRow : public UUserWidget
{
	GENERATED_BODY()

public:
	UCPP_UW_IHPCVideosRow(const FObjectInitializer& ObjectInitializer);

	/** The saved videos to show, left to right. Defaults = the 5 sample cards from Figma. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IHPC|Data")
	TArray<FIHPCTaggedMoment> Moments;

	/** Widget Blueprint used for each card (WBP_IHPC_VideoCard). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IHPC|Setup")
	TSubclassOf<UCPP_UW_IHPCVideoCard> CardClass;

	/** Gap between cards (px). Figma cards are ~263 px apart and 240 px wide. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IHPC|Layout")
	float CardSpacing = 23.f;

	/** Jump the avatar animation to a card's moment when it is clicked. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IHPC|Behaviour")
	bool bSeekTimelineOnSelect = true;

	/** Fired when the user picks a card. */
	UPROPERTY(BlueprintAssignable, Category = "IHPC|Events")
	FIHPCMomentEvent OnMomentSelected;

	/** Replace all cards. */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Videos")
	void SetMoments(const TArray<FIHPCTaggedMoment>& NewMoments);

	/** Add one card at the end (e.g. after recording a new clip). */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Videos")
	void AddMoment(const FIHPCTaggedMoment& NewMoment);

	UFUNCTION(BlueprintPure, Category = "IHPC|Videos")
	TArray<FIHPCTaggedMoment> GetMoments() const { return Moments; }

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UScrollBox> CardScrollBox;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> ScrollSlider;

private:
	UFUNCTION() void HandleCardClicked(UCPP_UW_IHPCVideoCard* Card);
	UFUNCTION() void HandleSliderChanged(float Value);
	UFUNCTION() void HandleSliderCaptureBegin();
	UFUNCTION() void HandleSliderCaptureEnd();

	/** Deletes and re-creates every card from Moments. */
	void RebuildCards();

	/** Moves the scroll indicator to match the scroll position (and hides it if nothing to scroll). */
	void SyncSliderToScroll();

	UPROPERTY()
	TArray<TObjectPtr<UCPP_UW_IHPCVideoCard>> Cards;

	UPROPERTY()
	TObjectPtr<UCPP_UW_IHPCVideoCard> SelectedCard;

	bool bDraggingSlider = false;
};
