// CPP_UW_IHPCVideoCard.cpp - one saved-video card. See header.

#include "IHPC_UI/CPP_UW_IHPCVideoCard.h"

#include "Components/Button.h"
#include "Components/TextBlock.h"

void UCPP_UW_IHPCVideoCard::NativePreConstruct()
{
	Super::NativePreConstruct();
	ApplyMoment();       // so the designer preview shows the sample text + tick position
	SetSelected(false);
}

void UCPP_UW_IHPCVideoCard::NativeConstruct()
{
	Super::NativeConstruct();
	if (CardButton)
	{
		CardButton->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCVideoCard::HandleClicked);
	}
}

void UCPP_UW_IHPCVideoCard::SetMoment(const FIHPCTaggedMoment& InMoment)
{
	Moment = InMoment;
	ApplyMoment();
}

void UCPP_UW_IHPCVideoCard::ApplyMoment()
{
	if (TitleText) TitleText->SetText(Moment.Title);
	if (DateText)  DateText->SetText(Moment.DateText);
	if (TimeText)  TimeText->SetText(Moment.TimeText);

	if (SeverityTick)
	{
		// Severity 0 -> left end of the bar, 1 -> right end.
		SeverityTick->SetRenderTranslation(FVector2D(FMath::Clamp(Moment.Severity, 0.f, 1.f) * TickTravelWidth, 0.f));
	}
}

void UCPP_UW_IHPCVideoCard::SetSelected(bool bInSelected)
{
	if (SelectionHighlight)
	{
		SelectionHighlight->SetVisibility(bInSelected ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
	}
}

void UCPP_UW_IHPCVideoCard::HandleClicked()
{
	OnCardClicked.Broadcast(this);
}
