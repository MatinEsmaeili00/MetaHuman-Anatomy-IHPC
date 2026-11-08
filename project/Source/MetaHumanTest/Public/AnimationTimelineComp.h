// Animation timeline: play / pause / reverse / scrub the pawn's animation. Lives on the pawn;
// the timeline widget (UWCPP_AnimationTimeline) only forwards button clicks and slider drags to it.

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

	UPROPERTY(Transient)
	TObjectPtr<USkeletalMeshComponent> DrivenMesh = nullptr;

	TWeakObjectPtr<UAnimInstance> SourceInstance;
	TWeakObjectPtr<UAnimSingleNodeInstance> SingleNodeInstance;

	EActiveMode ActiveMode = EActiveMode::None;
	ETimelinePlayState PlayState = ETimelinePlayState::Paused;
	ETimelinePlayState LastPlayDirection = ETimelinePlayState::Forward;
	float NormalizedTime = 0.f;
	float ClipLength = 0.f;
	bool bScrubbing = false;
};
