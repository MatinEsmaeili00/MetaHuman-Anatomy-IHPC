#include "AnimationTimelineComp.h"

#include "AnimationTimelineSource.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimSequenceBase.h"
#include "Animation/AnimSingleNodeInstance.h"
#include "Components/SkeletalMeshComponent.h"
#include "GameFramework/Actor.h"

DEFINE_LOG_CATEGORY_STATIC(LogAnimTimeline, Log, All);

UAnimationTimelineComp::UAnimationTimelineComp()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickGroup = TG_PrePhysics;
}

void UAnimationTimelineComp::BeginPlay()
{
	Super::BeginPlay();

	ResolveTarget();
	SetPlayState(bAutoPlay ? ETimelinePlayState::Forward : ETimelinePlayState::Paused);
}

void UAnimationTimelineComp::ResolveTarget()
{
	ActiveMode = EActiveMode::None;
	SourceInstance.Reset();
	SingleNodeInstance.Reset();

	AActor* Owner = GetOwner();
	DrivenMesh = SkeletalMesh;
	if (!DrivenMesh && Owner)
	{
		TArray<USkeletalMeshComponent*> Meshes;
		Owner->GetComponents<USkeletalMeshComponent>(Meshes);
		for (USkeletalMeshComponent* Mesh : Meshes)
		{
			if (Mesh && Mesh->GetFName() == TargetMeshName)
			{
				DrivenMesh = Mesh;
				break;
			}
		}
		if (!DrivenMesh)
		{
			// First mesh that animates by itself (not a Leader Pose follower).
			for (USkeletalMeshComponent* Mesh : Meshes)
			{
				if (Mesh && Mesh->GetAnimInstance() && !Mesh->LeaderPoseComponent.IsValid())
				{
					DrivenMesh = Mesh;
					break;
				}
			}
		}
	}

	if (!DrivenMesh)
	{
		UE_LOG(LogAnimTimeline, Warning, TEXT("%s: no skeletal mesh to drive"), *GetNameSafe(Owner));
		return;
	}

	// Set the time before the mesh evaluates its pose this frame.
	DrivenMesh->AddTickPrerequisiteComponent(this);

	UAnimInstance* AnimInstance = DrivenMesh->GetAnimInstance();
	const bool bCanDriveAnimBP = AnimInstance && AnimInstance->Implements<UAnimationTimelineSource>();

	switch (DriveMode)
	{
	case ETimelineDriveMode::Auto:
		ActiveMode = bCanDriveAnimBP ? EActiveMode::AnimBlueprint : (AnimationSequence ? EActiveMode::SingleClip : EActiveMode::RateOnly);
		break;
	case ETimelineDriveMode::AnimBlueprint:
		ActiveMode = bCanDriveAnimBP ? EActiveMode::AnimBlueprint : EActiveMode::RateOnly;
		break;
	case ETimelineDriveMode::SingleClip:
		ActiveMode = AnimationSequence ? EActiveMode::SingleClip : (bCanDriveAnimBP ? EActiveMode::AnimBlueprint : EActiveMode::RateOnly);
		break;
	}

	if (ActiveMode == EActiveMode::AnimBlueprint)
	{
		SourceInstance = AnimInstance;
		NormalizedTime = Cast<IAnimationTimelineSource>(AnimInstance)->GetTimelinePosition();
	}
	else if (ActiveMode == EActiveMode::SingleClip)
	{
		DrivenMesh->SetAnimationMode(EAnimationMode::AnimationSingleNode);
		DrivenMesh->SetAnimation(AnimationSequence);
		SingleNodeInstance = DrivenMesh->GetSingleNodeInstance();
		if (SingleNodeInstance.IsValid())
		{
			SingleNodeInstance->SetPlaying(false); // the timeline owns the clock
		}
		ClipLength = AnimationSequence->GetPlayLength();
	}

	static const TCHAR* ModeNames[] = { TEXT("None"), TEXT("AnimBlueprint"), TEXT("SingleClip"), TEXT("RateOnly (pause/play only)") };
	UE_LOG(LogAnimTimeline, Log, TEXT("%s: timeline drives %s in %s mode"), *GetNameSafe(Owner), *DrivenMesh->GetName(), ModeNames[static_cast<int32>(ActiveMode)]);
}

float UAnimationTimelineComp::GetCycleLength() const
{
	switch (ActiveMode)
	{
	case EActiveMode::AnimBlueprint:
		if (const IAnimationTimelineSource* Source = Cast<IAnimationTimelineSource>(SourceInstance.Get()))
		{
			return Source->GetTimelineCycleLength();
		}
		return 0.f;
	case EActiveMode::SingleClip:
		return ClipLength;
	default:
		return 0.f;
	}
}

bool UAnimationTimelineComp::CanScrub() const
{
	return ActiveMode == EActiveMode::AnimBlueprint || ActiveMode == EActiveMode::SingleClip;
}

float UAnimationTimelineComp::WrapOrClamp(float Time) const
{
	return bLoop ? FMath::Frac(Time) : FMath::Clamp(Time, 0.f, 1.f);
}

void UAnimationTimelineComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	// The anim instance is re-created if the mesh's anim class changes.
	if ((ActiveMode == EActiveMode::AnimBlueprint && !SourceInstance.IsValid()) ||
		(ActiveMode == EActiveMode::SingleClip && !SingleNodeInstance.IsValid()))
	{
		ResolveTarget();
	}

	const float Cycle = GetCycleLength();
	if (PlayState != ETimelinePlayState::Paused && !bScrubbing && Cycle > KINDA_SMALL_NUMBER)
	{
		const float Direction = PlayState == ETimelinePlayState::Reverse ? -1.f : 1.f;
		const float Next = NormalizedTime + Direction * PlaySpeed * DeltaTime / Cycle;

		if (!bLoop && (Next >= 1.f || Next <= 0.f))
		{
			NormalizedTime = FMath::Clamp(Next, 0.f, 1.f);
			SetPlayState(ETimelinePlayState::Paused);
		}
		else
		{
			NormalizedTime = WrapOrClamp(Next);
		}
	}

	PushTime();
}

void UAnimationTimelineComp::PushTime()
{
	switch (ActiveMode)
	{
	case EActiveMode::AnimBlueprint:
		if (IAnimationTimelineSource* Source = Cast<IAnimationTimelineSource>(SourceInstance.Get()))
		{
			Source->SetTimelinePosition(NormalizedTime);
		}
		break;
	case EActiveMode::SingleClip:
		if (UAnimSingleNodeInstance* SingleNode = SingleNodeInstance.Get())
		{
			SingleNode->SetPosition(NormalizedTime * ClipLength, /*bFireNotifies*/ false);
		}
		break;
	default:
		break;
	}
}

void UAnimationTimelineComp::SetPlayState(ETimelinePlayState NewState)
{
	if (NewState != ETimelinePlayState::Paused)
	{
		LastPlayDirection = NewState;
	}

	if (ActiveMode == EActiveMode::RateOnly && DrivenMesh)
	{
		// Anim BP we can't drive: all we can do is freeze or run its clock (no reverse / scrub).
		DrivenMesh->GlobalAnimRateScale = NewState == ETimelinePlayState::Forward ? PlaySpeed : 0.f;
	}

	if (PlayState != NewState)
	{
		PlayState = NewState;
		OnPlayStateChanged.Broadcast(PlayState);
	}
}

void UAnimationTimelineComp::PlayForward()
{
	if (!bLoop && NormalizedTime >= 1.f)
	{
		NormalizedTime = 0.f;
	}
	SetPlayState(ETimelinePlayState::Forward);
}

void UAnimationTimelineComp::PlayReverse()
{
	if (!bLoop && NormalizedTime <= 0.f)
	{
		NormalizedTime = 1.f;
	}
	SetPlayState(ActiveMode == EActiveMode::RateOnly ? ETimelinePlayState::Paused : ETimelinePlayState::Reverse);
}

void UAnimationTimelineComp::Pause()
{
	SetPlayState(ETimelinePlayState::Paused);
}

void UAnimationTimelineComp::TogglePlayPause()
{
	if (IsPlaying())
	{
		Pause();
	}
	else if (LastPlayDirection == ETimelinePlayState::Reverse)
	{
		PlayReverse();
	}
	else
	{
		PlayForward();
	}
}

void UAnimationTimelineComp::SetNormalizedTime(float InAlpha)
{
	NormalizedTime = FMath::Clamp(InAlpha, 0.f, 1.f);
	PushTime(); // show it right away, even while paused
}

void UAnimationTimelineComp::BeginScrub()
{
	bScrubbing = true;
}

void UAnimationTimelineComp::EndScrub()
{
	bScrubbing = false;
}

void UAnimationTimelineComp::StepSeconds(float Seconds)
{
	const float Cycle = GetCycleLength();
	if (Cycle > KINDA_SMALL_NUMBER)
	{
		Pause();
		NormalizedTime = WrapOrClamp(NormalizedTime + Seconds / Cycle);
		PushTime();
	}
}
