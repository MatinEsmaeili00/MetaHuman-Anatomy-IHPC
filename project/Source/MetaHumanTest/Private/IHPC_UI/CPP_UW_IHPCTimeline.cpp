// CPP_UW_IHPCTimeline.cpp - the playback / timeline bar. See the header for what each button does.

#include "IHPC_UI/CPP_UW_IHPCTimeline.h"

#include "IHPC_UI/IHPCUILibrary.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/ProgressBar.h"
#include "Components/Slider.h"
#include "Components/TextBlock.h"
#include "Engine/Texture2D.h"

#define LOCTEXT_NAMESPACE "IHPCTimeline"

UCPP_UW_IHPCTimeline::UCPP_UW_IHPCTimeline(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RecordingTint = UIHPCUILibrary::FigmaColor(TEXT("FF6B6B"));
	RecordingTitleFormat = LOCTEXT("RecordingTitle", "Recording {0}");
	BookmarkTitleFormat = LOCTEXT("BookmarkTitle", "Bookmark {0}");
}

// ============================================================================
// Setup
// ============================================================================

void UCPP_UW_IHPCTimeline::NativePreConstruct()
{
	Super::NativePreConstruct();

	SpeedIndex = SpeedSteps.Num() > 0 ? FMath::Clamp(DefaultSpeedIndex, 0, SpeedSteps.Num() - 1) : 0;
	ShowSpeed(GetSpeed());

	if (IsDesignTime())
	{
		// In the UMG designer: knob where Figma draws it, and the three Figma marker positions.
		ShowTime(DesignerPreviewTime);
	}
	RebuildMarkers();

	if (RecordingIndicator)
	{
		RecordingIndicator->SetVisibility(ESlateVisibility::Hidden);
	}
}

void UCPP_UW_IHPCTimeline::NativeConstruct()
{
	Super::NativeConstruct();

	// One handler per button.
	if (RecordButton)      RecordButton->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCTimeline::HandleRecord);
	if (RestartButton)     RestartButton->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCTimeline::HandleRestart);
	if (PauseButton)       PauseButton->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCTimeline::HandlePause);
	if (SlowDownButton)    SlowDownButton->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCTimeline::HandleSlowDown);
	if (SpeedUpButton)     SpeedUpButton->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCTimeline::HandleSpeedUp);
	if (PlayButton)        PlayButton->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCTimeline::HandlePlay);
	if (FastForwardButton) FastForwardButton->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCTimeline::HandleFastForward);
	if (BookmarkButton)    BookmarkButton->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCTimeline::HandleBookmark);

	// The invisible slider over the track does the scrubbing.
	if (ScrubSlider)
	{
		ScrubSlider->OnValueChanged.AddUniqueDynamic(this, &UCPP_UW_IHPCTimeline::HandleScrubChanged);
		ScrubSlider->OnMouseCaptureBegin.AddUniqueDynamic(this, &UCPP_UW_IHPCTimeline::HandleScrubBegin);
		ScrubSlider->OnMouseCaptureEnd.AddUniqueDynamic(this, &UCPP_UW_IHPCTimeline::HandleScrubEnd);
	}

	// Start at the default speed (1x unless changed in the Blueprint).
	SetSpeedIndex(SpeedIndex);
	RebuildMarkers();
}

void UCPP_UW_IHPCTimeline::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 1. Follow the animation (unless the user is dragging the knob).
	if (!bScrubbing)
	{
		const float Time = GetCurrentTime();
		ShowTime(Time);
		if (ScrubSlider)
		{
			ScrubSlider->SetValue(Time);   // SetValue does not fire OnValueChanged
		}
	}

	// 2. Keep the "x1.5" label right even if something else changes the speed.
	const float Speed = UIHPCUILibrary::GetAvatarPlaySpeed(this);
	if (Speed > 0.f && !FMath::IsNearlyEqual(Speed, ShownSpeed, 0.001f))
	{
		ShowSpeed(Speed);
	}

	// 3. Pulse the red dot while recording.
	if (bRecording && RecordingIndicator)
	{
		PulseClock += InDeltaTime;
		RecordingIndicator->SetRenderOpacity(0.55f + 0.45f * FMath::Sin(PulseClock * 6.f));
	}
}

// ============================================================================
// Showing the time / speed
// ============================================================================

float UCPP_UW_IHPCTimeline::GetCurrentTime() const
{
	return FMath::Max(UIHPCUILibrary::GetAvatarTimelineTime(this), 0.f);   // -1 (no timeline) -> 0
}

void UCPP_UW_IHPCTimeline::ShowTime(float Time)
{
	Time = FMath::Clamp(Time, 0.f, 1.f);
	if (TrackProgress)
	{
		TrackProgress->SetPercent(Time);   // dark (played) part grows from the left
	}
	if (ThumbGroup)
	{
		// The knob is placed centred on the START of the track in the Blueprint; slide it along.
		ThumbGroup->SetRenderTranslation(FVector2D(Time * TrackLength, 0.f));
	}
}

void UCPP_UW_IHPCTimeline::ShowSpeed(float Speed)
{
	ShownSpeed = Speed;
	if (SpeedText)
	{
		FNumberFormattingOptions Format;
		Format.MinimumFractionalDigits = 0;
		Format.MaximumFractionalDigits = 2;
		SpeedText->SetText(FText::Format(LOCTEXT("SpeedFormat", "x{0}"), FText::AsNumber(Speed, &Format)));   // "x1.5"
	}
}

// ============================================================================
// Buttons
// ============================================================================

void UCPP_UW_IHPCTimeline::HandleRecord()      { ToggleRecording(); }
void UCPP_UW_IHPCTimeline::HandleRestart()     { Restart(); }
void UCPP_UW_IHPCTimeline::HandlePause()       { Pause(); }
void UCPP_UW_IHPCTimeline::HandleSlowDown()    { SlowDown(); }
void UCPP_UW_IHPCTimeline::HandleSpeedUp()     { SpeedUp(); }
void UCPP_UW_IHPCTimeline::HandlePlay()        { Play(); }
void UCPP_UW_IHPCTimeline::HandleFastForward() { FastForwardToNextMarker(); }
void UCPP_UW_IHPCTimeline::HandleBookmark()    { AddBookmark(); }

void UCPP_UW_IHPCTimeline::Play()
{
	UIHPCUILibrary::PlayAvatar(this, /*bRestartIfAtEnd*/ true);
}

void UCPP_UW_IHPCTimeline::Pause()
{
	UIHPCUILibrary::PauseAvatar(this);
}

void UCPP_UW_IHPCTimeline::Restart()
{
	// Back to the start; if it was playing it keeps playing from there.
	UIHPCUILibrary::SeekAvatarTimeline(this, 0.f, /*bPauseAfterSeek*/ false);
}

void UCPP_UW_IHPCTimeline::SlowDown()
{
	SetSpeedIndex(SpeedIndex - 1);
}

void UCPP_UW_IHPCTimeline::SpeedUp()
{
	SetSpeedIndex(SpeedIndex + 1);
}

void UCPP_UW_IHPCTimeline::SetSpeedIndex(int32 Index)
{
	if (SpeedSteps.Num() == 0)
	{
		return;
	}
	SpeedIndex = FMath::Clamp(Index, 0, SpeedSteps.Num() - 1);
	const float Speed = SpeedSteps[SpeedIndex];
	UIHPCUILibrary::SetAvatarPlaySpeed(this, Speed);
	ShowSpeed(Speed);
	OnSpeedChanged.Broadcast(Speed);
}

float UCPP_UW_IHPCTimeline::GetSpeed() const
{
	return SpeedSteps.IsValidIndex(SpeedIndex) ? SpeedSteps[SpeedIndex] : 1.f;
}

void UCPP_UW_IHPCTimeline::FastForwardToNextMarker()
{
	const float Now = GetCurrentTime();

	// Find the closest marker that is still ahead of us.
	float Next = 1.f;   // no marker ahead -> go to the end
	for (const FIHPCTaggedMoment& Marker : Markers)
	{
		if (Marker.NormalizedTime > Now + 0.001f && Marker.NormalizedTime < Next)
		{
			Next = Marker.NormalizedTime;
		}
	}
	UIHPCUILibrary::SeekAvatarTimeline(this, Next, /*bPauseAfterSeek*/ false);
}

void UCPP_UW_IHPCTimeline::AddBookmark()
{
	const FIHPCTaggedMoment Moment = MakeMoment(FText::Format(BookmarkTitleFormat, ++BookmarkCount), GetCurrentTime(), FDateTime::Now());
	AddMarker(Moment);
	OnBookmarkAdded.Broadcast(Moment);
}

void UCPP_UW_IHPCTimeline::ToggleRecording()
{
	if (!bRecording)
	{
		// ---- start recording: remember where and when it started
		bRecording = true;
		RecordingStartTime = GetCurrentTime();
		RecordingStartClock = FDateTime::Now();
		PulseClock = 0.f;

		if (bPlayWhenRecordingStarts && !UIHPCUILibrary::IsAvatarPlaying(this))
		{
			Play();
		}
		if (RecordButton)
		{
			RecordButton->SetColorAndOpacity(RecordingTint);   // tints the button picture red
			RecordButton->SetToolTipText(LOCTEXT("StopRecording", "Stop recording"));
		}
		if (RecordingIndicator)
		{
			RecordingIndicator->SetVisibility(ESlateVisibility::HitTestInvisible);
		}
		return;
	}

	// ---- stop recording: save it as a new moment
	bRecording = false;
	if (RecordButton)
	{
		RecordButton->SetColorAndOpacity(FLinearColor::White);
		RecordButton->SetToolTipText(LOCTEXT("StartRecording", "Record a clip"));
	}
	if (RecordingIndicator)
	{
		RecordingIndicator->SetVisibility(ESlateVisibility::Hidden);
	}

	const FIHPCTaggedMoment Moment = MakeMoment(FText::Format(RecordingTitleFormat, ++RecordingCount), RecordingStartTime, RecordingStartClock);
	AddMarker(Moment);
	OnRecordingSaved.Broadcast(Moment);   // the HUD adds it to Saved Videos + Tagged Points
}

FIHPCTaggedMoment UCPP_UW_IHPCTimeline::MakeMoment(const FText& Title, float Time, const FDateTime& When) const
{
	// Severity 0: no injury-risk analysis exists for a new clip yet.
	return FIHPCTaggedMoment(Title, UIHPCUILibrary::FormatFigmaDate(When), UIHPCUILibrary::FormatFigmaTime(When), 0.f, Time);
}

// ============================================================================
// Scrubbing (dragging / clicking the track)
// ============================================================================

void UCPP_UW_IHPCTimeline::HandleScrubBegin()
{
	bScrubbing = true;
	// Pause while dragging so the animation does not fight the mouse; resume afterwards.
	bWasPlayingBeforeScrub = UIHPCUILibrary::IsAvatarPlaying(this);
	if (bWasPlayingBeforeScrub)
	{
		UIHPCUILibrary::PauseAvatar(this);
	}
}

void UCPP_UW_IHPCTimeline::HandleScrubChanged(float Value)
{
	if (!bScrubbing)
	{
		return;   // only react to the user, not to our own SetValue calls
	}
	UIHPCUILibrary::SeekAvatarTimeline(this, Value, /*bPauseAfterSeek*/ false);
	ShowTime(Value);
}

void UCPP_UW_IHPCTimeline::HandleScrubEnd()
{
	bScrubbing = false;
	if (bWasPlayingBeforeScrub)
	{
		UIHPCUILibrary::PlayAvatar(this, /*bRestartIfAtEnd*/ false);
	}
}

// ============================================================================
// Markers
// ============================================================================

void UCPP_UW_IHPCTimeline::SetMarkers(const TArray<FIHPCTaggedMoment>& NewMarkers)
{
	Markers = NewMarkers;
	RebuildMarkers();
}

void UCPP_UW_IHPCTimeline::AddMarker(const FIHPCTaggedMoment& Marker)
{
	Markers.Add(Marker);
	RebuildMarkers();
}

void UCPP_UW_IHPCTimeline::RebuildMarkers()
{
	if (!MarkerLayer || !WidgetTree)
	{
		return;
	}
	MarkerLayer->ClearChildren();
	MarkerButtons.Reset();

	// In the designer, show the three markers drawn in Figma (8%, 37%, 86% of the track).
	TArray<FIHPCTaggedMoment> ToShow = Markers;
	if (IsDesignTime() && ToShow.Num() == 0)
	{
		for (const float T : { 0.08f, 0.372f, 0.857f })
		{
			ToShow.Add(FIHPCTaggedMoment(FText::GetEmpty(), FText::GetEmpty(), FText::GetEmpty(), 0.f, T));
		}
	}

	// Marker look: the orange dot picture, a bit brighter on hover, darker when pressed.
	FSlateBrush Dot;
	Dot.DrawAs = ESlateBrushDrawType::Image;
	Dot.SetResourceObject(MarkerTexture);
	Dot.ImageSize = FVector2D(MarkerSize, MarkerSize);
	FSlateBrush DotHovered = Dot;
	DotHovered.TintColor = FSlateColor(FLinearColor(1.2f, 1.2f, 1.2f, 1.f));
	FSlateBrush DotPressed = Dot;
	DotPressed.TintColor = FSlateColor(FLinearColor(0.8f, 0.8f, 0.8f, 1.f));
	FButtonStyle Style;
	Style.SetNormal(Dot).SetHovered(DotHovered).SetPressed(DotPressed).SetDisabled(Dot)
	     .SetNormalPadding(FMargin(0.f)).SetPressedPadding(FMargin(0.f));

	for (const FIHPCTaggedMoment& Marker : ToShow)
	{
		UButton* Button = WidgetTree->ConstructWidget<UButton>(UButton::StaticClass());
		Button->SetStyle(Style);
		if (!Marker.Title.IsEmpty())
		{
			// Hover a marker to see what it is, e.g. "Soleus Muscle (R) - 13:55 p.m."
			Button->SetToolTipText(FText::Format(LOCTEXT("MarkerTip", "{0} - {1}"), Marker.Title, Marker.TimeText));
		}
		Button->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCTimeline::HandleMarkerClicked);

		if (UCanvasPanelSlot* MarkerSlot = MarkerLayer->AddChildToCanvas(Button))
		{
			// Anchor to the vertical middle of the track, centre the dot on its time.
			MarkerSlot->SetAnchors(FAnchors(0.f, 0.5f));
			MarkerSlot->SetAlignment(FVector2D(0.5f, 0.5f));
			MarkerSlot->SetSize(FVector2D(MarkerSize, MarkerSize));
			MarkerSlot->SetPosition(FVector2D(FMath::Clamp(Marker.NormalizedTime, 0.f, 1.f) * TrackLength, 0.f));
		}
		MarkerButtons.Add(Button);
	}
}

void UCPP_UW_IHPCTimeline::HandleMarkerClicked()
{
	// Button clicks carry no data, so find the marker under the mouse.
	for (int32 i = 0; i < MarkerButtons.Num() && i < Markers.Num(); ++i)
	{
		if (MarkerButtons[i] && MarkerButtons[i]->IsHovered())
		{
			UIHPCUILibrary::SeekAvatarTimeline(this, Markers[i].NormalizedTime, /*bPauseAfterSeek*/ true);
			OnMarkerClicked.Broadcast(Markers[i]);
			return;
		}
	}
}

#undef LOCTEXT_NAMESPACE
