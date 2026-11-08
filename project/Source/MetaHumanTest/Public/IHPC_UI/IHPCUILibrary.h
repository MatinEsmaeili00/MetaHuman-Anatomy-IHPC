// IHPCUILibrary.h
// -----------------------------------------------------------------------------
// Small helper functions shared by the IHPC widgets (and usable from Blueprint).
//
//  * Find the avatar's UAnimationTimelineComp (the component that already drives
//    the animation scrubbing in this project - see AnimationTimelineComp.h).
//  * Jump that timeline to a point (used when a video card / tagged point is clicked).
//  * Read the timeline's current position (used to draw the playhead on the graphs).
//  * Turn a Figma hex colour into an Unreal colour.
//  * Turn a severity value (0..1) into the matching colour on the green-to-red bar.
// -----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "IHPCUILibrary.generated.h"

class UAnimationTimelineComp;

UCLASS()
class METAHUMANTEST_API UIHPCUILibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Returns the animation timeline component on the player's current pawn, or nullptr. */
	UFUNCTION(BlueprintPure, Category = "IHPC|Timeline", meta = (WorldContext = "WorldContextObject"))
	static UAnimationTimelineComp* FindAvatarTimeline(const UObject* WorldContextObject);

	/** Jumps the avatar's animation to NormalizedTime (0..1). Returns false if no timeline was found. */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Timeline", meta = (WorldContext = "WorldContextObject"))
	static bool SeekAvatarTimeline(const UObject* WorldContextObject, float NormalizedTime, bool bPauseAfterSeek = true);

	/** Current position of the avatar's animation (0..1), or -1 if no timeline was found. */
	UFUNCTION(BlueprintPure, Category = "IHPC|Timeline", meta = (WorldContext = "WorldContextObject"))
	static float GetAvatarTimelineTime(const UObject* WorldContextObject);

	/** Starts the avatar animation playing forward. If it is at the very end and bRestartIfAtEnd, starts from 0. */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Timeline", meta = (WorldContext = "WorldContextObject"))
	static bool PlayAvatar(const UObject* WorldContextObject, bool bRestartIfAtEnd = true);

	/** Pauses the avatar animation. */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Timeline", meta = (WorldContext = "WorldContextObject"))
	static bool PauseAvatar(const UObject* WorldContextObject);

	/** True while the avatar animation is playing. */
	UFUNCTION(BlueprintPure, Category = "IHPC|Timeline", meta = (WorldContext = "WorldContextObject"))
	static bool IsAvatarPlaying(const UObject* WorldContextObject);

	/** Playback speed multiplier (1 = normal, 0.5 = half speed, 2 = double speed). */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Timeline", meta = (WorldContext = "WorldContextObject"))
	static bool SetAvatarPlaySpeed(const UObject* WorldContextObject, float Speed);

	/** Current playback speed multiplier, or -1 if no timeline was found. */
	UFUNCTION(BlueprintPure, Category = "IHPC|Timeline", meta = (WorldContext = "WorldContextObject"))
	static float GetAvatarPlaySpeed(const UObject* WorldContextObject);

	/** Date in the Figma card style, e.g. "Jan 4th, 2026". */
	UFUNCTION(BlueprintPure, Category = "IHPC|Format")
	static FText FormatFigmaDate(const FDateTime& When);

	/** Time in the Figma card style, e.g. "13:55 p.m.". */
	UFUNCTION(BlueprintPure, Category = "IHPC|Format")
	static FText FormatFigmaTime(const FDateTime& When);

	/** Converts a Figma hex colour (e.g. "B3CA7F") plus opacity into an Unreal linear colour. */
	UFUNCTION(BlueprintPure, Category = "IHPC|Style")
	static FLinearColor FigmaColor(const FString& Hex, float Opacity = 1.f);

	/** Colour on the card's green -> yellow -> orange -> red bar for a severity of 0..1. */
	UFUNCTION(BlueprintPure, Category = "IHPC|Style")
	static FLinearColor SeverityToColor(float Severity);
};
