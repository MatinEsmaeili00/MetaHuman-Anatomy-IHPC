// CPP_UW_IHPCListRow.h
// -----------------------------------------------------------------------------
// One row in the "Active Muscles" or "Tagged Points" list of the graph accordion.
//
// Figma only shows these two accordion sections CLOSED, so there is no design for
// their contents yet. This row is a simple, on-style placeholder:
//   [coloured dot]  Primary text ............................ secondary text
// It builds its own widgets in code (no Widget Blueprint) so it can be created
// from C++ for every item in the list.
// -----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPP_UW_IHPCListRow.generated.h"

class UButton;
class UTextBlock;
class UBorder;
class UCPP_UW_IHPCListRow;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIHPCListRowClicked, UCPP_UW_IHPCListRow*, Row);

UCLASS()
class METAHUMANTEST_API UCPP_UW_IHPCListRow : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Fills in the row. Call right after CreateWidget. */
	void Setup(const FText& PrimaryText, const FText& SecondaryText, const FLinearColor& DotColor,
	           const FSlateFontInfo& Font, bool bClickable);

	/** Index of the item this row shows (set by whoever creates the row). */
	UPROPERTY(BlueprintReadOnly, Category = "IHPC|List")
	int32 RowIndex = INDEX_NONE;

	UPROPERTY(BlueprintAssignable, Category = "IHPC|List")
	FIHPCListRowClicked OnRowClicked;

protected:
	/** Makes sure our code-built widgets exist before Unreal builds the on-screen version. */
	virtual TSharedRef<SWidget> RebuildWidget() override;

private:
	void BuildTreeIfNeeded();

	UFUNCTION()
	void HandleClicked();

	UPROPERTY()
	TObjectPtr<UButton> RowButton;

	UPROPERTY()
	TObjectPtr<UBorder> Dot;

	UPROPERTY()
	TObjectPtr<UTextBlock> Primary;

	UPROPERTY()
	TObjectPtr<UTextBlock> Secondary;
};
