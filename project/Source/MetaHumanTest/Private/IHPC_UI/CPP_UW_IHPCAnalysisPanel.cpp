// CPP_UW_IHPCAnalysisPanel.cpp - the big analysis panel. See header.

#include "IHPC_UI/CPP_UW_IHPCAnalysisPanel.h"

#include "IHPC_UI/CPP_UW_IHPCGraphAccordion.h"
#include "IHPC_UI/CPP_UW_IHPCLineGraph.h"
#include "IHPC_UI/CPP_UW_IHPCVideosRow.h"
#include "IHPC_UI/IHPCUILibrary.h"

#include "Components/Button.h"

void UCPP_UW_IHPCAnalysisPanel::NativePreConstruct()
{
	Super::NativePreConstruct();
	SetZoomedGraphVisible(false);   // start with the viewer area empty
}

void UCPP_UW_IHPCAnalysisPanel::NativeConstruct()
{
	Super::NativeConstruct();

	if (GraphAccordion)
	{
		GraphAccordion->OnZoomRequested.AddUniqueDynamic(this, &UCPP_UW_IHPCAnalysisPanel::HandleZoomRequested);

		// "Tagged Points" lists the same moments as the video cards, so both stay in sync.
		if (VideosRow)
		{
			GraphAccordion->SetTaggedPoints(VideosRow->GetMoments());
		}
	}

	if (InspectButton)
	{
		InspectButton->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCAnalysisPanel::HandleInspectClicked);
	}
}

void UCPP_UW_IHPCAnalysisPanel::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// The large graph gets the same playhead as the small ones.
	if (bZoomedGraphVisible && ZoomedGraph)
	{
		ZoomedGraph->SetPlayheadTime(UIHPCUILibrary::GetAvatarTimelineTime(this));
	}
}

void UCPP_UW_IHPCAnalysisPanel::SetZoomedGraphVisible(bool bVisible)
{
	bZoomedGraphVisible = bVisible;

	// Copy the lines (and which ones are toggled on) from the small Movement Graph.
	if (bVisible && ZoomedGraph && GraphAccordion && GraphAccordion->GetMovementGraph())
	{
		ZoomedGraph->SetSeries(GraphAccordion->GetMovementGraph()->GetSeries());
	}

	if (ZoomedGraphContainer)
	{
		ZoomedGraphContainer->SetVisibility(bVisible ? ESlateVisibility::SelfHitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UCPP_UW_IHPCAnalysisPanel::AddSavedVideo(const FIHPCTaggedMoment& Moment)
{
	if (VideosRow)
	{
		VideosRow->AddMoment(Moment);         // new card at the end of the row
	}
	if (GraphAccordion)
	{
		GraphAccordion->AddTaggedPoint(Moment);
	}
}

void UCPP_UW_IHPCAnalysisPanel::AddTaggedPoint(const FIHPCTaggedMoment& Moment)
{
	if (GraphAccordion)
	{
		GraphAccordion->AddTaggedPoint(Moment);
	}
}

TArray<FIHPCTaggedMoment> UCPP_UW_IHPCAnalysisPanel::GetSavedVideos() const
{
	return VideosRow ? VideosRow->GetMoments() : TArray<FIHPCTaggedMoment>();
}

void UCPP_UW_IHPCAnalysisPanel::HandleZoomRequested()
{
	SetZoomedGraphVisible(!bZoomedGraphVisible);   // zoom button toggles the large graph
}

void UCPP_UW_IHPCAnalysisPanel::HandleInspectClicked()
{
	if (bZoomedGraphVisible)
	{
		SetZoomedGraphVisible(false);   // back to the empty viewer
		return;
	}
	OnInspectClicked.Broadcast();       // hook for a future 3D close-up view
}
