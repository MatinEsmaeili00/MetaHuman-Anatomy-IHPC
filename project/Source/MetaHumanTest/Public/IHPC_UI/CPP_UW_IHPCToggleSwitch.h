// CPP_UW_IHPCToggleSwitch.h
// -----------------------------------------------------------------------------
// The little on/off switch. Figma: "Component 4" (38 x 20 px).
//   Default  (off): grey track, white knob on the left
//   Variant2 (on) : green #B3CA7F track, white knob on the right
//
// Widget Blueprint: /Game/UserInterface/IHPC/Widgets/WBP_IHPC_ToggleSwitch
//   It must contain a CheckBox named "SwitchCheckBox". The CheckBox style uses the
//   textures T_IHPC_Toggle_Off / T_IHPC_Toggle_On, so the look lives in the Blueprint
//   and the behaviour lives here.
// -----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPP_UW_IHPCToggleSwitch.generated.h"

class UCheckBox;
class UCPP_UW_IHPCToggleSwitch;

/** Fired when the user flips the switch. Passes the switch itself so one handler can serve many switches. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIHPCToggleChanged, UCPP_UW_IHPCToggleSwitch*, Switch, bool, bIsOn);

UCLASS(Abstract)
class METAHUMANTEST_API UCPP_UW_IHPCToggleSwitch : public UUserWidget
{
	GENERATED_BODY()

public:
	/** Starting state of the switch. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IHPC|Toggle")
	bool bStartsOn = false;

	UPROPERTY(BlueprintAssignable, Category = "IHPC|Toggle")
	FIHPCToggleChanged OnToggled;

	/** Changes the switch from code. bBroadcast = also fire OnToggled. */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Toggle")
	void SetIsOn(bool bOn, bool bBroadcast = false);

	UFUNCTION(BlueprintPure, Category = "IHPC|Toggle")
	bool IsOn() const;

protected:
	virtual void NativePreConstruct() override;
	virtual void NativeConstruct() override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UCheckBox> SwitchCheckBox;

private:
	UFUNCTION()
	void HandleCheckStateChanged(bool bIsChecked);
};
