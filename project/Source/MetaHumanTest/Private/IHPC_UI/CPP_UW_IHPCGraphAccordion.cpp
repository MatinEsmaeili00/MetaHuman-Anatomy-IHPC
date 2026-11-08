// CPP_UW_IHPCGraphAccordion.cpp - Figma "Component 3". See the header for the layout and behaviour.

#include "IHPC_UI/CPP_UW_IHPCGraphAccordion.h"

#include "IHPC_UI/CPP_UW_IHPCLineGraph.h"
#include "IHPC_UI/CPP_UW_IHPCListRow.h"
#include "IHPC_UI/CPP_UW_IHPCToggleSwitch.h"
#include "IHPC_UI/IHPCUILibrary.h"

#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Font.h"
#include "Engine/Texture2D.h"
#include "HAL/FileManager.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UObject/ConstructorHelpers.h"

#define LOCTEXT_NAMESPACE "IHPCGraphAccordion"

UCPP_UW_IHPCGraphAccordion::UCPP_UW_IHPCGraphAccordion(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	// Fallback list font (the Widget Blueprint sets the real Avenir font).
	if (!IsRunningDedicatedServer())
	{
		static ConstructorHelpers::FObjectFinder<UFont> DefaultFont(*UWidget::GetDefaultFontName());
		ListFont = FSlateFontInfo(DefaultFont.Object, 14, FName("Bold"));
	}

	// Sample "Active Muscles" rows - muscles that exist in the level's Muscles folder.
	ActiveMuscles = {
		LOCTEXT("Soleus", "Soleus (R)"),
		LOCTEXT("Gastrocnemius", "Gastrocnemius (R)"),
		LOCTEXT("TibialisAnterior", "Tibialis Anterior (R)"),
		LOCTEXT("VastusLateralis", "Vastus Lateralis (R)"),
		LOCTEXT("BicepsFemoris", "Biceps Femoris - Long Head (R)"),
		LOCTEXT("GluteusMaximus", "Gluteus Maximus (R)"),
	};
}

// ============================================================================
// Setup
// ============================================================================

void UCPP_UW_IHPCGraphAccordion::NativePreConstruct()
{
	Super::NativePreConstruct();

	// In the designer, show the chosen starting section so it can be laid out.
	OpenSectionState = StartOpenSection;
	RefreshSections();
	RebuildActiveMusclesList();
}

void UCPP_UW_IHPCGraphAccordion::NativeConstruct()
{
	Super::NativeConstruct();

	// Section headers
	if (Header_ShowGraphs)    Header_ShowGraphs->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCGraphAccordion::HandleShowGraphsHeader);
	if (Header_ActiveMuscles) Header_ActiveMuscles->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCGraphAccordion::HandleActiveMusclesHeader);
	if (Header_TaggedPoints)  Header_TaggedPoints->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCGraphAccordion::HandleTaggedPointsHeader);

	// Buttons under the graphs
	if (DownloadButton) DownloadButton->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCGraphAccordion::HandleDownload);
	if (ZoomButton)     ZoomButton->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCGraphAccordion::HandleZoom);

	// Bones / Nerves / Muscles / Tendons switches
	for (UCPP_UW_IHPCToggleSwitch* Toggle : GetToggles())
	{
		if (Toggle)
		{
			Toggle->OnToggled.AddUniqueDynamic(this, &UCPP_UW_IHPCGraphAccordion::HandleToggleChanged);
		}
	}

	// Give the two graphs different sample curves so they do not look identical.
	if (ActiveMusclesGraph && ActiveMusclesGraph->DemoSeed == (MovementGraph ? MovementGraph->DemoSeed : -1))
	{
		ActiveMusclesGraph->GenerateDemoData(ActiveMusclesGraph->DemoSeed + 7);
	}

	OpenSectionState = StartOpenSection;
	RefreshSections();
	ApplyToggleFilter();
	RebuildActiveMusclesList();
	RebuildTaggedPointsList();
}

void UCPP_UW_IHPCGraphAccordion::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// Keep the playhead on both graphs in step with the avatar animation.
	if (OpenSectionState == EIHPCAccordionSection::ShowGraphs)
	{
		const float Time = UIHPCUILibrary::GetAvatarTimelineTime(this);   // -1 hides the playhead
		if (MovementGraph)      MovementGraph->SetPlayheadTime(Time);
		if (ActiveMusclesGraph) ActiveMusclesGraph->SetPlayheadTime(Time);
	}
}

// ============================================================================
// Sections (open / close)
// ============================================================================

void UCPP_UW_IHPCGraphAccordion::OpenSection(EIHPCAccordionSection Section)
{
	OpenSectionState = Section;
	RefreshSections();
}

void UCPP_UW_IHPCGraphAccordion::ToggleSection(EIHPCAccordionSection Section)
{
	OpenSection(OpenSectionState == Section ? EIHPCAccordionSection::None : Section);
}

void UCPP_UW_IHPCGraphAccordion::HandleShowGraphsHeader()    { ToggleSection(EIHPCAccordionSection::ShowGraphs); }
void UCPP_UW_IHPCGraphAccordion::HandleActiveMusclesHeader() { ToggleSection(EIHPCAccordionSection::ActiveMuscles); }
void UCPP_UW_IHPCGraphAccordion::HandleTaggedPointsHeader()  { ToggleSection(EIHPCAccordionSection::TaggedPoints); }

void UCPP_UW_IHPCGraphAccordion::RefreshSections()
{
	// For each section: is it open? -> show its content panel and an "up" chevron.
	auto Apply = [this](UWidget* Content, UImage* Chevron, EIHPCAccordionSection Section)
	{
		const bool bOpen = (OpenSectionState == Section);
		if (Content)
		{
			Content->SetVisibility(bOpen ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
		}
		if (Chevron)
		{
			UTexture2D* Texture = bOpen ? ChevronUpTexture.Get() : ChevronDownTexture.Get();
			if (Texture)
			{
				Chevron->SetBrushFromTexture(Texture, /*bMatchSize*/ false);
			}
		}
	};

	Apply(Content_ShowGraphs, Chevron_ShowGraphs, EIHPCAccordionSection::ShowGraphs);
	Apply(Content_ActiveMuscles, Chevron_ActiveMuscles, EIHPCAccordionSection::ActiveMuscles);
	Apply(Content_TaggedPoints, Chevron_TaggedPoints, EIHPCAccordionSection::TaggedPoints);
}

// ============================================================================
// Toggles -> graph lines
// ============================================================================

TArray<UCPP_UW_IHPCToggleSwitch*> UCPP_UW_IHPCGraphAccordion::GetToggles() const
{
	// Order matters: it matches the series order in UCPP_UW_IHPCLineGraph::MakeDefaultSeriesLayout().
	return { Toggle_Bones, Toggle_Nerves, Toggle_Muscles, Toggle_Tendons };
}

void UCPP_UW_IHPCGraphAccordion::HandleToggleChanged(UCPP_UW_IHPCToggleSwitch* Switch, bool bIsOn)
{
	ApplyToggleFilter();
}

void UCPP_UW_IHPCGraphAccordion::ApplyToggleFilter()
{
	const TArray<UCPP_UW_IHPCToggleSwitch*> Toggles = GetToggles();

	bool bAnyOn = false;
	for (const UCPP_UW_IHPCToggleSwitch* Toggle : Toggles)
	{
		bAnyOn |= (Toggle && Toggle->IsOn());
	}

	for (int32 i = 0; i < Toggles.Num(); ++i)
	{
		// No toggle on -> show everything (Figma default). Otherwise only show switched-on lines.
		const bool bShow = !bAnyOn || (Toggles[i] && Toggles[i]->IsOn());
		if (MovementGraph)      MovementGraph->SetSeriesVisible(i, bShow);
		if (ActiveMusclesGraph) ActiveMusclesGraph->SetSeriesVisible(i, bShow);
	}
}

// ============================================================================
// Download (CSV export) and Zoom
// ============================================================================

void UCPP_UW_IHPCGraphAccordion::HandleDownload()
{
	ExportGraphsToCSV();
}

void UCPP_UW_IHPCGraphAccordion::HandleZoom()
{
	OnZoomRequested.Broadcast();   // the analysis panel shows the big graph
}

FString UCPP_UW_IHPCGraphAccordion::ExportGraphsToCSV()
{
	// <Project>/Saved/IHPC_Exports/IHPC_Graphs_20260106_135500.csv
	const FString Folder = FPaths::ProjectSavedDir() / TEXT("IHPC_Exports");
	IFileManager::Get().MakeDirectory(*Folder, /*Tree*/ true);
	const FString FilePath = FPaths::ConvertRelativePathToFull(
		Folder / FString::Printf(TEXT("IHPC_Graphs_%s.csv"), *FDateTime::Now().ToString(TEXT("%Y%m%d_%H%M%S"))));

	FString Csv;
	auto AppendGraph = [&Csv](const TCHAR* GraphName, const UCPP_UW_IHPCLineGraph* Graph)
	{
		if (!Graph)
		{
			return;
		}
		const TArray<FIHPCGraphSeries> Series = Graph->GetSeries();

		// Header row: Graph,Sample,NormalizedTime,Bones,Nerves,Muscles,Tendons
		if (Csv.IsEmpty())
		{
			Csv += TEXT("Graph,Sample,NormalizedTime");
			for (const FIHPCGraphSeries& S : Series)
			{
				Csv += TEXT(",") + S.Name.ToString();
			}
			Csv += LINE_TERMINATOR;
		}

		int32 NumSamples = 0;
		for (const FIHPCGraphSeries& S : Series)
		{
			NumSamples = FMath::Max(NumSamples, S.Values.Num());
		}

		for (int32 i = 0; i < NumSamples; ++i)
		{
			const float T = NumSamples > 1 ? static_cast<float>(i) / (NumSamples - 1) : 0.f;
			Csv += FString::Printf(TEXT("%s,%d,%.4f"), GraphName, i, T);
			for (const FIHPCGraphSeries& S : Series)
			{
				Csv += S.Values.IsValidIndex(i) ? FString::Printf(TEXT(",%.5f"), S.Values[i]) : FString(TEXT(","));
			}
			Csv += LINE_TERMINATOR;
		}
	};

	AppendGraph(TEXT("Movement Graph"), MovementGraph);
	AppendGraph(TEXT("Active Muscles"), ActiveMusclesGraph);

	if (FFileHelper::SaveStringToFile(Csv, *FilePath))
	{
		UE_LOG(LogTemp, Log, TEXT("[IHPC UI] Graphs exported to %s"), *FilePath);
		if (DownloadButton)
		{
			// Hovering the download icon now tells the user where the file went.
			DownloadButton->SetToolTipText(FText::Format(LOCTEXT("SavedTo", "Saved to {0}"), FText::FromString(FilePath)));
		}
		OnGraphsExported.Broadcast(FilePath);
		return FilePath;
	}

	UE_LOG(LogTemp, Error, TEXT("[IHPC UI] Could not write %s"), *FilePath);
	return FString();
}

// ============================================================================
// Lists: Active Muscles + Tagged Points
// ============================================================================

void UCPP_UW_IHPCGraphAccordion::SetActiveMuscles(const TArray<FText>& MuscleNames)
{
	ActiveMuscles = MuscleNames;
	RebuildActiveMusclesList();
}

void UCPP_UW_IHPCGraphAccordion::SetTaggedPoints(const TArray<FIHPCTaggedMoment>& Points)
{
	TaggedPoints = Points;
	RebuildTaggedPointsList();
}

void UCPP_UW_IHPCGraphAccordion::AddTaggedPoint(const FIHPCTaggedMoment& Point)
{
	TaggedPoints.Add(Point);
	RebuildTaggedPointsList();
}

void UCPP_UW_IHPCGraphAccordion::RebuildActiveMusclesList()
{
	if (!ActiveMusclesList)
	{
		return;
	}
	ActiveMusclesList->ClearChildren();

	for (int32 i = 0; i < ActiveMuscles.Num(); ++i)
	{
		UCPP_UW_IHPCListRow* Row = CreateWidget<UCPP_UW_IHPCListRow>(this, UCPP_UW_IHPCListRow::StaticClass());
		// The dot uses the "Muscles" legend colour from Figma (#3C2718), brightened for the dark panel.
		Row->Setup(ActiveMuscles[i], FText::GetEmpty(), UIHPCUILibrary::FigmaColor(TEXT("B07440")), ListFont, /*bClickable*/ false);
		Row->RowIndex = i;
		if (UVerticalBoxSlot* RowSlot = ActiveMusclesList->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.f, 1.f));
		}
	}
}

void UCPP_UW_IHPCGraphAccordion::RebuildTaggedPointsList()
{
	if (!TaggedPointsList)
	{
		return;
	}
	TaggedPointsList->ClearChildren();

	for (int32 i = 0; i < TaggedPoints.Num(); ++i)
	{
		const FIHPCTaggedMoment& Moment = TaggedPoints[i];
		UCPP_UW_IHPCListRow* Row = CreateWidget<UCPP_UW_IHPCListRow>(this, UCPP_UW_IHPCListRow::StaticClass());
		// Dot colour = the same green-to-red severity colour as the card's bar.
		Row->Setup(Moment.Title, Moment.TimeText, UIHPCUILibrary::SeverityToColor(Moment.Severity), ListFont, /*bClickable*/ true);
		Row->RowIndex = i;
		Row->OnRowClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCGraphAccordion::HandleTaggedPointRowClicked);
		if (UVerticalBoxSlot* RowSlot = TaggedPointsList->AddChildToVerticalBox(Row))
		{
			RowSlot->SetPadding(FMargin(0.f, 1.f));
		}
	}
}

void UCPP_UW_IHPCGraphAccordion::HandleTaggedPointRowClicked(UCPP_UW_IHPCListRow* Row)
{
	if (!Row || !TaggedPoints.IsValidIndex(Row->RowIndex))
	{
		return;
	}
	const FIHPCTaggedMoment& Moment = TaggedPoints[Row->RowIndex];
	if (bSeekTimelineOnTaggedPointClick)
	{
		UIHPCUILibrary::SeekAvatarTimeline(this, Moment.NormalizedTime);
	}
	OnTaggedPointClicked.Broadcast(Moment);
}

#undef LOCTEXT_NAMESPACE
