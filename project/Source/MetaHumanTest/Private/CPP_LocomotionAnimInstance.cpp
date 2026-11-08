#include "CPP_LocomotionAnimInstance.h"

#include "Animation/AnimSequence.h"
#include "MetaHumanTest/CPP_AbbyCharacter.h"

void UCPP_LocomotionAnimInstance::FGaitCycles::Build(const UAnimSequenceBase* Anim, FName Marker)
{
	Length = Anim ? Anim->GetPlayLength() : 0.f;
	Starts.Reset();

	if (const UAnimSequence* Sequence = Cast<UAnimSequence>(Anim))
	{
		for (const FAnimSyncMarker& SyncMarker : Sequence->AuthoredSyncMarkers)
		{
			if (SyncMarker.MarkerName == Marker)
			{
				Starts.Add(SyncMarker.Time);
			}
		}
	}
	Starts.Sort();
	if (Starts.Num() == 0)
	{
		Starts.Add(0.f); // no markers: the whole clip is one cycle
	}
}

float UCPP_LocomotionAnimInstance::FGaitCycles::Duration(int32 Cycle) const
{
	if (Length <= 0.f)
	{
		return 0.f;
	}
	const int32 Num = Starts.Num();
	const int32 Index = ((Cycle % Num) + Num) % Num;
	const float Start = Starts[Index];
	const float End = (Index + 1 < Num) ? Starts[Index + 1] : Starts[0] + Length;
	return FMath::Max(End - Start, KINDA_SMALL_NUMBER);
}

float UCPP_LocomotionAnimInstance::FGaitCycles::TimeAt(int32 Cycle, float InPhase) const
{
	if (Length <= 0.f)
	{
		return 0.f;
	}
	const int32 Num = Starts.Num();
	const int32 Index = ((Cycle % Num) + Num) % Num;
	float Time = FMath::Fmod(Starts[Index] + InPhase * Duration(Cycle), Length);
	if (Time < 0.f)
	{
		Time += Length;
	}
	return Time;
}

void UCPP_LocomotionAnimInstance::NativeInitializeAnimation()
{
	Super::NativeInitializeAnimation();

	Walk.Build(WalkAnim, CycleStartMarker);
	Jog.Build(JogAnim, CycleStartMarker);
	Run.Build(RunAnim, CycleStartMarker);

	if (const ACPP_AbbyCharacter* Abby = Cast<ACPP_AbbyCharacter>(TryGetPawnOwner()))
	{
		TargetSpeed = BlendSpeed = Abby->Speed; // start at the right gait, no blend-in from walk
	}
	UpdateOutputs();
}

void UCPP_LocomotionAnimInstance::NativeUpdateAnimation(float DeltaSeconds)
{
	Super::NativeUpdateAnimation(DeltaSeconds);

	if (bReadSpeedFromPawn)
	{
		if (const ACPP_AbbyCharacter* Abby = Cast<ACPP_AbbyCharacter>(TryGetPawnOwner()))
		{
			TargetSpeed = Abby->Speed;
		}
	}
	BlendSpeed = FMath::FInterpTo(BlendSpeed, FMath::Clamp(TargetSpeed, 0.f, 1.f), DeltaSeconds, SpeedSmoothing);

	// Nobody set the phase this frame (no timeline on the pawn, or the Anim BP editor preview): play forward.
	if (GFrameCounter > LastExternalFrame + 1 && CycleLength > KINDA_SMALL_NUMBER)
	{
		MovePhase(FMath::Frac(Phase + DeltaSeconds / CycleLength));
	}

	UpdateOutputs();
}

void UCPP_LocomotionAnimInstance::SetTimelinePosition(float NormalizedPosition)
{
	LastExternalFrame = GFrameCounter;
	MovePhase(FMath::Clamp(NormalizedPosition, 0.f, 1.f));
	UpdateOutputs();
}

void UCPP_LocomotionAnimInstance::MovePhase(float NewPhase)
{
	// A jump of more than half a cycle is a wrap: step to the next (or previous) gait cycle,
	// so clips with several cycles play all of them, forward and backward.
	const float Delta = NewPhase - Phase;
	if (Delta < -0.5f)
	{
		++CycleIndex;
	}
	else if (Delta > 0.5f)
	{
		--CycleIndex;
	}
	Phase = NewPhase;
}

void UCPP_LocomotionAnimInstance::UpdateOutputs()
{
	WalkJogAlpha = FMath::GetMappedRangeValueClamped(FVector2f(WalkToJogSpeed), FVector2f(0.f, 1.f), BlendSpeed);
	JogRunAlpha = FMath::GetMappedRangeValueClamped(FVector2f(JogToRunSpeed), FVector2f(0.f, 1.f), BlendSpeed);

	const float WalkWeight = (1.f - WalkJogAlpha) * (1.f - JogRunAlpha);
	const float JogWeight = WalkJogAlpha * (1.f - JogRunAlpha);
	const float RunWeight = JogRunAlpha;
	CycleLength = WalkWeight * Walk.Duration(CycleIndex) + JogWeight * Jog.Duration(CycleIndex) + RunWeight * Run.Duration(CycleIndex);

	// MetaHuman neck / spine bones bend forward around local Z (yaw +); negative yaw straightens the neck.
	const float NeckDegrees = JogWeight * NeckUprightAtJog + RunWeight * NeckUprightAtRun;
	NeckCorrection = FRotator(0.f, -NeckDegrees, 0.f);
	HeadCorrection = FRotator(0.f, NeckDegrees, 0.f);

	WalkTime = Walk.TimeAt(CycleIndex, Phase);
	JogTime = Jog.TimeAt(CycleIndex, Phase);
	RunTime = Run.TimeAt(CycleIndex, Phase);
}
