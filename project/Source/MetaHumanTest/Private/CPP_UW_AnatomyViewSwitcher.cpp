#include "CPP_UW_AnatomyViewSwitcher.h"

#include "AnatomyViewComp.h"
#include "Components/Button.h"
#include "Components/Slider.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UCPP_UW_AnatomyViewSwitcher::NativeConstruct()
{
	Super::NativeConstruct();

	AnatomyViewSwitcher_Slider->SetMinValue(0.f);
	AnatomyViewSwitcher_Slider->SetMaxValue(1.f);
	AnatomyViewSwitcher_Slider->OnValueChanged.AddUniqueDynamic(this, &UCPP_UW_AnatomyViewSwitcher::OnSliderValueChanged);
	AnatomyViewSwitcher_Slider->OnMouseCaptureBegin.AddUniqueDynamic(this, &UCPP_UW_AnatomyViewSwitcher::OnSliderCaptureBegin);
	AnatomyViewSwitcher_Slider->OnMouseCaptureEnd.AddUniqueDynamic(this, &UCPP_UW_AnatomyViewSwitcher::OnSliderCaptureEnd);

	if (AnatomyMetaHuman_Btn)
	{
		AnatomyMetaHuman_Btn->OnClicked.AddUniqueDynamic(this, &UCPP_UW_AnatomyViewSwitcher::OnMetaHumanClicked);
	}
	if (AnatomyMuscle_Btn)
	{
		AnatomyMuscle_Btn->OnClicked.AddUniqueDynamic(this, &UCPP_UW_AnatomyViewSwitcher::OnMuscleClicked);
	}
	if (AnatomySkeleton_Btn)
	{
		AnatomySkeleton_Btn->OnClicked.AddUniqueDynamic(this, &UCPP_UW_AnatomyViewSwitcher::OnSkeletonClicked);
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &UCPP_UW_AnatomyViewSwitcher::HandlePossessedPawnChanged);
		BindToPawn(PC->GetPawn());
	}
}

void UCPP_UW_AnatomyViewSwitcher::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &UCPP_UW_AnatomyViewSwitcher::HandlePossessedPawnChanged);
	}
	BindToPawn(nullptr);
	Super::NativeDestruct();
}

void UCPP_UW_AnatomyViewSwitcher::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// The pawn's components can be re-created (e.g. its construction script re-runs): look again, twice a second.
	if (!IsValid(AnatomyComp))
	{
		RebindCooldown -= InDeltaTime;
		if (RebindCooldown <= 0.f)
		{
			RebindCooldown = 0.5f;
			BindToPawn(GetOwningPlayerPawn());
		}
	}
}

void UCPP_UW_AnatomyViewSwitcher::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	BindToPawn(NewPawn);
}

void UCPP_UW_AnatomyViewSwitcher::BindToPawn(APawn* Pawn)
{
	if (IsValid(AnatomyComp))
	{
		AnatomyComp->OnRevealDepthChanged.RemoveDynamic(this, &UCPP_UW_AnatomyViewSwitcher::HandleRevealDepthChanged);
	}

	// Same buttons, different pawn: whatever anatomy component the pawn has does the work (none = greyed out).
	AnatomyComp = Pawn ? Pawn->FindComponentByClass<UAnatomyViewComp>() : nullptr;
	SetIsEnabled(AnatomyComp != nullptr);

	if (AnatomyComp)
	{
		AnatomyComp->OnRevealDepthChanged.AddUniqueDynamic(this, &UCPP_UW_AnatomyViewSwitcher::HandleRevealDepthChanged);
		AnatomyViewSwitcher_Slider->SetValue(DepthToSlider(AnatomyComp->GetRevealDepth()));
	}
}

float UCPP_UW_AnatomyViewSwitcher::SliderToDepth(float SliderValue) const
{
	return bSliderMaxIsSkin ? 1.f - SliderValue : SliderValue;
}

float UCPP_UW_AnatomyViewSwitcher::DepthToSlider(float Depth) const
{
	return bSliderMaxIsSkin ? 1.f - Depth : Depth;
}

void UCPP_UW_AnatomyViewSwitcher::OnSliderValueChanged(float Value)
{
	if (AnatomyComp)
	{
		AnatomyComp->SetRevealDepth(SliderToDepth(Value), /*bAnimate*/ false);
	}
}

void UCPP_UW_AnatomyViewSwitcher::OnSliderCaptureBegin()
{
	bDragging = true;
}

void UCPP_UW_AnatomyViewSwitcher::OnSliderCaptureEnd()
{
	bDragging = false;
}

void UCPP_UW_AnatomyViewSwitcher::OnMetaHumanClicked()
{
	if (AnatomyComp)
	{
		AnatomyComp->ShowLayer(0);
	}
}

void UCPP_UW_AnatomyViewSwitcher::OnMuscleClicked()
{
	if (AnatomyComp)
	{
		AnatomyComp->ShowLayer(1);
	}
}

void UCPP_UW_AnatomyViewSwitcher::OnSkeletonClicked()
{
	if (AnatomyComp)
	{
		AnatomyComp->ShowLayer(AnatomyComp->GetNumLayers() - 1);
	}
}

void UCPP_UW_AnatomyViewSwitcher::HandleRevealDepthChanged(float RevealDepth)
{
	// Keep the slider in sync while a button animates the depth (but don't fight the user's drag).
	if (!bDragging)
	{
		AnatomyViewSwitcher_Slider->SetValue(DepthToSlider(RevealDepth));
	}
}
