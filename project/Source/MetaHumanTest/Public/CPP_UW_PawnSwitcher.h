// Pawn switcher widget (WB_PawnSwitcher): Up / Down ask the player controller to possess the next / previous pawn.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPP_UW_PawnSwitcher.generated.h"

class UButton;

UCLASS()
class METAHUMANTEST_API UCPP_UW_PawnSwitcher : public UUserWidget
{
	GENERATED_BODY()

protected:
	virtual void NativeConstruct() override;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> SwitchPawnUpBtn;

	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UButton> SwitchPawnDownBtn;

	UFUNCTION()
	void OnSwitchPawnUpClicked();

	UFUNCTION()
	void OnSwitchPawnDownClicked();
};
