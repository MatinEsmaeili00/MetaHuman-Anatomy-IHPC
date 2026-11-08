// CPP_UW_IHPCLineGraph.h
// -----------------------------------------------------------------------------
// A live line graph, drawn in code (no Widget Blueprint needed).
// Figma: the "Movement Graph" and "Active Muscles" charts inside "Component 3".
//
// In Figma these charts are pictures. Here they are drawn from real numbers so
// that the Bones / Nerves / Muscles / Tendons toggles can show and hide lines,
// the download button can export the numbers, and a playhead follows the
// avatar's animation timeline.
//
// Until the project has real motion-capture data, GenerateDemoData() fills the
// graph with smooth sample curves that look like the Figma charts.
// To show real data, call SetSeries() with your own values.
//
// Usage: put it inside a SizeBox in any Widget Blueprint (it has no natural size).
// -----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "IHPC_UI/IHPCTypes.h"
#include "CPP_UW_IHPCLineGraph.generated.h"

UCLASS()
class METAHUMANTEST_API UCPP_UW_IHPCLineGraph : public UUserWidget
{
	GENERATED_BODY()

public:
	UCPP_UW_IHPCLineGraph(const FObjectInitializer& ObjectInitializer);

	/** The lines to draw. Empty + bGenerateDemoDataIfEmpty = sample curves. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IHPC|Graph")
	TArray<FIHPCGraphSeries> Series;

	/** Fill the graph with sample curves when no data was given. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IHPC|Graph")
	bool bGenerateDemoDataIfEmpty = true;

	/** Changes the shape of the sample curves (use a different seed per graph). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IHPC|Graph")
	int32 DemoSeed = 1;

	/** Bottom and top of the value axis. The Figma charts run from -1 to .5. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Graph")
	float MinValue = -1.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Graph")
	float MaxValue = 0.7f;

	/** Values that get a grid line and a label on the left (Figma: .5, 0, -.5, -1). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Graph")
	TArray<float> ValueTicks = { 0.5f, 0.f, -0.5f, -1.f };

	/** Space around the plot area for the axis labels. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	FMargin PlotPadding = FMargin(22.f, 6.f, 4.f, 10.f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	FLinearColor AxisColor = FLinearColor(1.f, 1.f, 1.f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	FLinearColor GridColor = FLinearColor(1.f, 1.f, 1.f, 0.08f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	FLinearColor LabelColor = FLinearColor(1.f, 1.f, 1.f, 0.85f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	FSlateFontInfo LabelFont;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	float LineThickness = 1.6f;

	/** Vertical line showing where the avatar animation currently is. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	bool bShowPlayhead = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	FLinearColor PlayheadColor = FLinearColor(FColor(0xB3, 0xCA, 0x7F, 0xE6));

	/** Replace all lines with new data. */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Graph")
	void SetSeries(const TArray<FIHPCGraphSeries>& NewSeries);

	UFUNCTION(BlueprintPure, Category = "IHPC|Graph")
	TArray<FIHPCGraphSeries> GetSeries() const { return Series; }

	/** Show / hide one line (0 = Bones, 1 = Nerves, 2 = Muscles, 3 = Tendons for the default layout). */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Graph")
	void SetSeriesVisible(int32 SeriesIndex, bool bVisible);

	/** Moves the playhead (0..1). Pass a negative value to hide it. */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Graph")
	void SetPlayheadTime(float NormalizedTime) { PlayheadTime = NormalizedTime; }

	/** Fills the graph with smooth sample curves (4 lines: Bones, Nerves, Muscles, Tendons). */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Graph")
	void GenerateDemoData(int32 Seed, int32 NumSamples = 120);

	/** The four lines used by the Figma legend, with names and colours but no values. */
	static TArray<FIHPCGraphSeries> MakeDefaultSeriesLayout();

protected:
	virtual void NativePreConstruct() override;

	/** All the drawing happens here: grid, axes, labels, lines, playhead. */
	virtual int32 NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
	                          FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
	                          bool bParentEnabled) const override;

private:
	/** Turns a value into a vertical pixel position inside the plot area. */
	float ValueToY(float Value, float PlotTop, float PlotHeight) const;

	/** ".5", "0", "-.5", "-1" - same style as the Figma chart labels. */
	static FString FormatTick(float Value);

	float PlayheadTime = -1.f;
};
