// IHPCTypes.h
// -----------------------------------------------------------------------------
// Shared data types for the IHPC (Avatar Injury Prevention) UI that was built
// from the Figma file "Avatar Injury Prevention Program".
//
// Nothing in this file draws anything. It only describes the DATA the widgets
// pass around, so every widget agrees on the same shapes:
//   * EIHPCNavTab          - the five tabs in the drop-down nav bar
//   * EIHPCAnatomyLayer    - the three buttons of the layer switcher (Body / Muscles / Skeleton)
//   * EIHPCAccordionSection- the three sections of the graph accordion
//   * FIHPCTaggedMoment    - one saved video / tagged point (a video card)
//   * FIHPCGraphSeries     - one coloured line in a line graph
// -----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "IHPCTypes.generated.h"

/** The tabs in the nav bar (Figma components "Profile", "Progress", "Saved Videos",
 *  "History & Timeline" and "Settings"). None = no tab selected. */
UENUM(BlueprintType)
enum class EIHPCNavTab : uint8
{
	None            UMETA(DisplayName = "None"),
	Profile         UMETA(DisplayName = "Profile"),
	Progress        UMETA(DisplayName = "Progress"),
	SavedVideos     UMETA(DisplayName = "Saved Videos"),
	HistoryTimeline UMETA(DisplayName = "History & Timeline"),
	Settings        UMETA(DisplayName = "Settings")
};

/** The three round buttons of the layer switcher (Figma "ihpc ui rttd 2"): which part of the avatar is shown. */
UENUM(BlueprintType)
enum class EIHPCAnatomyLayer : uint8
{
	Body     UMETA(DisplayName = "Body (layers icon)"),
	Muscles  UMETA(DisplayName = "Muscles"),
	Skeleton UMETA(DisplayName = "Skeleton")
};

/** The sections of the graph accordion (Figma "Component 3"). Only one can be open at a time. */
UENUM(BlueprintType)
enum class EIHPCAccordionSection : uint8
{
	None          UMETA(DisplayName = "None (all closed)"),
	ShowGraphs    UMETA(DisplayName = "Show graphs"),
	ActiveMuscles UMETA(DisplayName = "Active Muscles"),
	TaggedPoints  UMETA(DisplayName = "Tagged Points")
};

/**
 * One saved video / tagged moment. A video card in the "videos row" shows one of these,
 * and the "Tagged Points" list in the accordion shows the same data.
 * Example from Figma: "Soleus Muscle (R)", "Jan 4th, 2026", "13:55 p.m."
 */
USTRUCT(BlueprintType)
struct FIHPCTaggedMoment
{
	GENERATED_BODY()

	/** Big bold line on the card, e.g. "Soleus Muscle (R)". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC")
	FText Title;

	/** First small line, e.g. "Jan 4th, 2026". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC")
	FText DateText;

	/** Second small line, e.g. "13:55 p.m.". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC")
	FText TimeText;

	/** Where the white tick sits on the green-to-red bar at the top of the card.
	 *  0 = far left (green, low risk) ... 1 = far right (red, high risk). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float Severity = 0.f;

	/** Where this moment is on the avatar's animation timeline (0 = start, 1 = end).
	 *  Clicking the card jumps the existing animation timeline to this point. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float NormalizedTime = 0.f;

	FIHPCTaggedMoment() = default;

	FIHPCTaggedMoment(const FText& InTitle, const FText& InDate, const FText& InTime, float InSeverity, float InNormalizedTime)
		: Title(InTitle), DateText(InDate), TimeText(InTime), Severity(InSeverity), NormalizedTime(InNormalizedTime)
	{
	}
};

/** One coloured line in a line graph (e.g. the "Bones" line). */
USTRUCT(BlueprintType)
struct FIHPCGraphSeries
{
	GENERATED_BODY()

	/** Name shown in the legend and written to the CSV export, e.g. "Bones". */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC")
	FText Name;

	/** Colour of the line on the graph. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC")
	FLinearColor LineColor = FLinearColor::White;

	/** Colour of the little legend bar next to the toggle (exact Figma colour). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC")
	FLinearColor LegendColor = FLinearColor::White;

	/** The data points, evenly spaced from the left edge to the right edge of the graph. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC")
	TArray<float> Values;

	/** If false the line is not drawn (controlled by the Bones/Nerves/Muscles/Tendons toggles). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC")
	bool bVisible = true;
};

// -----------------------------------------------------------------------------
// Events ("delegates") shared by several widgets. Blueprint can bind to these.
// -----------------------------------------------------------------------------

/** An event with no data, e.g. "zoom button clicked". */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FIHPCSimpleEvent);

/** An event that carries one tagged moment, e.g. "this video card was chosen". */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FIHPCMomentEvent, const FIHPCTaggedMoment&, Moment);
