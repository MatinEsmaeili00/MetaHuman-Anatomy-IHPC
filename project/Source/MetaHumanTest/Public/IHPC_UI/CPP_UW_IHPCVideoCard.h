// CPP_UW_IHPCVideoCard.h
// -----------------------------------------------------------------------------
// One saved-video card. Figma: each card inside "videos row".
//
//   |====green====|=====yellow=====|====red====|   <- severity bar, white tick shows severity
//   | [film icon] | Soleus Muscle (R)           |
//   |             | Jan 4th, 2026               |
//   |             | 13:55 p.m.                  |
//
// The card picture (T_IHPC_VideoCard_BG) is the Figma card art with the text and
// tick removed; the text and tick are real widgets filled from an FIHPCTaggedMoment.
// Clicking a card tells the videos row, which jumps the avatar animation there.
//
// Widget Blueprint: /Game/UserInterface/IHPC/Widgets/WBP_IHPC_VideoCard
// -----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IHPC_UI/IHPCTypes.h"
#include "CPP_UW_IHPCVideoCard.generated.h"

class UButton;
class UTextBlock;
class UCPP_UW_IHPCVideoCard;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIHPCVideoCardClicked, UCPP_UW_IHPCVideoCard*, Card);

UCLASS(Abstract)
class METAHUMANTEST_API UCPP_UW_IHPCVideoCard : public UUserWidget
{
	GENERATED_BODY()

public:
	/** What this card shows. Editable in the designer for previewing. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IHPC|Card")
	FIHPCTaggedMoment Moment;

	/** How far (px) the tick can travel: card width (240) minus tick width (3). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Layout")
	float TickTravelWidth = 237.f;

	UPROPERTY(BlueprintAssignable, Category = "IHPC|Card")
	FIHPCVideoCardClicked OnCardClicked;

	UFUNCTION(BlueprintCallable, Category = "IHPC|Card")
	void SetMoment(const FIHPCTaggedMoment& InMoment);

	UFUNCTION(BlueprintPure, Category = "IHPC|Card")
	FIHPCTaggedMoment GetMoment() const { return Moment; }

	/** Shows the highlight outline around the chosen card. */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Card")
	void SetSelected(bool bInSelected);

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> CardButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> TitleText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> DateText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> TimeText;

	/** The small white marker on the severity bar. Placed at X = 0 in the Blueprint; moved in code. */
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> SeverityTick;

	/** Optional outline shown when the card is selected. */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> SelectionHighlight;

private:
	UFUNCTION()
	void HandleClicked();

	/** Copies Moment into the text blocks and moves the tick. */
	void ApplyMoment();
};
