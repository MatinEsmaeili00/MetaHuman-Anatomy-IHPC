#include "CPP_UW_PawnSwitcher.h"

#include "Components/Button.h"
#include "MetaHumanTest/CPP_PlayerController.h"

void UCPP_UW_PawnSwitcher::NativeConstruct()
{
	Super::NativeConstruct();

	if (SwitchPawnUpBtn)
	{
		SwitchPawnUpBtn->OnClicked.AddUniqueDynamic(this, &UCPP_UW_PawnSwitcher::OnSwitchPawnUpClicked);
	}
	if (SwitchPawnDownBtn)
	{
		SwitchPawnDownBtn->OnClicked.AddUniqueDynamic(this, &UCPP_UW_PawnSwitcher::OnSwitchPawnDownClicked);
	}
}

void UCPP_UW_PawnSwitcher::OnSwitchPawnUpClicked()
{
	// Possession is the controller's job; the other widgets follow via OnPossessedPawnChanged.
	if (ACPP_PlayerController* PC = Cast<ACPP_PlayerController>(GetOwningPlayer()))
	{
		PC->SwitchPawn(+1);
	}
}

void UCPP_UW_PawnSwitcher::OnSwitchPawnDownClicked()
{
	if (ACPP_PlayerController* PC = Cast<ACPP_PlayerController>(GetOwningPlayer()))
	{
		PC->SwitchPawn(-1);
	}
}
