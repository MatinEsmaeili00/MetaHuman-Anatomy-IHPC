// Muscle info widget (WB_MuscleInfo): a small name tooltip that follows the mouse while a muscle is hovered, and an
// info popup when one is clicked. It only listens to the UAnatomyPickerComp on the possessed pawn.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPP_UW_MuscleInfo.generated.h"

class UAnatomyPickerComp;
class UButton;
class UTextBlock;
class UWidget;

UCLASS()
class METAHUMANTEST_API UCPP_UW_MuscleInfo : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Muscle Info")
	TObjectPtr<UAnatomyPickerComp> Picker = nullptr;

	/** Tooltip offset from the mouse cursor (viewport units). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Muscle Info")
	FVector2D TooltipOffset = FVector2D(18.0, 14.0);

protected:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// Tooltip: a panel (in a Canvas Panel slot) with the muscle name.
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidget> TooltipBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> MuscleNameText;

	// Popup
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UWidget> PopupBox;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> PopupTitle;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> PopupRegion;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> PopupFunction;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> PopupRunning;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UTextBlock> PopupInjury;

	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<UButton> PopupCloseBtn;

	UFUNCTION() void HandleHoverChanged(int32 MuscleId);
	UFUNCTION() void HandleSelectionChanged(int32 MuscleId);
	UFUNCTION() void HandleCloseClicked();
	UFUNCTION() void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	void BindToPawn(APawn* Pawn);

private:
	float RebindCooldown = 0.f;
};
