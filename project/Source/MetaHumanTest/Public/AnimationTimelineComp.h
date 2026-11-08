// Animation timeline: play / pause / reverse / scrub the pawn's animation. Lives on the pawn;
// the timeline widgets (IHPC timeline bar, old UWCPP_AnimationTimeline) only forward button clicks and slider drags to it.
// It also plays named ACTION clips (PlayAction / StopAction) instead of the walk / jog / run locomotion - the IHPC HUD's
// "Actions" menu buttons (Action_Running, Action_TennisForehand, Action_SoccerWarmup) call them.

#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AnimationTimelineComp.generated.h"

class UAnimInstance;
class UAnimSequenceBase;
class UAnimSingleNodeInstance;
class USkeletalMeshComponent;

UENUM(BlueprintType)
enum class ETimelineDriveMode : uint8
{
	/** Drive the mesh's Anim BP if it implements IAnimationTimelineSource; otherwise play AnimationSequence as a single clip. */
	Auto,
	/** Drive the mesh's Anim BP (it must implement IAnimationTimelineSource, e.g. UCPP_LocomotionAnimInstance). */
	AnimBlueprint,
	/** Play AnimationSequence on the mesh in single-node mode (this replaces the Anim BP). */
	SingleClip,
};

UENUM(BlueprintType)
enum class ETimelinePlayState : uint8
{
	Paused,
	Forward,
	Reverse,
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimelinePlayStateChanged, ETimelinePlayState, NewState);

/** Fired when an action clip starts (ActionName) or the pawn goes back to its walk / jog / run locomotion (None). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTimelineActionChanged, FName, ActionName);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class METAHUMANTEST_API UAnimationTimelineComp : public UActorComponent
{
	GENERATED_BODY()

public:
	UAnimationTimelineComp();

	// ---------------- Setup ----------------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation Timeline")
	ETimelineDriveMode DriveMode = ETimelineDriveMode::Auto;

	/** Mesh to drive. If empty: the component named TargetMeshName, else the first skeletal mesh that runs its own animation. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation Timeline")
	TObjectPtr<USkeletalMeshComponent> SkeletalMesh = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation Timeline")
	FName TargetMeshName = TEXT("Body");

	/** Clip for SingleClip mode (also used by Auto when the Anim BP can't be driven). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation Timeline")
	TObjectPtr<UAnimSequenceBase> AnimationSequence = nullptr;

	/** Wrap around at the ends; if false, playback stops at 0 / 1. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation Timeline")
	bool bLoop = true;

	/** Playback speed multiplier (1 = real time). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation Timeline", meta=(ClampMin="0"))
	float PlaySpeed = 1.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation Timeline")
	bool bAutoPlay = true;

	UPROPERTY(BlueprintAssignable, Category="Animation Timeline")
	FOnTimelinePlayStateChanged OnPlayStateChanged;

	// ---------------- Actions (the IHPC "Actions" menu: Running, Tennis Forehand, Soccer Warmup ...) ----------------

	/**
	 * Clips the pawn can play INSTEAD of its walk / jog / run locomotion, by action name.
	 * The IHPC HUD's buttons named "Action_<Name>" call PlayAction(<Name>) on the possessed pawn, so the keys here
	 * must match the button names without the "Action_" prefix: Running, TennisForehand, SoccerWarmup.
	 * Per pawn, so every character uses clips made for its own skeleton:
	 *   Abby8 (Abby's skeleton)            -> /Game/MadisonAnimations/AS_*
	 *   Beau / Madi (metahuman_base_skel)  -> /Game/MadisonAnimations/MetaHuman/AS_*_MH (retargeted with RTG_MH_IKRig)
	 * While an action plays the Body mesh runs the clip in single-node mode; the play / pause / scrub / speed controls
	 * drive the clip, and every Leader Pose follower (face, clothes, muscles, skeleton) follows it.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation Timeline|Actions")
	TMap<FName, TObjectPtr<UAnimSequenceBase>> ActionClips;

	/** Loop action clips (true) or stop on their last frame (false). The locomotion always loops (bLoop). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Animation Timeline|Actions")
	bool bLoopActions = true;

	UPROPERTY(BlueprintAssignable, Category="Animation Timeline|Actions")
	FOnTimelineActionChanged OnActionChanged;

	/** Play ActionClips[ActionName] from the start (replaces the walk / jog / run Anim BP until StopAction).
	 *  Returns false if the pawn has no clip for that action or the clip doesn't fit the mesh's skeleton. */
	UFUNCTION(BlueprintCallable, Category="Animation Timeline|Actions")
	bool PlayAction(FName ActionName);

	/** Back to the walk / jog / run locomotion (the mesh's Anim BP). */
	UFUNCTION(BlueprintCallable, Category="Animation Timeline|Actions")
	void StopAction();

	/** The action being played, or None while the pawn walks / jogs / runs. */
	UFUNCTION(BlueprintPure, Category="Animation Timeline|Actions")
	FName GetCurrentAction() const { return CurrentAction; }

	UFUNCTION(BlueprintPure, Category="Animation Timeline|Actions")
	bool IsPlayingAction() const { return !CurrentAction.IsNone(); }

	UFUNCTION(BlueprintPure, Category="Animation Timeline|Actions")
	bool HasAction(FName ActionName) const { return ActionClips.FindRef(ActionName) != nullptr; }

	// ---------------- Controls for UI ----------------

	UFUNCTION(BlueprintCallable, Category="Animation Timeline")
	void PlayForward();

	UFUNCTION(BlueprintCallable, Category="Animation Timeline")
	void PlayReverse();

	UFUNCTION(BlueprintCallable, Category="Animation Timeline")
	void Pause();

	/** Pause if playing, otherwise resume in the last direction. */
	UFUNCTION(BlueprintCallable, Category="Animation Timeline")
	void TogglePlayPause();

	/** Jump to a position (0..1). */
	UFUNCTION(BlueprintCallable, Category="Animation Timeline")
	void SetNormalizedTime(float InAlpha);

	UFUNCTION(BlueprintPure, Category="Animation Timeline")
	float GetNormalizedTime() const { return NormalizedTime; }

	/** Slider grabbed: hold the position while dragging (playback resumes on EndScrub if it was playing). */
	UFUNCTION(BlueprintCallable, Category="Animation Timeline")
	void BeginScrub();

	UFUNCTION(BlueprintCallable, Category="Animation Timeline")
	void EndScrub();

	/** Step by a number of seconds (negative = back) and pause, e.g. for frame-by-frame keys. */
	UFUNCTION(BlueprintCallable, Category="Animation Timeline")
	void StepSeconds(float Seconds);

	UFUNCTION(BlueprintCallable, Category="Animation Timeline")
	void SetPlaySpeed(float NewPlaySpeed) { PlaySpeed = FMath::Max(0.f, NewPlaySpeed); }

	UFUNCTION(BlueprintPure, Category="Animation Timeline")
	ETimelinePlayState GetPlayState() const { return PlayState; }

	UFUNCTION(BlueprintPure, Category="Animation Timeline")
	bool IsPlaying() const { return PlayState != ETimelinePlayState::Paused; }

	UFUNCTION(BlueprintPure, Category="Animation Timeline")
	bool IsScrubbing() const { return bScrubbing; }

	/** Seconds one 0 -> 1 cycle takes at PlaySpeed 1 (gait cycle for the locomotion Anim BP, clip length for a single clip). */
	UFUNCTION(BlueprintPure, Category="Animation Timeline")
	float GetCycleLength() const;

	/** False if there's nothing on the pawn this timeline can drive. */
	UFUNCTION(BlueprintPure, Category="Animation Timeline")
	bool CanScrub() const;

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	enum class EActiveMode : uint8 { None, AnimBlueprint, SingleClip, RateOnly };

	void ResolveTarget();
	void SetPlayState(ETimelinePlayState NewState);
	void PushTime();
	float WrapOrClamp(float Time) const;
	bool ShouldLoop() const { return CurrentAction.IsNone() ? bLoop : bLoopActions; }

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> DrivenMesh = nullptr;

	/** Clip of the action being played (null = locomotion). */
	UPROPERTY(Transient)
	TObjectPtr<UAnimSequenceBase> ActionClip = nullptr;

	FName CurrentAction;

	TWeakObjectPtr<UAnimInstance> SourceInstance;
	TWeakObjectPtr<UAnimSingleNodeInstance> SingleNodeInstance;

	EActiveMode ActiveMode = EActiveMode::None;
	ETimelinePlayState PlayState = ETimelinePlayState::Paused;
	ETimelinePlayState LastPlayDirection = ETimelinePlayState::Forward;
	float NormalizedTime = 0.f;
	float ClipLength = 0.f;
	bool bScrubbing = false;
};
