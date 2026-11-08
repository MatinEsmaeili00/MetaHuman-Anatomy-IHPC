// Timeline widget (WB_AmationTimeline): forwards Play / Pause / Reverse and the time slider to the
// UAnimationTimelineComp on whatever pawn the owning controller possesses.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "AnimationTimelineComp.h"
#include "WCPP_AnimationTimeline.generated.h"

class UButton;
class USlider;
class UTextBlock;

UCLASS()
class METAHUMANTEST_API UWCPP_AnimationTimeline : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Timeline on the possessed pawn (re-found automatically when the controller possesses another pawn). */
	UPROPERTY(BlueprintReadWrite, Category="Timeline")
	TObjectPtr<UAnimationTimelineComp> TimelineComp = nullptr;

	/** Tint for the button of the current play state. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Timeline")
	FLinearColor ActiveButtonColor = FLinearColor(0.25f, 0.65f, 1.f, 1.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Timeline")
	FLinearColor IdleButtonColor = FLinearColor::White;

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// BindWidget = auto-binds to the Designer widgets with the same name
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> PlayBtn;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> PauseBtn;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> ReverseBtn;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<USlider> Slider_Time;

	/** Optional label, e.g. "Gait cycle 42%  (0.41 s)". */
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UTextBlock> TimeText;

	UFUNCTION() void OnPlayClicked();
	UFUNCTION() void OnPauseClicked();
	UFUNCTION() void OnReverseClicked();
	UFUNCTION() void OnSliderValueChanged(float Value);
	UFUNCTION() void OnSliderCaptureBegin();
	UFUNCTION() void OnSliderCaptureEnd();
	UFUNCTION() void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);
	UFUNCTION() void HandlePlayStateChanged(ETimelinePlayState NewState);

	void BindToPawn(APawn* Pawn);
	void RefreshButtons();

private:
	bool bDragging = false;
	float RebindCooldown = 0.f;
};
