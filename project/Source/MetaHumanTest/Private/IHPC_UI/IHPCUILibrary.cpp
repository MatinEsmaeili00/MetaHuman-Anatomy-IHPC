// IHPCUILibrary.cpp - see IHPCUILibrary.h for what each function is for.

#include "IHPC_UI/IHPCUILibrary.h"

#include "AnimationTimelineComp.h"   // existing project component that scrubs the avatar animation
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"

UAnimationTimelineComp* UIHPCUILibrary::FindAvatarTimeline(const UObject* WorldContextObject)
{
	// Player 0's pawn is the avatar the user is looking at (same pawn the existing
	// WCPP_AnimationTimeline widget talks to).
	APawn* Pawn = UGameplayStatics::GetPlayerPawn(WorldContextObject, 0);
	return Pawn ? Pawn->FindComponentByClass<UAnimationTimelineComp>() : nullptr;
}

bool UIHPCUILibrary::SeekAvatarTimeline(const UObject* WorldContextObject, float NormalizedTime, bool bPauseAfterSeek)
{
	UAnimationTimelineComp* Timeline = FindAvatarTimeline(WorldContextObject);
	if (!Timeline)
	{
		UE_LOG(LogTemp, Warning, TEXT("[IHPC UI] No AnimationTimelineComp on the player pawn - cannot jump to %.2f"), NormalizedTime);
		return false;
	}

	// Stop playback first so the timeline does not immediately move away from the moment.
	if (bPauseAfterSeek)
	{
		Timeline->Pause();
	}
	Timeline->SetNormalizedTime(FMath::Clamp(NormalizedTime, 0.f, 1.f));
	return true;
}

float UIHPCUILibrary::GetAvatarTimelineTime(const UObject* WorldContextObject)
{
	const UAnimationTimelineComp* Timeline = FindAvatarTimeline(WorldContextObject);
	return Timeline ? Timeline->GetNormalizedTime() : -1.f;
}

bool UIHPCUILibrary::PlayAvatar(const UObject* WorldContextObject, bool bRestartIfAtEnd)
{
	UAnimationTimelineComp* Timeline = FindAvatarTimeline(WorldContextObject);
	if (!Timeline)
	{
		return false;
	}
	// At the end of the clip, pressing play should start again instead of doing nothing.
	if (bRestartIfAtEnd && Timeline->GetNormalizedTime() >= 0.999f)
	{
		Timeline->SetNormalizedTime(0.f);
	}
	Timeline->PlayForward();
	return true;
}

bool UIHPCUILibrary::PauseAvatar(const UObject* WorldContextObject)
{
	UAnimationTimelineComp* Timeline = FindAvatarTimeline(WorldContextObject);
	if (!Timeline)
	{
		return false;
	}
	Timeline->Pause();
	return true;
}

bool UIHPCUILibrary::IsAvatarPlaying(const UObject* WorldContextObject)
{
	const UAnimationTimelineComp* Timeline = FindAvatarTimeline(WorldContextObject);
	return Timeline && Timeline->IsPlaying();
}

bool UIHPCUILibrary::SetAvatarPlaySpeed(const UObject* WorldContextObject, float Speed)
{
	UAnimationTimelineComp* Timeline = FindAvatarTimeline(WorldContextObject);
	if (!Timeline)
	{
		return false;
	}
	// PlaySpeed is used every frame by UAnimationTimelineComp::TickComponent, so changing it
	// takes effect immediately, even while the animation is playing.
	Timeline->PlaySpeed = FMath::Max(Speed, 0.01f);
	return true;
}

float UIHPCUILibrary::GetAvatarPlaySpeed(const UObject* WorldContextObject)
{
	const UAnimationTimelineComp* Timeline = FindAvatarTimeline(WorldContextObject);
	return Timeline ? Timeline->PlaySpeed : -1.f;
}

FText UIHPCUILibrary::FormatFigmaDate(const FDateTime& When)
{
	static const TCHAR* Months[] = { TEXT("Jan"), TEXT("Feb"), TEXT("Mar"), TEXT("Apr"), TEXT("May"), TEXT("Jun"),
	                                 TEXT("Jul"), TEXT("Aug"), TEXT("Sep"), TEXT("Oct"), TEXT("Nov"), TEXT("Dec") };
	const int32 Day = When.GetDay();
	// 1st, 2nd, 3rd, 4th ... 11th, 12th, 13th ... 21st, 22nd, 23rd ...
	const TCHAR* Suffix = TEXT("th");
	if (Day % 100 < 11 || Day % 100 > 13)
	{
		switch (Day % 10)
		{
		case 1: Suffix = TEXT("st"); break;
		case 2: Suffix = TEXT("nd"); break;
		case 3: Suffix = TEXT("rd"); break;
		default: break;
		}
	}
	return FText::FromString(FString::Printf(TEXT("%s %d%s, %d"), Months[When.GetMonth() - 1], Day, Suffix, When.GetYear()));
}

FText UIHPCUILibrary::FormatFigmaTime(const FDateTime& When)
{
	// Figma shows "13:55 p.m." (24-hour clock + a.m./p.m.), so keep exactly that style.
	const int32 Hour = When.GetHour();
	return FText::FromString(FString::Printf(TEXT("%02d:%02d %s"), Hour, When.GetMinute(), Hour < 12 ? TEXT("a.m.") : TEXT("p.m.")));
}

FLinearColor UIHPCUILibrary::FigmaColor(const FString& Hex, float Opacity)
{
	// Figma colours are sRGB. Building an FColor and converting it to FLinearColor
	// does the sRGB -> linear conversion Unreal expects, so the colour on screen matches Figma.
	FLinearColor Color = FLinearColor(FColor::FromHex(Hex));
	Color.A = FMath::Clamp(Opacity, 0.f, 1.f);
	return Color;
}

FLinearColor UIHPCUILibrary::SeverityToColor(float Severity)
{
	// Colours sampled from the bar at the top of the Figma video cards.
	struct FStop { float At; const TCHAR* Hex; };
	static const FStop Stops[] = {
		{ 0.00f, TEXT("70BF44") },   // green
		{ 0.45f, TEXT("F2DE1C") },   // yellow
		{ 0.75f, TEXT("F7A21F") },   // orange
		{ 1.00f, TEXT("ED1C24") },   // red
	};

	const float S = FMath::Clamp(Severity, 0.f, 1.f);
	for (int32 i = 1; i < UE_ARRAY_COUNT(Stops); ++i)
	{
		if (S <= Stops[i].At)
		{
			const float Alpha = (S - Stops[i - 1].At) / (Stops[i].At - Stops[i - 1].At);
			return FLinearColor::LerpUsingHSV(FigmaColor(Stops[i - 1].Hex), FigmaColor(Stops[i].Hex), Alpha);
		}
	}
	return FigmaColor(Stops[UE_ARRAY_COUNT(Stops) - 1].Hex);
}
