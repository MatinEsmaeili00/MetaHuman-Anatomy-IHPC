// CPP_UW_IHPCLayerSwitcher.cpp - the Body / Muscles / Skeleton buttons. See the header for the Figma mapping.

#include "IHPC_UI/CPP_UW_IHPCLayerSwitcher.h"

#include "AnatomyViewComp.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Engine/Texture2D.h"

void UCPP_UW_IHPCLayerSwitcher::NativePreConstruct()
{
	Super::NativePreConstruct();

	SelectedLayer = DefaultLayer;
	RefreshVisuals();
}

void UCPP_UW_IHPCLayerSwitcher::NativeConstruct()
{
	// The parent binds its (collapsed) slider and finds the AnatomyView component on the possessed pawn.
	Super::NativeConstruct();

	if (BodyLayer_Btn)
	{
		BodyLayer_Btn->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCLayerSwitcher::HandleBodyClicked);
	}
	if (MusclesLayer_Btn)
	{
		MusclesLayer_Btn->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCLayerSwitcher::HandleMusclesClicked);
	}
	if (SkeletonLayer_Btn)
	{
		SkeletonLayer_Btn->OnClicked.AddUniqueDynamic(this, &UCPP_UW_IHPCLayerSwitcher::HandleSkeletonClicked);
	}

	SelectedLayer = DefaultLayer;
	RefreshVisuals();

	if (bApplyDefaultLayerOnStart)
	{
		SelectLayer(DefaultLayer);
	}
	else
	{
		SyncSelectionFromPawn();
	}
}

void UCPP_UW_IHPCLayerSwitcher::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	// The parent re-binds AnatomyComp when the controller possesses another pawn (or the component is re-created).
	Super::NativeTick(MyGeometry, InDeltaTime);

	if (SyncedAnatomyComp.Get() != AnatomyComp.Get())
	{
		SyncSelectionFromPawn();
	}
}

void UCPP_UW_IHPCLayerSwitcher::HandleBodyClicked()     { SelectLayer(EIHPCAnatomyLayer::Body); }
void UCPP_UW_IHPCLayerSwitcher::HandleMusclesClicked()  { SelectLayer(EIHPCAnatomyLayer::Muscles); }
void UCPP_UW_IHPCLayerSwitcher::HandleSkeletonClicked() { SelectLayer(EIHPCAnatomyLayer::Skeleton); }

void UCPP_UW_IHPCLayerSwitcher::SelectLayer(EIHPCAnatomyLayer Layer)
{
	SelectedLayer = Layer;
	RefreshVisuals();

	const float RevealDepth = GetRevealDepthForLayer(Layer);
	if (RevealDepth >= 0.f)
	{
		AnatomyComp->SetRevealDepth(RevealDepth, /*bAnimate*/ true); // animates at the component's TransitionSpeed
		SyncedAnatomyComp = AnatomyComp;
	}
	else if (!bWarnedMissingLayers)
	{
		bWarnedMissingLayers = true;
		UE_LOG(LogTemp, Warning, TEXT("[IHPC] Layer switcher: the possessed pawn has no AnatomyView component "
			"(UAnatomyViewComp) with layers, so only the button highlight changed."));
	}

	OnLayerChanged.Broadcast(Layer, RevealDepth);
}

FName UCPP_UW_IHPCLayerSwitcher::GetLayerName(EIHPCAnatomyLayer Layer) const
{
	switch (Layer)
	{
	case EIHPCAnatomyLayer::Muscles:  return MusclesLayerName;
	case EIHPCAnatomyLayer::Skeleton: return SkeletonLayerName;
	case EIHPCAnatomyLayer::Body:
	default:                          return BodyLayerName;
	}
}

int32 UCPP_UW_IHPCLayerSwitcher::GetAnatomyLayerIndex(EIHPCAnatomyLayer Layer) const
{
	if (!CanDriveAvatarLayers())
	{
		return INDEX_NONE;
	}

	const int32 ByName = AnatomyComp->FindLayerIndex(GetLayerName(Layer));
	if (ByName != INDEX_NONE)
	{
		return ByName;
	}

	// The pawn names its layers differently: outermost / second / innermost.
	const int32 Last = AnatomyComp->GetNumLayers() - 1;
	switch (Layer)
	{
	case EIHPCAnatomyLayer::Muscles:  return FMath::Min(1, Last);
	case EIHPCAnatomyLayer::Skeleton: return Last;
	case EIHPCAnatomyLayer::Body:
	default:                          return 0;
	}
}

float UCPP_UW_IHPCLayerSwitcher::GetRevealDepthForLayer(EIHPCAnatomyLayer Layer) const
{
	if (!CanDriveAvatarLayers())
	{
		return -1.f;
	}
	if (bUseSkinAmounts)
	{
		// Skin amount 1 = all skin (depth 0), 0.5 = muscles (depth 0.5), 0 = bones (depth 1).
		float Amount = BodySkinAmount;
		switch (Layer)
		{
		case EIHPCAnatomyLayer::Muscles:  Amount = MusclesSkinAmount; break;
		case EIHPCAnatomyLayer::Skeleton: Amount = SkeletonSkinAmount; break;
		case EIHPCAnatomyLayer::Body:
		default:                          break;
		}
		return 1.f - FMath::Clamp(Amount, 0.f, 1.f);
	}
	return AnatomyComp->GetLayerDepth(GetAnatomyLayerIndex(Layer));
}

bool UCPP_UW_IHPCLayerSwitcher::CanDriveAvatarLayers() const
{
	return IsValid(AnatomyComp) && AnatomyComp->GetNumLayers() > 0;
}

void UCPP_UW_IHPCLayerSwitcher::SyncSelectionFromPawn()
{
	SyncedAnatomyComp = AnatomyComp;
	if (!CanDriveAvatarLayers())
	{
		return; // the parent greys the widget out; keep the last highlight
	}

	// Highlight the button whose layer is closest to where the pawn currently is.
	const float Depth = AnatomyComp->GetRevealDepth();
	EIHPCAnatomyLayer Best = SelectedLayer;
	float BestDistance = TNumericLimits<float>::Max();
	for (const EIHPCAnatomyLayer Layer : { EIHPCAnatomyLayer::Body, EIHPCAnatomyLayer::Muscles, EIHPCAnatomyLayer::Skeleton })
	{
		const float Distance = FMath::Abs(GetRevealDepthForLayer(Layer) - Depth);
		if (Distance < BestDistance)
		{
			BestDistance = Distance;
			Best = Layer;
		}
	}
	SelectedLayer = Best;
	RefreshVisuals();
}

void UCPP_UW_IHPCLayerSwitcher::SetSkeletonBadgeVisible(bool bVisible)
{
	bShowSkeletonBadge = bVisible;
	RefreshVisuals();
}

void UCPP_UW_IHPCLayerSwitcher::RefreshVisuals()
{
	ApplyButtonLook(BodyLayer_Btn, SelectedLayer == EIHPCAnatomyLayer::Body ? BodySelectedTexture : BodyUnselectedTexture);
	ApplyButtonLook(MusclesLayer_Btn, SelectedLayer == EIHPCAnatomyLayer::Muscles ? MusclesSelectedTexture : MusclesUnselectedTexture);
	ApplyButtonLook(SkeletonLayer_Btn, SelectedLayer == EIHPCAnatomyLayer::Skeleton ? SkeletonSelectedTexture : SkeletonUnselectedTexture);

	if (SkeletonBadge)
	{
		// The badge is only decoration: it must not catch the click meant for the skeleton button.
		SkeletonBadge->SetVisibility(bShowSkeletonBadge ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Collapsed);
	}
}

void UCPP_UW_IHPCLayerSwitcher::ApplyButtonLook(UButton* Button, UTexture2D* Texture) const
{
	if (!Button || !Texture)
	{
		return;   // keep whatever look the Widget Blueprint has
	}

	auto MakeBrush = [&](float Tint)
	{
		FSlateBrush Brush;
		Brush.SetResourceObject(Texture);
		Brush.ImageSize = ButtonSize;
		Brush.DrawAs = ESlateBrushDrawType::Image;
		Brush.TintColor = FSlateColor(FLinearColor(Tint, Tint, Tint, 1.f));
		return Brush;
	};

	FButtonStyle Style = Button->GetStyle();
	Style.SetNormal(MakeBrush(1.f));
	Style.SetHovered(MakeBrush(HoveredTint));
	Style.SetPressed(MakeBrush(PressedTint));
	Style.SetDisabled(MakeBrush(0.5f));
	Style.SetNormalPadding(FMargin(0.f));
	Style.SetPressedPadding(FMargin(0.f));
	Button->SetStyle(Style);
	Button->SetBackgroundColor(FLinearColor::White);
}
