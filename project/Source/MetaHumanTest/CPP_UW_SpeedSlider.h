// Speed slider widget (BP_SpeedSlider): 0..1 walk -> jog -> run on the possessed pawn.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "CPP_UW_SpeedSlider.generated.h"

class USlider;
class ACPP_AbbyCharacter;

UCLASS()
class METAHUMANTEST_API UCPP_UW_SpeedSlider : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Speed")
	float MinSpeed = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Speed")
	float MaxSpeed = 1.f;

protected:
	UPROPERTY(meta=(BindWidgetOptional))
	TObjectPtr<USlider> SpeedSlider;

	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

private:
	UFUNCTION()
	void HandleSliderChanged(float NormalizedValue);

	UFUNCTION()
	void HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn);

	void BindToPawn(APawn* Pawn);

	UPROPERTY()
	TObjectPtr<ACPP_AbbyCharacter> AbbyCharacter = nullptr;
};
