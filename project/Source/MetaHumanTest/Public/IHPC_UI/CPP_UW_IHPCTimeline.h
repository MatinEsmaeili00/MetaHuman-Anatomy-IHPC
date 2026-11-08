// CPP_UW_IHPCTimeline.h
// -----------------------------------------------------------------------------
// The playback / timeline bar at the bottom of the screen.
// Figma: layer "ihpc ui (1)ee 1" on frame "iPad Pro 12.9" - 1" (1048 x 102 px), plus the
// "x1.5" text layer on top of the slider knob.
//
//  (O)  (|<)  (||)  (slow)  ====o======(x1.5)------o-----  (fast)  (>)  (>|)  (bookmark)
//  rec  restart pause  -speed     timeline + markers           +speed  play  ff
//
//   Record       : first click starts recording (and playback), second click stops and saves
//                  a new clip -> a new card in Saved Videos, a new Tagged Point, a new marker.
//   Restart      : jumps to the start (keeps playing if it was playing).
//   Pause / Play : pause / play the avatar animation (play at the end starts again).
//   Slow / Fast  : steps the playback speed down / up through SpeedSteps (0.25x ... 3x).
//                  The knob shows the current speed ("x1.5" in Figma).
//   Timeline     : dark part = played, light part = still to play. Drag (or click) to scrub.
//   Markers      : orange dots = saved videos, recordings and bookmarks. Click one to jump to it.
//   Fast-forward : jumps to the next marker after the current time (or to the end if none).
//   Bookmark     : drops a marker at the current time and adds it to Tagged Points.
//
// Everything controls the project's existing UAnimationTimelineComp on the player pawn
// (through UIHPCUILibrary), the same component the old prototype timeline used.
//
// Widget Blueprint: /Game/UserInterface/IHPC/Widgets/WBP_IHPC_Timeline
// Lives inside WBP_IHPC_HUD (bottom centre). It replaces the old prototype WB_AmationTimeline.
// -----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IHPC_UI/IHPCTypes.h"
#include "CPP_UW_IHPCTimeline.generated.h"

class UButton;
class UCanvasPanel;
class UProgressBar;
class USlider;
class UTextBlock;
class UTexture2D;

/** Fired when the playback speed changes. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIHPCSpeedChanged, float, NewSpeed);

UCLASS(Abstract)
class METAHUMANTEST_API UCPP_UW_IHPCTimeline : public UUserWidget
{
	GENERATED_BODY()

public:
	UCPP_UW_IHPCTimeline(const FObjectInitializer& ObjectInitializer);

	// ---------------------------------------------------------------- settings

	/** The speeds the slow-down / speed-up buttons step through. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Timeline")
	TArray<float> SpeedSteps = { 0.25f, 0.5f, 1.f, 1.5f, 2.f, 3.f };

	/** Index in SpeedSteps used at the start (2 = 1x). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Timeline")
	int32 DefaultSpeedIndex = 2;

	/** Start playing the animation when recording starts (so there is something to record). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Behaviour")
	bool bPlayWhenRecordingStarts = true;

	/** Length of the track in px (Figma: 513, from x 274 to x 787). The knob and markers travel this far. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Layout")
	float TrackLength = 513.f;

	/** Size of a marker dot (Figma: white ring 19 px). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Layout")
	float MarkerSize = 19.f;

	/** Where the knob sits in the UMG designer preview (Figma: 54.5%). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Layout")
	float DesignerPreviewTime = 0.545f;

	/** Picture used for each marker (T_IHPC_TL_Marker). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	TObjectPtr<UTexture2D> MarkerTexture;

	/** Colour the record button turns while recording. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	FLinearColor RecordingTint;

	/** Title of a new recording; {0} = number. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Text")
	FText RecordingTitleFormat;

	/** Title of a new bookmark; {0} = number. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Text")
	FText BookmarkTitleFormat;

	// ---------------------------------------------------------------- events

	/** A recording was stopped. The HUD adds it to Saved Videos + Tagged Points. */
	UPROPERTY(BlueprintAssignable, Category = "IHPC|Events")
	FIHPCMomentEvent OnRecordingSaved;

	/** The bookmark button was pressed. The HUD adds it to Tagged Points. */
	UPROPERTY(BlueprintAssignable, Category = "IHPC|Events")
	FIHPCMomentEvent OnBookmarkAdded;

	/** A marker on the track was clicked. */
	UPROPERTY(BlueprintAssignable, Category = "IHPC|Events")
	FIHPCMomentEvent OnMarkerClicked;

	UPROPERTY(BlueprintAssignable, Category = "IHPC|Events")
	FIHPCSpeedChanged OnSpeedChanged;

	// ---------------------------------------------------------------- functions (one per button)

	UFUNCTION(BlueprintCallable, Category = "IHPC|Timeline") void Play();
	UFUNCTION(BlueprintCallable, Category = "IHPC|Timeline") void Pause();
	UFUNCTION(BlueprintCallable, Category = "IHPC|Timeline") void Restart();
	UFUNCTION(BlueprintCallable, Category = "IHPC|Timeline") void SlowDown();
	UFUNCTION(BlueprintCallable, Category = "IHPC|Timeline") void SpeedUp();
	UFUNCTION(BlueprintCallable, Category = "IHPC|Timeline") void FastForwardToNextMarker();
	UFUNCTION(BlueprintCallable, Category = "IHPC|Timeline") void AddBookmark();
	UFUNCTION(BlueprintCallable, Category = "IHPC|Timeline") void ToggleRecording();

	UFUNCTION(BlueprintPure, Category = "IHPC|Timeline")
	bool IsRecording() const { return bRecording; }

	/** Picks a speed from SpeedSteps by index (clamped). */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Timeline")
	void SetSpeedIndex(int32 Index);

	UFUNCTION(BlueprintPure, Category = "IHPC|Timeline")
	float GetSpeed() const;

	/** Replace all markers (the HUD passes the saved videos at start). */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Timeline")
	void SetMarkers(const TArray<FIHPCTaggedMoment>& NewMarkers);

	UFUNCTION(BlueprintCallable, Category = "IHPC|Timeline")
	void AddMarker(const FIHPCTaggedMoment& Marker);

	UFUNCTION(BlueprintPure, Category = "IHPC|Timeline")
	TArray<FIHPCTaggedMoment> GetMarkers() const { return Markers; }

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// ---- Widgets that must exist in the Widget Blueprint (names must match exactly) ----

	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> RecordButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> RestartButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> PauseButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SlowDownButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> SpeedUpButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> PlayButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> FastForwardButton;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UButton> BookmarkButton;

	/** The two-tone track: fill = played (dark), background = still to play (light). */
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UProgressBar> TrackProgress;

	/** Invisible slider on top of the track, used for dragging / clicking to scrub. */
	UPROPERTY(meta = (BindWidget)) TObjectPtr<USlider> ScrubSlider;

	/** Knob picture + speed text. Placed at the START of the track; moved sideways in code. */
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UWidget> ThumbGroup;

	/** "x1.5" text on the knob. */
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SpeedText;

	/** Canvas over the track that holds one button per marker (created in code). */
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UCanvasPanel> MarkerLayer;

	/** Optional red dot shown (pulsing) while recording. */
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> RecordingIndicator;

private:
	UFUNCTION() void HandleRecord();
	UFUNCTION() void HandleRestart();
	UFUNCTION() void HandlePause();
	UFUNCTION() void HandleSlowDown();
	UFUNCTION() void HandleSpeedUp();
	UFUNCTION() void HandlePlay();
	UFUNCTION() void HandleFastForward();
	UFUNCTION() void HandleBookmark();
	UFUNCTION() void HandleScrubChanged(float Value);
	UFUNCTION() void HandleScrubBegin();
	UFUNCTION() void HandleScrubEnd();
	UFUNCTION() void HandleMarkerClicked();

	/** Moves the knob and the dark/light split to Time (0..1). */
	void ShowTime(float Time);

	/** Current animation time (0..1), or 0 if there is no timeline. */
	float GetCurrentTime() const;

	/** Re-creates the marker buttons from Markers. */
	void RebuildMarkers();

	/** Writes "x1.5" etc. on the knob. */
	void ShowSpeed(float Speed);

	/** Builds a tagged moment for a new recording / bookmark. */
	FIHPCTaggedMoment MakeMoment(const FText& Title, float Time, const FDateTime& When) const;

	UPROPERTY()
	TArray<FIHPCTaggedMoment> Markers;

	UPROPERTY()
	TArray<TObjectPtr<UButton>> MarkerButtons;

	int32 SpeedIndex = 2;
	float ShownSpeed = -1.f;

	bool bRecording = false;
	float RecordingStartTime = 0.f;
	FDateTime RecordingStartClock;
	int32 RecordingCount = 0;
	int32 BookmarkCount = 0;

	bool bScrubbing = false;
	bool bWasPlayingBeforeScrub = false;

	float PulseClock = 0.f;
};
