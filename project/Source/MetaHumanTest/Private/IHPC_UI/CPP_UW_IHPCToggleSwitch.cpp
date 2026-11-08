// CPP_UW_IHPCToggleSwitch.cpp - Figma "Component 4". See header.

#include "IHPC_UI/CPP_UW_IHPCToggleSwitch.h"

#include "Components/CheckBox.h"

void UCPP_UW_IHPCToggleSwitch::NativePreConstruct()
{
	Super::NativePreConstruct();

	// Show the starting state in the designer and in game.
	if (SwitchCheckBox)
	{
		SwitchCheckBox->SetIsChecked(bStartsOn);
	}
}

void UCPP_UW_IHPCToggleSwitch::NativeConstruct()
{
	Super::NativeConstruct();

	if (SwitchCheckBox)
	{
		SwitchCheckBox->OnCheckStateChanged.AddUniqueDynamic(this, &UCPP_UW_IHPCToggleSwitch::HandleCheckStateChanged);
	}
}

void UCPP_UW_IHPCToggleSwitch::SetIsOn(bool bOn, bool bBroadcast)
{
	if (SwitchCheckBox)
	{
		SwitchCheckBox->SetIsChecked(bOn);   // SetIsChecked does NOT fire OnCheckStateChanged
	}
	if (bBroadcast)
	{
		OnToggled.Broadcast(this, bOn);
	}
}

bool UCPP_UW_IHPCToggleSwitch::IsOn() const
{
	return SwitchCheckBox && SwitchCheckBox->IsChecked();
}

void UCPP_UW_IHPCToggleSwitch::HandleCheckStateChanged(bool bIsChecked)
{
	OnToggled.Broadcast(this, bIsChecked);
}
