// CPP_UW_IHPCListRow.cpp - one list row, built entirely in code. See header.

#include "IHPC_UI/CPP_UW_IHPCListRow.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/TextBlock.h"

namespace IHPCListRow
{
	/** A brush that draws a rounded rectangle in one colour (used for hover + the dot). */
	static FSlateBrush MakeRoundedBrush(const FLinearColor& Color, float Radius)
	{
		FSlateBrush Brush;
		Brush.DrawAs = ESlateBrushDrawType::RoundedBox;
		Brush.TintColor = FSlateColor(Color);
		Brush.OutlineSettings.RoundingType = ESlateBrushRoundingType::FixedRadius;
		Brush.OutlineSettings.CornerRadii = FVector4(Radius, Radius, Radius, Radius);
		Brush.OutlineSettings.Width = 0.f;
		return Brush;
	}
}

void UCPP_UW_IHPCListRow::BuildTreeIfNeeded()
{
	if (!WidgetTree || WidgetTree->RootWidget)
	{
		return;   // already built (or not initialised yet)
	}

	// Root: a flat button. Transparent normally, a faint white rounded box on hover.
	RowButton = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), TEXT("RowButton"));
	FSlateBrush Invisible;
	Invisible.DrawAs = ESlateBrushDrawType::NoDrawType;
	FButtonStyle Style;
	Style.SetNormal(Invisible)
	     .SetHovered(IHPCListRow::MakeRoundedBrush(FLinearColor(1.f, 1.f, 1.f, 0.08f), 6.f))
	     .SetPressed(IHPCListRow::MakeRoundedBrush(FLinearColor(1.f, 1.f, 1.f, 0.14f), 6.f))
	     .SetDisabled(Invisible)
	     .SetNormalPadding(FMargin(0.f))
	     .SetPressedPadding(FMargin(0.f));
	RowButton->SetStyle(Style);
	WidgetTree->RootWidget = RowButton;

	// Inside the button: [dot] [primary text (fills)] [secondary text]
	UHorizontalBox* Row = WidgetTree->ConstructWidget<UHorizontalBox>(UHorizontalBox::StaticClass(), TEXT("RowBox"));
	if (UButtonSlot* ButtonSlot = Cast<UButtonSlot>(RowButton->AddChild(Row)))
	{
		ButtonSlot->SetPadding(FMargin(10.f, 6.f));
		ButtonSlot->SetHorizontalAlignment(HAlign_Fill);
		ButtonSlot->SetVerticalAlignment(VAlign_Center);
	}

	USizeBox* DotSize = WidgetTree->ConstructWidget<USizeBox>(USizeBox::StaticClass(), TEXT("DotSize"));
	DotSize->SetWidthOverride(8.f);
	DotSize->SetHeightOverride(8.f);
	Dot = WidgetTree->ConstructWidget<UBorder>(UBorder::StaticClass(), TEXT("Dot"));
	DotSize->AddChild(Dot);
	if (UHorizontalBoxSlot* DotSlot = Row->AddChildToHorizontalBox(DotSize))
	{
		DotSlot->SetVerticalAlignment(VAlign_Center);
		DotSlot->SetPadding(FMargin(0.f, 0.f, 10.f, 0.f));
	}

	Primary = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Primary"));
	if (UHorizontalBoxSlot* PrimarySlot = Row->AddChildToHorizontalBox(Primary))
	{
		PrimarySlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		PrimarySlot->SetVerticalAlignment(VAlign_Center);
	}

	Secondary = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("Secondary"));
	if (UHorizontalBoxSlot* SecondarySlot = Row->AddChildToHorizontalBox(Secondary))
	{
		SecondarySlot->SetVerticalAlignment(VAlign_Center);
		SecondarySlot->SetPadding(FMargin(8.f, 0.f, 0.f, 0.f));
	}

	RowButton->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCListRow::HandleClicked);
}

void UCPP_UW_IHPCListRow::Setup(const FText& PrimaryText, const FText& SecondaryText, const FLinearColor& DotColor,
                                const FSlateFontInfo& Font, bool bClickable)
{
	BuildTreeIfNeeded();
	if (!RowButton)
	{
		return;
	}

	Primary->SetText(PrimaryText);
	Primary->SetFont(Font);
	Primary->SetColorAndOpacity(FSlateColor(FLinearColor::White));

	FSlateFontInfo SmallFont = Font;
	SmallFont.Size = FMath::Max(Font.Size - 3, 8);
	Secondary->SetText(SecondaryText);
	Secondary->SetFont(SmallFont);
	Secondary->SetColorAndOpacity(FSlateColor(FLinearColor(1.f, 1.f, 1.f, 0.65f)));
	Secondary->SetVisibility(SecondaryText.IsEmpty() ? ESlateVisibility::Collapsed : ESlateVisibility::HitTestInvisible);

	Dot->SetBrush(IHPCListRow::MakeRoundedBrush(DotColor, 4.f));

	// Rows that do nothing when clicked should not light up on hover either.
	RowButton->SetVisibility(bClickable ? ESlateVisibility::Visible : ESlateVisibility::HitTestInvisible);
}

TSharedRef<SWidget> UCPP_UW_IHPCListRow::RebuildWidget()
{
	BuildTreeIfNeeded();
	return Super::RebuildWidget();
}

void UCPP_UW_IHPCListRow::HandleClicked()
{
	OnRowClicked.Broadcast(this);
}
