// CPP_UW_IHPCNavTab.h
// -----------------------------------------------------------------------------
// ONE tab in the drop-down nav bar.
// Figma: the components "Profile", "Progress", "Saved Videos", "History & Timeline"
// and "Settings". Each Figma component has two variants:
//   Default  : Inter Medium 20, white                 -> "normal" look
//   Variant2 : Inter Bold 24, green #B3CA7F           -> "highlight" look
// We show the highlight look while the mouse is over the tab OR the tab is selected.
//
// Widget Blueprint: /Game/UserInterface/IHPC/Widgets/WBP_IHPC_NavTab
//   It must contain a Button named "TabButton" with a TextBlock named "TabLabel" inside.
// -----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IHPC_UI/IHPCTypes.h"
#include "CPP_UW_IHPCNavTab.generated.h"

class UButton;
class UTextBlock;

/** Fired when a tab is clicked. The HUD listens to this. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIHPCNavTabClicked, EIHPCNavTab, Tab);

UCLASS(Abstract)
class METAHUMANTEST_API UCPP_UW_IHPCNavTab : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Which tab this is. The HUD sets this automatically for its five tabs. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IHPC|Tab")
	EIHPCNavTab TabId = EIHPCNavTab::None;

	/** Text shown on the tab, e.g. "Saved Videos". */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IHPC|Tab")
	FText Label;

	/** Figma "Default" variant font (Inter Medium 20 in Figma). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	FSlateFontInfo NormalFont;

	/** Figma "Variant2" font (Inter Bold in Figma). Keep the same size as NormalFont;
	 *  the extra size comes from HighlightScale so the other tabs do not jump around. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	FSlateFontInfo HighlightFont;

	/** Figma Variant2 is 24 px vs 20 px -> 1.2x. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	float HighlightScale = 1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	FSlateColor NormalColor = FSlateColor(FLinearColor::White);

	/** #B3CA7F in Figma. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	FSlateColor HighlightColor = FSlateColor(FLinearColor(FColor(0xB3, 0xCA, 0x7F)));

	/** Broadcast when the user clicks this tab. */
	UPROPERTY(BlueprintAssignable, Category = "IHPC|Tab")
	FIHPCNavTabClicked OnTabClicked;

	/** Selected tabs stay in the highlight look. */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Tab")
	void SetSelected(bool bInSelected);

	UFUNCTION(BlueprintPure, Category = "IHPC|Tab")
	bool IsSelected() const { return bSelected; }

	/** Used by the HUD to give each of its tabs the right id and default text. */
	void SetTabIdAndDefaultLabel(EIHPCNavTab InTabId, const FText& DefaultLabel);

protected:
	virtual void NativePreConstruct() override;   // runs in the UMG designer too, so the label shows while editing
	virtual void NativeConstruct() override;      // runs once in game: hook up the button events

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> TabButton;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UTextBlock> TabLabel;

private:
	UFUNCTION()
	void HandleClicked();

	UFUNCTION()
	void HandleHovered();

	UFUNCTION()
	void HandleUnhovered();

	/** Applies the normal or highlight look depending on bSelected / bHovered. */
	void RefreshVisuals();

	bool bSelected = false;
	bool bHovered = false;
};
