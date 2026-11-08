#include "CPP_UW_MuscleInfo.h"

#include "AnatomyPickerComp.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Button.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/TextBlock.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"

void UCPP_UW_MuscleInfo::NativeConstruct()
{
	Super::NativeConstruct();

	TooltipBox->SetVisibility(ESlateVisibility::Collapsed);
	PopupBox->SetVisibility(ESlateVisibility::Collapsed);
	if (PopupCloseBtn)
	{
		PopupCloseBtn->OnClicked.AddUniqueDynamic(this, &UCPP_UW_MuscleInfo::HandleCloseClicked);
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &UCPP_UW_MuscleInfo::HandlePossessedPawnChanged);
		BindToPawn(PC->GetPawn());
	}
}

void UCPP_UW_MuscleInfo::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &UCPP_UW_MuscleInfo::HandlePossessedPawnChanged);
	}
	BindToPawn(nullptr);
	Super::NativeDestruct();
}

void UCPP_UW_MuscleInfo::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	BindToPawn(NewPawn);
}

void UCPP_UW_MuscleInfo::BindToPawn(APawn* Pawn)
{
	if (IsValid(Picker))
	{
		Picker->OnHoverChanged.RemoveDynamic(this, &UCPP_UW_MuscleInfo::HandleHoverChanged);
		Picker->OnSelectionChanged.RemoveDynamic(this, &UCPP_UW_MuscleInfo::HandleSelectionChanged);
	}

	Picker = Pawn ? Pawn->FindComponentByClass<UAnatomyPickerComp>() : nullptr;
	TooltipBox->SetVisibility(ESlateVisibility::Collapsed);
	PopupBox->SetVisibility(ESlateVisibility::Collapsed);

	if (Picker)
	{
		Picker->OnHoverChanged.AddUniqueDynamic(this, &UCPP_UW_MuscleInfo::HandleHoverChanged);
		Picker->OnSelectionChanged.AddUniqueDynamic(this, &UCPP_UW_MuscleInfo::HandleSelectionChanged);
	}
}

void UCPP_UW_MuscleInfo::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (!IsValid(Picker))
	{
		RebindCooldown -= InDeltaTime;
		if (RebindCooldown <= 0.f)
		{
			RebindCooldown = 0.5f;
			BindToPawn(GetOwningPlayerPawn());
		}
		return;
	}

	// Keep the tooltip next to the mouse.
	if (TooltipBox->GetVisibility() != ESlateVisibility::Collapsed)
	{
		if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(TooltipBox->Slot))
		{
			CanvasSlot->SetPosition(UWidgetLayoutLibrary::GetMousePositionOnViewport(this) + TooltipOffset);
		}
	}
}

void UCPP_UW_MuscleInfo::HandleHoverChanged(int32 MuscleId)
{
	FMuscleInfo Info;
	if (MuscleId > 0 && Picker && Picker->GetMuscleInfo(MuscleId, Info))
	{
		MuscleNameText->SetText(FText::Format(NSLOCTEXT("MuscleInfo", "Tooltip", "{0} ({1})"), Info.Name, Info.Side));
		TooltipBox->SetVisibility(ESlateVisibility::HitTestInvisible);
	}
	else
	{
		TooltipBox->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCPP_UW_MuscleInfo::HandleSelectionChanged(int32 MuscleId)
{
	FMuscleInfo Info;
	if (MuscleId > 0 && Picker && Picker->GetMuscleInfo(MuscleId, Info))
	{
		PopupTitle->SetText(FText::Format(NSLOCTEXT("MuscleInfo", "Title", "{0} - {1}"), Info.Name, Info.Side));
		PopupRegion->SetText(Info.Region);
		PopupFunction->SetText(FText::Format(NSLOCTEXT("MuscleInfo", "Function", "What it does: {0}"), Info.Function));
		PopupRunning->SetText(FText::Format(NSLOCTEXT("MuscleInfo", "Running", "When running: {0}"), Info.Running));
		PopupInjury->SetText(FText::Format(NSLOCTEXT("MuscleInfo", "Injury", "Watch out for: {0}"), Info.Injury));
		PopupBox->SetVisibility(ESlateVisibility::Visible);
	}
	else
	{
		PopupBox->SetVisibility(ESlateVisibility::Collapsed);
	}
}

void UCPP_UW_MuscleInfo::HandleCloseClicked()
{
	if (Picker)
	{
		Picker->ClearSelection(); // fires HandleSelectionChanged(0) -> hides the popup
	}
	PopupBox->SetVisibility(ESlateVisibility::Collapsed);
}
