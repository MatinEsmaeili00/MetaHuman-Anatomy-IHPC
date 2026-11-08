// Interface between UAnimationTimelineComp (on the pawn) and an anim instance whose time it can drive.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AnimationTimelineSource.generated.h"

UINTERFACE(MinimalAPI, meta=(CannotImplementInterfaceInBlueprint))
class UAnimationTimelineSource : public UInterface
{
	GENERATED_BODY()
};

/**
 * An anim instance whose playback position can be set from outside, so a timeline can
 * play, pause, reverse and scrub it. Position is normalized 0..1 over one "cycle"
 * (for UCPP_LocomotionAnimInstance: one gait cycle, 0 = left-foot contact).
 */
class METAHUMANTEST_API IAnimationTimelineSource
{
	GENERATED_BODY()

public:
	/** Seconds one 0 -> 1 cycle takes at play rate 1, for the pose being shown right now. */
	virtual float GetTimelineCycleLength() const = 0;

	/** Show the pose at this normalized position. Called every frame while a timeline drives the instance. */
	virtual void SetTimelinePosition(float NormalizedPosition) = 0;

	/** Current normalized position (so a timeline can pick up where the anim instance is). */
	virtual float GetTimelinePosition() const = 0;
};
