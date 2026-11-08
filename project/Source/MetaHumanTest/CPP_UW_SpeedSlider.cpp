#include "CPP_UW_SpeedSlider.h"

#include "CPP_AbbyCharacter.h"
#include "Components/Slider.h"
#include "GameFramework/PlayerController.h"

void UCPP_UW_SpeedSlider::NativeConstruct()
{
	Super::NativeConstruct();

	if (SpeedSlider)
	{
		SpeedSlider->OnValueChanged.AddUniqueDynamic(this, &UCPP_UW_SpeedSlider::HandleSliderChanged);
	}

	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.AddUniqueDynamic(this, &UCPP_UW_SpeedSlider::HandlePossessedPawnChanged);
		BindToPawn(PC->GetPawn());
	}
}

void UCPP_UW_SpeedSlider::NativeDestruct()
{
	if (APlayerController* PC = GetOwningPlayer())
	{
		PC->OnPossessedPawnChanged.RemoveDynamic(this, &UCPP_UW_SpeedSlider::HandlePossessedPawnChanged);
	}
	Super::NativeDestruct();
}

void UCPP_UW_SpeedSlider::HandlePossessedPawnChanged(APawn* OldPawn, APawn* NewPawn)
{
	BindToPawn(NewPawn);
}

void UCPP_UW_SpeedSlider::BindToPawn(APawn* Pawn)
{
	// Only pawns with a locomotion speed react; anything else greys the slider out (used to crash here).
	AbbyCharacter = Cast<ACPP_AbbyCharacter>(Pawn);
	SetIsEnabled(AbbyCharacter != nullptr);

	if (AbbyCharacter && SpeedSlider && MaxSpeed > MinSpeed)
	{
		SpeedSlider->SetValue((AbbyCharacter->Speed - MinSpeed) / (MaxSpeed - MinSpeed));
	}
}

void UCPP_UW_SpeedSlider::HandleSliderChanged(float NormalizedValue)
{
	if (AbbyCharacter)
	{
		AbbyCharacter->Speed = FMath::Lerp(MinSpeed, MaxSpeed, FMath::Clamp(NormalizedValue, 0.f, 1.f));
	}
}
