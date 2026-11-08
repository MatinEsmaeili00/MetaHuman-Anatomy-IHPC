#include "WCPP_AnimationTimeline.h"

#include "AnimationTimelineComp.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UWCPP_AnimationTimeline::NativeConstruct()
{
	Super::NativeConstruct();

	// AddUnique: NativeConstruct runs again if the widget is removed and re-added to the viewport.
	PlayBtn->OnClicked.AddUniqueDynamic(this, &UWCPP_AnimationTimeline::OnPlayClicked);
	PauseBtn->OnClicked.AddUniqueDynamic(this, &UWCPP_AnimationTimeline::OnPauseClicked);
	ReverseBtn->OnClicked.AddUniqueDynamic(this, &UWCPP_AnimationTimeline::OnReverseClicked);
	Slider_Time->OnValueChanged.AddUniqueDynamic(this, &UWCPP_AnimationTimeline::OnSliderValueChanged);
	Slider_Time->OnMouseCaptureBegin.AddUniqueDynamic(this, &UWCPP_AnimationTimeline::OnSliderCaptureBegin);
	Slider_Time->OnMouseCaptureEnd.AddUniqueDynamic(this, &UWCPP_AnimationTimeline::OnSliderCaptureEnd);
	Slider_Time->SetMinValue(0.f);
	Slider_Time->SetMaxValue(1.f);

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &UWCPP_AnimationTimeline::HandlePossessedPawnChanged);
		BindToPawn(PC->GetPawn());
	}
}

void UWCPP_AnimationTimeline::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &UWCPP_AnimationTimeline::HandlePossessedPawnChanged);
	}
	BindToPawn(nullptr);
	Super::NativeDestruct();
}

void UWCPP_AnimationTimeline::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	BindToPawn(NewPawn);
}

void UWCPP_AnimationTimeline::BindToPawn(APawn* Pawn)
{
	if (IsValid(TimelineComp))
	{
		TimelineComp->OnPlayStateChanged.RemoveDynamic(this, &UWCPP_AnimationTimeline::HandlePlayStateChanged);
	}

	// The pawn decides what the buttons do: a pawn without a timeline greys the widget out.
	TimelineComp = Pawn ? Pawn->FindComponentByClass<UAnimationTimelineComp>() : nullptr;
	SetIsEnabled(TimelineComp != nullptr);

	if (TimelineComp)
	{
		TimelineComp->OnPlayStateChanged.AddUniqueDynamic(this, &UWCPP_AnimationTimeline::HandlePlayStateChanged);
		Slider_Time->SetValue(TimelineComp->GetNormalizedTime());
	}
	RefreshButtons();
}

void UWCPP_AnimationTimeline::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!IsValid(TimelineComp))
	{
		// The pawn's components can be re-created (e.g. its construction script re-runs): look again, twice a second.
		RebindCooldown -= InDeltaTime;
		if (RebindCooldown <= 0.f)
		{
			RebindCooldown = 0.5f;
			BindToPawn(GetOwningPlayerPawn());
		}
		return;
	}

	// The component owns the clock; the slider just shows it (SetValue doesn't fire OnValueChanged).
	if (!bDragging)
	{
		Slider_Time->SetValue(TimelineComp->GetNormalizedTime());
	}

	if (TimeText)
	{
		const float Cycle = TimelineComp->GetCycleLength();
		TimeText->SetText(FText::FromString(FString::Printf(TEXT("%3.0f%%   %.2f / %.2f s"),
			TimelineComp->GetNormalizedTime() * 100.f, TimelineComp->GetNormalizedTime() * Cycle, Cycle)));
	}
}

void UWCPP_AnimationTimeline::OnPlayClicked()
{
	if (TimelineComp)
	{
		TimelineComp->PlayForward();
	}
}

void UWCPP_AnimationTimeline::OnPauseClicked()
{
	if (TimelineComp)
	{
		TimelineComp->Pause();
	}
}

void UWCPP_AnimationTimeline::OnReverseClicked()
{
	if (TimelineComp)
	{
		TimelineComp->PlayReverse();
	}
}

void UWCPP_AnimationTimeline::OnSliderValueChanged(float Value)
{
	if (TimelineComp)
	{
		TimelineComp->SetNormalizedTime(Value);
	}
}

void UWCPP_AnimationTimeline::OnSliderCaptureBegin()
{
	bDragging = true;
	if (TimelineComp)
	{
		TimelineComp->BeginScrub();
	}
}

void UWCPP_AnimationTimeline::OnSliderCaptureEnd()
{
	bDragging = false;
	if (TimelineComp)
	{
		TimelineComp->EndScrub();
	}
}

void UWCPP_AnimationTimeline::HandlePlayStateChanged(ETimelinePlayState NewState)
{
	RefreshButtons();
}

void UWCPP_AnimationTimeline::RefreshButtons()
{
	const ETimelinePlayState State = TimelineComp ? TimelineComp->GetPlayState() : ETimelinePlayState::Paused;
	PlayBtn->SetBackgroundColor(State == ETimelinePlayState::Forward ? ActiveButtonColor : IdleButtonColor);
	PauseBtn->SetBackgroundColor(State == ETimelinePlayState::Paused ? ActiveButtonColor : IdleButtonColor);
	ReverseBtn->SetBackgroundColor(State == ETimelinePlayState::Reverse ? ActiveButtonColor : IdleButtonColor);
}
