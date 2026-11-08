// Walk / jog / run locomotion evaluated at an explicit gait-cycle phase, so a timeline can play,
// pause, reverse and scrub it (every Leader Pose follower - muscles, skeleton, clothes - follows).

#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "AnimationTimelineSource.h"
#include "CPP_LocomotionAnimInstance.generated.h"

class UAnimSequenceBase;

/**
 * Parent class for ABP_Abby_Timeline.
 *
 * Phase 0..1 is one gait cycle: 0 = left-foot contact (sync marker "L"), 1 = the next left-foot contact.
 * Clips that contain several cycles (the jog has three) step through them as the phase wraps.
 *
 * AnimGraph: three Sequence Evaluators (Explicit Time = WalkTime / JogTime / RunTime) into two
 * Two-Way Blends (Alpha = WalkJogAlpha, then JogRunAlpha). Speed comes from the pawn's Speed (0..1).
 * If a UAnimationTimelineComp on the pawn drives it, the timeline owns the phase; otherwise it plays forward.
 */
UCLASS(Blueprintable, BlueprintType)
class METAHUMANTEST_API UCPP_LocomotionAnimInstance : public UAnimInstance, public IAnimationTimelineSource
{
	GENERATED_BODY()

public:
	// ---------------- Setup (set in the Anim BP's class defaults) ----------------

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion")
	TObjectPtr<UAnimSequenceBase> WalkAnim = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion")
	TObjectPtr<UAnimSequenceBase> JogAnim = nullptr;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion")
	TObjectPtr<UAnimSequenceBase> RunAnim = nullptr;

	/** Sync marker that starts each gait cycle in the clips. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion")
	FName CycleStartMarker = TEXT("L");

	/** Speed range (0..1) over which walk blends into jog. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion")
	FVector2D WalkToJogSpeed = FVector2D(0.45, 0.62);

	/** Speed range (0..1) over which jog blends into run. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion")
	FVector2D JogToRunSpeed = FVector2D(0.62, 0.80);

	/** How quickly BlendSpeed follows TargetSpeed (FInterpTo speed). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Locomotion")
	float SpeedSmoothing = 4.f;

	/** Read TargetSpeed from the owning ACPP_AbbyCharacter's Speed every frame. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category="Locomotion")
	bool bReadSpeedFromPawn = true;

	/** Straighten the neck by this much (degrees) at full run / full jog; the head turns back the same amount so the
	 *  gaze stays level. Fed to two Transform (Modify) Bone nodes (neck_01, head) after the gait blend. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Locomotion|Posture")
	float NeckUprightAtRun = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Locomotion|Posture")
	float NeckUprightAtJog = 6.f;

	/** Speed 0..1 to blend toward (walk ... run). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Locomotion")
	float TargetSpeed = 0.f;

	// ---------------- Outputs for the AnimGraph ----------------

	UPROPERTY(Transient, BlueprintReadOnly, Category="Locomotion|Output")
	float BlendSpeed = 0.f;

	/** Gait-cycle phase 0..1 (0 = left-foot contact). */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Locomotion|Output")
	float Phase = 0.f;

	/** Which gait cycle we're in (counts up when the phase wraps forward, down when it wraps backward). */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Locomotion|Output")
	int32 CycleIndex = 0;

	/** Seconds per gait cycle for the current blend at real-time speed. */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Locomotion|Output")
	float CycleLength = 1.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Locomotion|Output")
	float WalkTime = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Locomotion|Output")
	float JogTime = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Locomotion|Output")
	float RunTime = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Locomotion|Output")
	float WalkJogAlpha = 0.f;

	UPROPERTY(Transient, BlueprintReadOnly, Category="Locomotion|Output")
	float JogRunAlpha = 0.f;

	/** Additive bone-space rotation for neck_01 (straightens the neck while jogging / running). */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Locomotion|Output")
	FRotator NeckCorrection = FRotator::ZeroRotator;

	/** Additive bone-space rotation for head (cancels NeckCorrection's tilt so the gaze stays level). */
	UPROPERTY(Transient, BlueprintReadOnly, Category="Locomotion|Output")
	FRotator HeadCorrection = FRotator::ZeroRotator;

	// IAnimationTimelineSource
	virtual float GetTimelineCycleLength() const override { return CycleLength; }
	virtual void SetTimelinePosition(float NormalizedPosition) override;
	virtual float GetTimelinePosition() const override { return Phase; }

protected:
	virtual void NativeInitializeAnimation() override;
	virtual void NativeUpdateAnimation(float DeltaSeconds) override;

private:
	/** Where each gait cycle starts inside one clip. */
	struct FGaitCycles
	{
		float Length = 0.f;
		TArray<float> Starts;

		void Build(const UAnimSequenceBase* Anim, FName Marker);
		float Duration(int32 Cycle) const;
		float TimeAt(int32 Cycle, float InPhase) const;
	};

	void MovePhase(float NewPhase);
	void UpdateOutputs();

	FGaitCycles Walk;
	FGaitCycles Jog;
	FGaitCycles Run;
	uint64 LastExternalFrame = 0;
};
