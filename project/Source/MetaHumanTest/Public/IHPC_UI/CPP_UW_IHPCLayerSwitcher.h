// CPP_UW_IHPCLayerSwitcher.h
// -----------------------------------------------------------------------------
// The LAYER SWITCHER at the bottom right of the screen.
// Figma (frame "iPad Pro 12.9" - 1"):
//   * "ihpc ui rttd 2" - three glossy round buttons: Body (layers icon), Muscles, Skeleton.
//                        Selected = dark navy circle + white icon, unselected = light blue + navy icon.
//                        The skeleton button has a small orange badge (hidden by default, team decision).
//   * "Rectangle 24"   - the see-through pill behind them (#1D3C55 at 26 %, 204 x 65, radius 35.5).
//
// What the buttons do (merged with the Abby anatomy branch):
//   Each button shows one ANATOMY LAYER of the possessed pawn through its UAnatomyViewComp ("AnatomyView"
//   component, plugin MetaHumanAnatomyLayers):
//     Body     -> layer "Skin"     (the whole MetaHuman: body, face, hair, clothes)
//     Muscles  -> layer "Muscles"  (segmented muscle mesh, skin shown as an x-ray shell)
//     Skeleton -> layer "Skeleton" (segmented skeleton mesh)
//   Each button animates the AnatomyView to a fixed "skin amount" (same scale as the old anatomy slider,
//   1 = top / all skin): Body = 1.0, Muscles = 0.5, Skeleton = 0.0 (BodySkinAmount / MusclesSkinAmount /
//   SkeletonSkinAmount; reveal depth = 1 - amount). The pawns' AnatomyView uses equal fade lengths for Skin and
//   Muscles, so 0.5 is exactly "muscles fully shown, skin as an x-ray shell". Untick bUseSkinAmounts to jump to the
//   named layers' own depths instead. There is no opacity slider: the parent class's slider is kept only because
//   the parent binds it, and stays Collapsed.
//
//   The parent class (UCPP_UW_AnatomyViewSwitcher) finds the AnatomyView component on the possessed pawn and
//   re-binds when the controller possesses another pawn (Abby -> Beau -> ...). A pawn without the component
//   greys the buttons out.
//
// Widget Blueprint: /Game/UserInterface/IHPC/Widgets/WBP_IHPC_LayerSwitcher
//   Needs Buttons "BodyLayer_Btn", "MusclesLayer_Btn", "SkeletonLayer_Btn", optional Image "SkeletonBadge",
//   and (for the parent class) a Slider "AnatomyViewSwitcher_Slider" (Collapsed); "AnatomyMetaHuman_Btn" is optional.
// Created by CPP_PlayerController (BP_PlayerController -> Anatomy View Switcher Widget Class).
// -----------------------------------------------------------------------------

#pragma once

#include "CoreMinimal.h"
#include "CPP_UW_AnatomyViewSwitcher.h"
#include "IHPC_UI/IHPCTypes.h"
#include "CPP_UW_IHPCLayerSwitcher.generated.h"

class UButton;
class UImage;
class UTexture2D;
class UAnatomyViewComp;

/** Fired when a layer button is clicked. RevealDepth = where the anatomy is going (0 = skin ... 1 = skeleton),
 *  or -1 if the pawn has no anatomy layers. */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FIHPCLayerChanged, EIHPCAnatomyLayer, Layer, float, RevealDepth);

UCLASS(Abstract)
class METAHUMANTEST_API UCPP_UW_IHPCLayerSwitcher : public UCPP_UW_AnatomyViewSwitcher
{
	GENERATED_BODY()

public:
	// ---- Layers: which anatomy layer each button shows --------------------------------------------

	/** Button that looks selected when the game starts (Figma frame 1: Body). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IHPC|Layers")
	EIHPCAnatomyLayer DefaultLayer = EIHPCAnatomyLayer::Body;

	/** Also SHOW the default layer when the game starts. Off = the avatar starts as the AnatomyView component's
	 *  Initial Reveal Depth says (skin by default). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "IHPC|Layers")
	bool bApplyDefaultLayerOnStart = false;

	/** UAnatomyViewComp layer shown by the Body button. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Layers")
	FName BodyLayerName = TEXT("Skin");

	/** UAnatomyViewComp layer shown by the Muscles button. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Layers")
	FName MusclesLayerName = TEXT("Muscles");

	/** UAnatomyViewComp layer shown by the Skeleton button. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Layers")
	FName SkeletonLayerName = TEXT("Skeleton");

	/** Use the fixed "skin amounts" below (true, default) instead of jumping to the named layers' own depths. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Layers")
	bool bUseSkinAmounts = true;

	/** How much of the outside each button shows, on the old anatomy slider's scale (1 = all skin, 0 = only bones):
	 *  the button animates the AnatomyView to reveal depth 1 - amount. Left button: all skin. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Layers", meta = (ClampMin = "0", ClampMax = "1", EditCondition = "bUseSkinAmounts"))
	float BodySkinAmount = 1.f;

	/** Middle button: the muscles (skin turned into the x-ray shell). With the AnatomyView's default layers
	 *  (Skin / Muscles / Skeleton, equal fade lengths) depth 0.5 is exactly where the muscles are fully shown. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Layers", meta = (ClampMin = "0", ClampMax = "1", EditCondition = "bUseSkinAmounts"))
	float MusclesSkinAmount = 0.5f;

	/** Right button: the bones. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Layers", meta = (ClampMin = "0", ClampMax = "1", EditCondition = "bUseSkinAmounts"))
	float SkeletonSkinAmount = 0.f;

	/** Reveal depth (0 = skin ... 1 = skeleton) a button animates to on the possessed pawn, or -1 without anatomy layers. */
	UFUNCTION(BlueprintPure, Category = "IHPC|Layers")
	float GetRevealDepthForLayer(EIHPCAnatomyLayer Layer) const;

	// ---- Look (textures made from the Figma pictures by Tools/FigmaUI/make_ui_textures.py) ----------

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	TObjectPtr<UTexture2D> BodySelectedTexture;          // T_IHPC_LS_Body_On

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	TObjectPtr<UTexture2D> BodyUnselectedTexture;        // T_IHPC_LS_Body_Off

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	TObjectPtr<UTexture2D> MusclesSelectedTexture;       // T_IHPC_LS_Muscles_On

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	TObjectPtr<UTexture2D> MusclesUnselectedTexture;     // T_IHPC_LS_Muscles_Off

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	TObjectPtr<UTexture2D> SkeletonSelectedTexture;      // T_IHPC_LS_Skeleton_On (not drawn in Figma, rebuilt)

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	TObjectPtr<UTexture2D> SkeletonUnselectedTexture;    // T_IHPC_LS_Skeleton_Off

	/** Size each round button is drawn at (Figma: 46 x 46). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	FVector2D ButtonSize = FVector2D(46.f, 46.f);

	/** Brightness while the mouse is over a button / while it is pressed (same as the timeline buttons). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	float HoveredTint = 1.15f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	float PressedTint = 0.8f;

	/** Show the small orange badge on the skeleton button (Figma shows it). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "IHPC|Style")
	bool bShowSkeletonBadge = true;

	// ---- Events / calls --------------------------------------------------------------------------

	/** Broadcast every time a layer button is clicked (also when the pawn has no anatomy layers). */
	UPROPERTY(BlueprintAssignable, Category = "IHPC|Events")
	FIHPCLayerChanged OnLayerChanged;

	/** Same as clicking a button: highlights it and shows that anatomy layer on the possessed pawn. */
	UFUNCTION(BlueprintCallable, Category = "IHPC|Layers")
	void SelectLayer(EIHPCAnatomyLayer Layer);

	UFUNCTION(BlueprintPure, Category = "IHPC|Layers")
	EIHPCAnatomyLayer GetSelectedLayer() const { return SelectedLayer; }

	/** Index of the AnatomyView layer a button shows (by name, falling back to outermost / middle / innermost),
	 *  or INDEX_NONE without an AnatomyView component. */
	UFUNCTION(BlueprintPure, Category = "IHPC|Layers")
	int32 GetAnatomyLayerIndex(EIHPCAnatomyLayer Layer) const;

	UFUNCTION(BlueprintCallable, Category = "IHPC|Layers")
	void SetSkeletonBadgeVisible(bool bVisible);

	/** True when the possessed pawn has an AnatomyView component with at least one layer. */
	UFUNCTION(BlueprintPure, Category = "IHPC|Layers")
	bool CanDriveAvatarLayers() const;

protected:
	virtual void NativePreConstruct() override;   // runs in the UMG designer too, so the buttons show their look
	virtual void NativeConstruct() override;      // in game: parent binds to the pawn's AnatomyView, we bind the buttons
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> BodyLayer_Btn;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> MusclesLayer_Btn;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UButton> SkeletonLayer_Btn;

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UImage> SkeletonBadge;

private:
	UFUNCTION()
	void HandleBodyClicked();

	UFUNCTION()
	void HandleMusclesClicked();

	UFUNCTION()
	void HandleSkeletonClicked();

	/** Highlight the button that matches the layer the (new) pawn is showing, without changing the pawn. */
	void SyncSelectionFromPawn();

	FName GetLayerName(EIHPCAnatomyLayer Layer) const;

	/** Gives every button its selected / unselected picture and shows or hides the badge. */
	void RefreshVisuals();

	void ApplyButtonLook(UButton* Button, UTexture2D* Texture) const;

	EIHPCAnatomyLayer SelectedLayer = EIHPCAnatomyLayer::Body;

	/** The AnatomyView the buttons were last synced to (changes when the controller possesses another pawn). */
	TWeakObjectPtr<UAnatomyViewComp> SyncedAnatomyComp;

	bool bWarnedMissingLayers = false;
};
