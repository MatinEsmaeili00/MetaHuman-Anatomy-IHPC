// CPP_UW_IHPCNavTab.cpp - one tab in the nav bar. See the header for the Figma mapping.

#include "IHPC_UI/CPP_UW_IHPCNavTab.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UCPP_UW_IHPCNavTab::SetTabIdAndDefaultLabel(EIHPCNavTab InTabId, const FText& DefaultLabel)
{
	TabId = InTabId;
	if (Label.IsEmpty())          // keep any text the designer typed in the Widget Blueprint
	{
		Label = DefaultLabel;
	}
	if (TabLabel)
	{
		TabLabel->SetText(Label);
	}
}

void UCPP_UW_IHPCNavTab::NativePreConstruct()
{
	Super::NativePreConstruct();

	if (TabLabel)
	{
		// If no font was chosen on the tab, use whatever font the TextBlock has in the designer.
		if (!NormalFont.HasValidFont())
		{
			NormalFont = TabLabel->GetFont();
		}
		if (!HighlightFont.HasValidFont())
		{
			HighlightFont = NormalFont;
		}
		if (!Label.IsEmpty())
		{
			TabLabel->SetText(Label);
		}
		// Scale around the centre of the text so it grows evenly in both directions.
		TabLabel->SetRenderTransformPivot(FVector2D(0.5f, 0.5f));
	}
	RefreshVisuals();
}

void UCPP_UW_IHPCNavTab::NativeConstruct()
{
	Super::NativeConstruct();

	if (TabButton)
	{
		// AddUniqueDynamic avoids double-binding if the widget is constructed more than once.
		TabButton->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCNavTab::HandleClicked);
		TabButton->OnHovered.AddUniqueDynamic(this, &UCPP_UW_IHPCNavTab::HandleHovered);
		TabButton->OnUnhovered.AddUniqueDynamic(this, &UCPP_UW_IHPCNavTab::HandleUnhovered);
	}
}

void UCPP_UW_IHPCNavTab::SetSelected(bool bInSelected)
{
	bSelected = bInSelected;
	RefreshVisuals();
}

void UCPP_UW_IHPCNavTab::HandleClicked()
{
	OnTabClicked.Broadcast(TabId);   // the HUD decides what selecting this tab means
}

void UCPP_UW_IHPCNavTab::HandleHovered()
{
	bHovered = true;
	RefreshVisuals();
}

void UCPP_UW_IHPCNavTab::HandleUnhovered()
{
	bHovered = false;
	RefreshVisuals();
}

void UCPP_UW_IHPCNavTab::RefreshVisuals()
{
	if (!TabLabel)
	{
		return;
	}

	const bool bHighlight = bSelected || bHovered;   // Figma Variant2 when hovered or selected

	TabLabel->SetFont(bHighlight ? HighlightFont : NormalFont);
	TabLabel->SetColorAndOpacity(bHighlight ? HighlightColor : NormalColor);
	TabLabel->SetRenderScale(FVector2D(bHighlight ? HighlightScale : 1.f));
}
