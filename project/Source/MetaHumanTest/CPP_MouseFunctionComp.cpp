#include "CPP_MouseFunctionComp.h"

#include "GameFramework/Actor.h"
#include "GameFramework/SpringArmComponent.h"

UCPP_MouseFunctionComp::UCPP_MouseFunctionComp()
{
	PrimaryComponentTick.bCanEverTick = false; // input-driven, nothing to do per frame
}

void UCPP_MouseFunctionComp::BeginPlay()
{
	Super::BeginPlay();
	SyncFromCurrentSpringArm();

	if (!SpringArmComponent)
	{
		UE_LOG(LogTemp, Warning, TEXT("%s: CPP_MouseFunctionComp found no Spring Arm Component"), *GetNameSafe(GetOwner()));
	}
}

void UCPP_MouseFunctionComp::OrbitFromMouse(const FVector2D& MouseDelta)
{
	if (!SpringArmComponent)
	{
		return;
	}

	CurrentArmRot.Yaw += MouseDelta.X * YawSpeedDegPerPx;
	CurrentArmRot.Pitch = FMath::Clamp(CurrentArmRot.Pitch + MouseDelta.Y * PitchSpeedDegPerPx, MinPitch, MaxPitch);

	SpringArmComponent->SetRelativeRotation(CurrentArmRot);
}

void UCPP_MouseFunctionComp::ZoomCamera(float AxisValue)
{
	if (!SpringArmComponent || FMath::IsNearlyZero(AxisValue))
	{
		return;
	}

	// Positive AxisValue -> zoom in (shorter arm), negative -> zoom out (longer arm)
	CurrentCameraDistance = FMath::Clamp(CurrentCameraDistance - AxisValue * ZoomSpeed, MinZoom, MaxZoom);
	SpringArmComponent->TargetArmLength = CurrentCameraDistance;
}

void UCPP_MouseFunctionComp::PanFromMouse(const FVector2D& MouseDelta)
{
	if (!SpringArmComponent)
	{
		return;
	}

	// Use the camera rotation so the pan feels screen-space
	const FRotator CamRot = SpringArmComponent->GetComponentRotation();
	const FVector Right = FRotationMatrix(CamRot).GetScaledAxis(EAxis::Y);
	const FVector Up = FRotationMatrix(CamRot).GetScaledAxis(EAxis::Z);

	const FVector WorldDelta = (Right * MouseDelta.X + Up * MouseDelta.Y) * PanSpeed;
	SpringArmComponent->SetRelativeLocation(SpringArmComponent->GetRelativeLocation() + WorldDelta);
}

void UCPP_MouseFunctionComp::SyncFromCurrentSpringArm()
{
	if (!SpringArmComponent && GetOwner())
	{
		SpringArmComponent = GetOwner()->FindComponentByClass<USpringArmComponent>();
	}
	if (!SpringArmComponent)
	{
		return;
	}

	CurrentCameraDistance = SpringArmComponent->TargetArmLength;
	CurrentArmRot = SpringArmComponent->GetRelativeRotation();
}
