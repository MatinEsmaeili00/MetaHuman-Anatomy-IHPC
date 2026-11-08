// CPP_UW_IHPCVideosRow.cpp - Figma "videos row". See header.

#include "IHPC_UI/CPP_UW_IHPCVideosRow.h"

#include "IHPC_UI/CPP_UW_IHPCVideoCard.h"
#include "IHPC_UI/IHPCUILibrary.h"

#include "Components/ScrollBox.h"
#include "Components/ScrollBoxSlot.h"
#include "Components/Slider.h"

#define LOCTEXT_NAMESPACE "IHPCVideosRow"

UCPP_UW_IHPCVideosRow::UCPP_UW_IHPCVideosRow(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// The five sample cards shown in Figma. Severity values are where the white tick sits
	// on each Figma card (Soleus ~0.17, Neck ~0.41). Times are spread over the animation.
	const FText Date = LOCTEXT("SampleDate", "Jan 4th, 2026");
	const FText Time = LOCTEXT("SampleTime", "13:55 p.m.");
	const FText Soleus = LOCTEXT("Soleus", "Soleus Muscle (R)");
	const FText Neck = LOCTEXT("Neck", "Neck Axis (C2)");

	Moments = {
		FIHPCTaggedMoment(Soleus, Date, Time, 0.17f, 0.10f),
		FIHPCTaggedMoment(Neck,   Date, Time, 0.41f, 0.30f),
		FIHPCTaggedMoment(Soleus, Date, Time, 0.17f, 0.50f),
		FIHPCTaggedMoment(Soleus, Date, Time, 0.17f, 0.70f),
		FIHPCTaggedMoment(Neck,   Date, Time, 0.41f, 0.90f),
	};
}

void UCPP_UW_IHPCVideosRow::NativePreConstruct()
{
	Super::NativePreConstruct();
	RebuildCards();   // also runs in the designer, so the row can be previewed
}

void UCPP_UW_IHPCVideosRow::NativeConstruct()
{
	Super::NativeConstruct();

	if (ScrollSlider)
	{
		ScrollSlider->OnValueChanged.AddUniqueDynamic(this, &UCPP_UW_IHPCVideosRow::HandleSliderChanged);
		ScrollSlider->OnMouseCaptureBegin.AddUniqueDynamic(this, &UCPP_UW_IHPCVideosRow::HandleSliderCaptureBegin);
		ScrollSlider->OnMouseCaptureEnd.AddUniqueDynamic(this, &UCPP_UW_IHPCVideosRow::HandleSliderCaptureEnd);
	}
	RebuildCards();
}

void UCPP_UW_IHPCVideosRow::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);
	SyncSliderToScroll();   // mouse-wheel scrolling moves the indicator too
}

// ============================================================================
// Cards
// ============================================================================

void UCPP_UW_IHPCVideosRow::SetMoments(const TArray<FIHPCTaggedMoment>& NewMoments)
{
	Moments = NewMoments;
	RebuildCards();
}

void UCPP_UW_IHPCVideosRow::AddMoment(const FIHPCTaggedMoment& NewMoment)
{
	Moments.Add(NewMoment);
	RebuildCards();
}

void UCPP_UW_IHPCVideosRow::RebuildCards()
{
	if (!CardScrollBox || !CardClass)
	{
		return;
	}

	CardScrollBox->ClearChildren();
	Cards.Reset();
	SelectedCard = nullptr;

	for (int32 i = 0; i < Moments.Num(); ++i)
	{
		UCPP_UW_IHPCVideoCard* Card = CreateWidget<UCPP_UW_IHPCVideoCard>(this, CardClass);
		if (!Card)
		{
			continue;
		}
		Card->SetMoment(Moments[i]);
		Card->OnCardClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCVideosRow::HandleCardClicked);

		if (UScrollBoxSlot* CardSlot = Cast<UScrollBoxSlot>(CardScrollBox->AddChild(Card)))
		{
			// Space after every card except the last one.
			const float Right = (i < Moments.Num() - 1) ? CardSpacing : 0.f;
			CardSlot->SetPadding(FMargin(0.f, 0.f, Right, 0.f));
			CardSlot->SetVerticalAlignment(VAlign_Top);
		}
		Cards.Add(Card);
	}
}

void UCPP_UW_IHPCVideosRow::HandleCardClicked(UCPP_UW_IHPCVideoCard* Card)
{
	if (!Card)
	{
		return;
	}

	// Only one card is highlighted at a time.
	if (SelectedCard)
	{
		SelectedCard->SetSelected(false);
	}
	SelectedCard = Card;
	SelectedCard->SetSelected(true);

	const FIHPCTaggedMoment Moment = Card->GetMoment();
	if (bSeekTimelineOnSelect)
	{
		UIHPCUILibrary::SeekAvatarTimeline(this, Moment.NormalizedTime);
	}
	OnMomentSelected.Broadcast(Moment);
}

// ============================================================================
// Scroll indicator <-> scroll box
// ============================================================================

void UCPP_UW_IHPCVideosRow::HandleSliderChanged(float Value)
{
	// Slider 0..1 -> scroll position 0..end.
	if (CardScrollBox)
	{
		CardScrollBox->SetScrollOffset(Value * CardScrollBox->GetScrollOffsetOfEnd());
	}
}

void UCPP_UW_IHPCVideosRow::HandleSliderCaptureBegin() { bDraggingSlider = true; }
void UCPP_UW_IHPCVideosRow::HandleSliderCaptureEnd()   { bDraggingSlider = false; }

void UCPP_UW_IHPCVideosRow::SyncSliderToScroll()
{
	if (!CardScrollBox || !ScrollSlider || bDraggingSlider)
	{
		return;   // while dragging, the slider is in charge
	}

	const float End = CardScrollBox->GetScrollOffsetOfEnd();
	if (End <= 1.f)
	{
		// Everything fits - nothing to scroll, so hide the indicator thumb.
		ScrollSlider->SetVisibility(ESlateVisibility::Hidden);
		return;
	}

	ScrollSlider->SetVisibility(ESlateVisibility::Visible);
	ScrollSlider->SetValue(FMath::Clamp(CardScrollBox->GetScrollOffset() / End, 0.f, 1.f));
}

#undef LOCTEXT_NAMESPACE
