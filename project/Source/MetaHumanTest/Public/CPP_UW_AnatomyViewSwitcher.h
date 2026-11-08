// Anatomy widget (WB_AnatomyViewSwitcher): the slider peels the character from the skin down to the
// skeleton; the MetaHuman / Muscle / Skeleton buttons animate to that layer. All the work happens in the
// UAnatomyViewComp on the possessed pawn - this widget only forwards input and mirrors the current depth.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPP_UW_AnatomyViewSwitcher.generated.h"

class UButton;
class USlider;
class UAnatomyViewComp;

UCLASS()
class METAHUMANTEST_API UCPP_UW_AnatomyViewSwitcher : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Anatomy component on the possessed pawn (re-found when the controller possesses another pawn). */
	UPROPERTY(BlueprintReadOnly, Category="Anatomy")
	TObjectPtr<UAnatomyViewComp> AnatomyComp = nullptr;

	/** The slider in WB_AnatomyViewSwitcher is rotated so 1 is at the top, next to the MetaHuman button:
	 *  top = skin (reveal depth 0), bottom = skeleton (reveal depth 1). Untick if your slider runs the other way. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Anatomy")
	bool bSliderMaxIsSkin = true;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// These names must match the widget names in the Designer.
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<USlider> AnatomyViewSwitcher_Slider;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> AnatomyMetaHuman_Btn;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> AnatomyMuscle_Btn;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> AnatomySkeleton_Btn;

	UFUNCTION() void OnSliderValueChanged(float Value);
	UFUNCTION() void OnSliderCaptureBegin();
	UFUNCTION() void OnSliderCaptureEnd();
	UFUNCTION() void OnMetaHumanClicked();
	UFUNCTION() void OnMuscleClicked();
	UFUNCTION() void OnSkeletonClicked();
	UFUNCTION() void HandleRevealDepthChanged(float RevealDepth);
	UFUNCTION() void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	void BindToPawn(APawn* Pawn);
	float SliderToDepth(float SliderValue) const;
	float DepthToSlider(float Depth) const;

private:
	bool bDragging = false;
	float RebindCooldown = 0.f;
};
