// CPP_UW_IHPCLineGraph.cpp - the live line graph. See the header for the why.

#include "IHPC_UI/CPP_UW_IHPCLineGraph.h"

#include "IHPC_UI/IHPCUILibrary.h"
#include "Components/Widget.h"
#include "Engine/Font.h"
#include "Fonts/FontMeasure.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Rendering/SlateRenderer.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "IHPCLineGraph"

UCPP_UW_IHPCLineGraph::UCPP_UW_IHPCLineGraph(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Default label font = Unreal's built-in UI font (Roboto), size 8 - like the small
	// serif numbers on the Figma chart. Designers can change LabelFont in the details panel.
	if (!IsRunningDedicatedServer())
	{
		static ConstructorHelpers::FObjectFinder<UFont> DefaultFont(*UWidget::GetDefaultFontName());
		LabelFont = FSlateFontInfo(DefaultFont.Object, 8, FName("Regular"));
	}
}

TArray<FIHPCGraphSeries> UCPP_UW_IHPCLineGraph::MakeDefaultSeriesLayout()
{
	// Names + legend colours come straight from Figma "Component 3" (the small coloured
	// bars next to the toggles). The line colours are brighter versions of the same
	// colours so the lines are readable on the dark panel, like in the Figma chart image.
	struct FDef { const TCHAR* Name; const TCHAR* Legend; const TCHAR* Line; };
	static const FDef Defs[] = {
		{ TEXT("Bones"),   TEXT("1D277E"), TEXT("4F63E0") },
		{ TEXT("Nerves"),  TEXT("5D601E"), TEXT("B4B847") },
		{ TEXT("Muscles"), TEXT("3C2718"), TEXT("B07440") },
		{ TEXT("Tendons"), TEXT("713D53"), TEXT("D77AA0") },
	};

	TArray<FIHPCGraphSeries> Out;
	for (const FDef& D : Defs)
	{
		FIHPCGraphSeries S;
		S.Name = FText::FromString(D.Name);
		S.LegendColor = UIHPCUILibrary::FigmaColor(D.Legend);
		S.LineColor = UIHPCUILibrary::FigmaColor(D.Line);
		Out.Add(S);
	}
	return Out;
}

void UCPP_UW_IHPCLineGraph::NativePreConstruct()
{
	Super::NativePreConstruct();

	// Runs in the designer and in game, so the sample curves are visible while editing too.
	if (Series.Num() == 0 && bGenerateDemoDataIfEmpty)
	{
		GenerateDemoData(DemoSeed);
	}
}

void UCPP_UW_IHPCLineGraph::SetSeries(const TArray<FIHPCGraphSeries>& NewSeries)
{
	Series = NewSeries;
}

void UCPP_UW_IHPCLineGraph::SetSeriesVisible(int32 SeriesIndex, bool bVisible)
{
	if (Series.IsValidIndex(SeriesIndex))
	{
		Series[SeriesIndex].bVisible = bVisible;
	}
}

void UCPP_UW_IHPCLineGraph::GenerateDemoData(int32 Seed, int32 NumSamples)
{
	NumSamples = FMath::Max(NumSamples, 2);

	// Keep the visibility the user already chose, if lines already existed.
	TArray<bool> PreviousVisibility;
	for (const FIHPCGraphSeries& S : Series)
	{
		PreviousVisibility.Add(S.bVisible);
	}

	Series = MakeDefaultSeriesLayout();
	for (int32 SeriesIndex = 0; SeriesIndex < Series.Num(); ++SeriesIndex)
	{
		// A seeded random stream gives the same curves every time for the same seed.
		FRandomStream Random(Seed * 97 + SeriesIndex * 13);
		const float Freq1 = Random.FRandRange(0.8f, 1.8f);
		const float Freq2 = Random.FRandRange(2.2f, 3.8f);
		const float Freq3 = Random.FRandRange(4.5f, 7.0f);
		const float Phase1 = Random.FRandRange(0.f, 2.f * PI);
		const float Phase2 = Random.FRandRange(0.f, 2.f * PI);
		const float Phase3 = Random.FRandRange(0.f, 2.f * PI);
		const float Offset = Random.FRandRange(-0.45f, -0.2f);

		TArray<float>& Values = Series[SeriesIndex].Values;
		Values.SetNum(NumSamples);
		for (int32 i = 0; i < NumSamples; ++i)
		{
			const float T = static_cast<float>(i) / (NumSamples - 1);
			// Three overlapping waves = a smooth, organic-looking line like the Figma chart.
			const float V = Offset
				+ 0.42f * FMath::Sin(2.f * PI * Freq1 * T + Phase1)
				+ 0.18f * FMath::Sin(2.f * PI * Freq2 * T + Phase2)
				+ 0.06f * FMath::Sin(2.f * PI * Freq3 * T + Phase3);
			Values[i] = FMath::Clamp(V, MinValue + 0.05f, MaxValue - 0.05f);
		}

		if (PreviousVisibility.IsValidIndex(SeriesIndex))
		{
			Series[SeriesIndex].bVisible = PreviousVisibility[SeriesIndex];
		}
	}
}

float UCPP_UW_IHPCLineGraph::ValueToY(float Value, float PlotTop, float PlotHeight) const
{
	const float Range = FMath::Max(MaxValue - MinValue, KINDA_SMALL_NUMBER);
	const float Alpha = FMath::Clamp((Value - MinValue) / Range, 0.f, 1.f);
	return PlotTop + (1.f - Alpha) * PlotHeight;   // screen Y grows downwards, values grow upwards
}

FString UCPP_UW_IHPCLineGraph::FormatTick(float Value)
{
	if (FMath::IsNearlyEqual(Value, FMath::RoundToFloat(Value), 0.001f))
	{
		return FString::Printf(TEXT("%d"), FMath::RoundToInt(Value));   // "0", "-1"
	}
	FString Text = FString::Printf(TEXT("%.1f"), Value);               // "0.5", "-0.5"
	if (Text.StartsWith(TEXT("0.")))                                    // drop the leading zero
	{
		Text.RightChopInline(1);                                        // ".5"  (Figma style)
	}
	else if (Text.StartsWith(TEXT("-0.")))
	{
		Text = TEXT("-") + Text.RightChop(2);                           // "-.5" (Figma style)
	}
	return Text;
}

int32 UCPP_UW_IHPCLineGraph::NativePaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect,
                                         FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle,
                                         bool bParentEnabled) const
{
	LayerId = Super::NativePaint(Args, AllottedGeometry, MyCullingRect, OutDrawElements, LayerId, InWidgetStyle, bParentEnabled);

	const FVector2D Size = AllottedGeometry.GetLocalSize();
	if (Size.X < 20.0 || Size.Y < 20.0)
	{
		return LayerId;   // too small to draw anything useful
	}

	// --- Plot area (the rectangle the lines live in) --------------------------------
	const float Left = PlotPadding.Left;
	const float Top = PlotPadding.Top;
	const float PlotW = Size.X - PlotPadding.Left - PlotPadding.Right;
	const float PlotH = Size.Y - PlotPadding.Top - PlotPadding.Bottom;
	const float Bottom = Top + PlotH;
	const float Right = Left + PlotW;

	const FPaintGeometry PaintGeometry = AllottedGeometry.ToPaintGeometry();
	const ESlateDrawEffect Effect = (bParentEnabled && GetIsEnabled()) ? ESlateDrawEffect::None : ESlateDrawEffect::DisabledEffect;
	const FLinearColor Tint = InWidgetStyle.GetColorAndOpacityTint();   // respects the widget's opacity

	auto DrawLine = [&](const FVector2f& A, const FVector2f& B, const FLinearColor& Color, float Thickness, int32 Layer)
	{
		TArray<FVector2f> Points = { A, B };
		FSlateDrawElement::MakeLines(OutDrawElements, Layer, PaintGeometry, Points, Effect, Color * Tint, true, Thickness);
	};

	// --- 1. Grid --------------------------------------------------------------------
	const int32 GridLayer = LayerId + 1;
	for (const float Tick : ValueTicks)
	{
		const float Y = ValueToY(Tick, Top, PlotH);
		DrawLine(FVector2f(Left, Y), FVector2f(Right, Y), GridColor, 1.f, GridLayer);
	}
	for (int32 i = 1; i <= 4; ++i)   // 4 vertical lines = 5 equal columns
	{
		const float X = Left + PlotW * (i / 5.f);
		DrawLine(FVector2f(X, Top), FVector2f(X, Bottom), GridColor, 1.f, GridLayer);
	}

	// --- 2. Axes + small tick marks --------------------------------------------------
	const int32 AxisLayer = GridLayer + 1;
	DrawLine(FVector2f(Left, Top), FVector2f(Left, Bottom), AxisColor, 1.5f, AxisLayer);      // left axis
	DrawLine(FVector2f(Left, Bottom), FVector2f(Right, Bottom), AxisColor, 1.5f, AxisLayer);  // bottom axis
	for (int32 i = 1; i <= 5; ++i)
	{
		const float X = Left + PlotW * (i / 5.f);
		DrawLine(FVector2f(X, Bottom), FVector2f(X, Bottom - 3.f), AxisColor, 1.f, AxisLayer);
	}

	// --- 3. Value labels on the left ---------------------------------------------------
	const TSharedRef<FSlateFontMeasure> FontMeasure = FSlateApplication::Get().GetRenderer()->GetFontMeasureService();
	for (const float Tick : ValueTicks)
	{
		const FString Text = FormatTick(Tick);
		const FVector2D TextSize = FVector2D(FontMeasure->Measure(Text, LabelFont));
		const float X = Left - 4.f - static_cast<float>(TextSize.X);           // right-aligned against the axis
		const float Y = ValueToY(Tick, Top, PlotH) - static_cast<float>(TextSize.Y) * 0.5f;
		FSlateDrawElement::MakeText(OutDrawElements, AxisLayer,
		                            AllottedGeometry.ToPaintGeometry(FVector2f(TextSize), FSlateLayoutTransform(1.f, FVector2f(X, Y))),
		                            Text, LabelFont, Effect, LabelColor * Tint);
	}

	// --- 4. The data lines -----------------------------------------------------------------
	const int32 LineLayer = AxisLayer + 1;
	for (const FIHPCGraphSeries& S : Series)
	{
		if (!S.bVisible || S.Values.Num() < 2)
		{
			continue;
		}
		TArray<FVector2f> Points;
		Points.Reserve(S.Values.Num());
		for (int32 i = 0; i < S.Values.Num(); ++i)
		{
			const float X = Left + PlotW * (static_cast<float>(i) / (S.Values.Num() - 1));
			Points.Add(FVector2f(X, ValueToY(S.Values[i], Top, PlotH)));
		}
		FSlateDrawElement::MakeLines(OutDrawElements, LineLayer, PaintGeometry, Points, Effect, S.LineColor * Tint, true, LineThickness);
	}

	// --- 5. Playhead (where the avatar animation is right now) ----------------------------
	int32 TopLayer = LineLayer;
	if (bShowPlayhead && PlayheadTime >= 0.f)
	{
		TopLayer = LineLayer + 1;
		const float X = Left + PlotW * FMath::Clamp(PlayheadTime, 0.f, 1.f);
		DrawLine(FVector2f(X, Top), FVector2f(X, Bottom), PlayheadColor, 1.5f, TopLayer);
	}

	return TopLayer;
}

#undef LOCTEXT_NAMESPACE
