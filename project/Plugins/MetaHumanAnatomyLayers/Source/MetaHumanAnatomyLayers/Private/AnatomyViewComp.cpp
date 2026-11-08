#include "AnatomyViewComp.h"

#include "MetaHumanAnatomyLayers.h"
#include "Components/MeshComponent.h"
#include "Components/SkinnedMeshComponent.h"
#include "GameFramework/Actor.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

namespace AnatomyView
{
	// Grooms live in the HairStrands plugin; match by class name so this module doesn't depend on it.
	static bool IsGroom(const UMeshComponent* Mesh)
	{
		for (const UClass* Class = Mesh ? Mesh->GetClass() : nullptr; Class; Class = Class->GetSuperClass())
		{
			if (Class->GetFName() == TEXT("GroomComponent"))
			{
				return true;
			}
		}
		return false;
	}

	static constexpr float Opaque = 0.999f;
	static constexpr float Gone = 0.001f;
}

UAnatomyViewComp::UAnatomyViewComp()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = false; // only ticks while animating between layers

	HiddenMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/MetaHumanAnatomyLayers/Anatomy/Materials/M_Anatomy_Hidden.M_Anatomy_Hidden")));

	FAnatomyLayerDef& Skin = Layers.Emplace_GetRef(TEXT("Skin"), TArray<FName>{ TEXT("SkinPart") });
	Skin.GhostMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/MetaHumanAnatomyLayers/Anatomy/Materials/M_Anatomy_XRay.M_Anatomy_XRay")));
	Skin.GhostBelowOpacity = 1.f; // straight to the x-ray shell as soon as the slider leaves the top: no skin/muscle mix
	Skin.FadeLength = 0.5f;       // skin -> muscles takes the first third of the slider, muscles -> skeleton the rest
	FAnatomyLayerDef& Muscles = Layers.Emplace_GetRef(TEXT("Muscles"), TArray<FName>{ TEXT("MuscleMesh"), TEXT("MusclePart") });
	Muscles.FadeMaterial = TSoftObjectPtr<UMaterialInterface>(FSoftObjectPath(TEXT("/MetaHumanAnatomyLayers/Anatomy/Materials/M_Anatomy_MuscleFade.M_Anatomy_MuscleFade")));
	Layers.Emplace(TEXT("Skeleton"), TArray<FName>{ TEXT("SkeletonMesh"), TEXT("SkeletonPart") });
}

void UAnatomyViewComp::BeginPlay()
{
	Super::BeginPlay();

	LoadedHiddenMaterial = HiddenMaterial.LoadSynchronous();
	BuildRuntime();

	CurrentDepth = TargetDepth = FMath::Clamp(InitialRevealDepth, 0.f, 1.f);
	ApplyDepth(CurrentDepth);
}

void UAnatomyViewComp::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// Removing just this component at runtime should leave the character as it found it.
	if (EndPlayReason == EEndPlayReason::Destroyed && GetOwner() && !GetOwner()->IsActorBeingDestroyed())
	{
		RestoreAll();
	}
	Super::EndPlay(EndPlayReason);
}

int32 UAnatomyViewComp::FindLayerFor(const UMeshComponent* Mesh) const
{
	for (int32 LayerIndex = 0; LayerIndex < Layers.Num(); ++LayerIndex)
	{
		for (const FName& Key : Layers[LayerIndex].ComponentNamesOrTags)
		{
			if (Mesh->GetFName() == Key || Mesh->ComponentHasTag(Key))
			{
				return LayerIndex;
			}
		}
	}
	return INDEX_NONE;
}

void UAnatomyViewComp::BuildRuntime()
{
	Runtime.Reset();
	Runtime.SetNum(Layers.Num());
	for (int32 LayerIndex = 0; LayerIndex < Layers.Num(); ++LayerIndex)
	{
		Runtime[LayerIndex].LayerName = Layers[LayerIndex].LayerName;
		Runtime[LayerIndex].Ghost = Layers[LayerIndex].GhostMaterial.LoadSynchronous();
		Runtime[LayerIndex].GhostBelowOpacity = Layers[LayerIndex].GhostBelowOpacity;
		Runtime[LayerIndex].FadeMaterial = Layers[LayerIndex].FadeMaterial.LoadSynchronous();
	}

	AActor* Owner = GetOwner();
	if (!Owner || Layers.Num() == 0)
	{
		bBuilt = true;
		return;
	}

	TArray<UMeshComponent*> Meshes;
	Owner->GetComponents<UMeshComponent>(Meshes);

	for (UMeshComponent* Mesh : Meshes)
	{
		if (!Mesh || Mesh->ComponentHasTag(IgnoreTag))
		{
			continue;
		}

		int32 LayerIndex = FindLayerFor(Mesh);
		if (LayerIndex == INDEX_NONE)
		{
			// Unclaimed meshes are the outer "skin" layer, but only ones that are actually shown in game
			// (skips editor-only helpers and parts the designer hid on purpose).
			if (!Mesh->IsVisible() || Mesh->bHiddenInGame)
			{
				continue;
			}
			LayerIndex = 0;
		}

		FAnatomyFadeTarget Target;
		Target.Mesh = Mesh;
		if (const USkinnedMeshComponent* Skinned = Cast<USkinnedMeshComponent>(Mesh))
		{
			Target.OriginalTickOption = Skinned->VisibilityBasedAnimTickOption;
		}

		const int32 NumMaterials = Mesh->GetNumMaterials();
		Target.Slots.SetNum(NumMaterials);
		for (int32 SlotIndex = 0; SlotIndex < NumMaterials; ++SlotIndex)
		{
			FAnatomyFadeSlot& Slot = Target.Slots[SlotIndex];
			Slot.Original = Mesh->GetMaterial(SlotIndex);
			if (!Slot.Original)
			{
				continue;
			}

			if (UMaterialInterface* LayerFade = Runtime[LayerIndex].FadeMaterial)
			{
				// The layer brings its own fade material (the mesh's material can't fade itself).
				Slot.Fader = UMaterialInstanceDynamic::Create(LayerFade, Mesh);
				Slot.Parameter = FadeParameterName;
				Slot.FullValue = 1.f;
				continue;
			}

			float Value = 0.f;
			// "Fade" counts only when it's the visibility fade (authored at 1). MetaHuman facial-hair cards use a
			// "Fade" that defaults to 0 for something else, so those fall through to Opacity / hidden.
			if (Slot.Original->GetScalarParameterValue(FHashedMaterialParameterInfo(FadeParameterName), Value) && Value > 0.5f)
			{
				Slot.Parameter = FadeParameterName;
				Slot.FullValue = 1.f;
			}
			else if (Slot.Original->GetScalarParameterValue(FHashedMaterialParameterInfo(OpacityParameterName), Value))
			{
				Slot.Parameter = OpacityParameterName;
				Slot.FullValue = Value;
			}

			if (!Slot.Parameter.IsNone())
			{
				Slot.Fader = UMaterialInstanceDynamic::Create(Slot.Original, Mesh);
			}
		}

		Runtime[LayerIndex].Targets.Add(MoveTemp(Target));
	}

	for (const FAnatomyLayerRuntime& Layer : Runtime)
	{
		FString Names;
		for (const FAnatomyFadeTarget& Target : Layer.Targets)
		{
			Names += Target.Mesh.IsValid() ? Target.Mesh->GetName() + TEXT(" ") : FString();
		}
		UE_LOG(LogAnatomyLayers, Log, TEXT("[%s] layer %s: %s"), *GetNameSafe(Owner), *Layer.LayerName.ToString(), *Names);
	}

	bBuilt = true;
}

void UAnatomyViewComp::RestoreAll()
{
	for (FAnatomyLayerRuntime& Layer : Runtime)
	{
		for (FAnatomyFadeTarget& Target : Layer.Targets)
		{
			Target.bInitialized = false;
			ApplyOpacity(Target, 1.f);
		}
	}
}

void UAnatomyViewComp::RefreshLayers()
{
	RestoreAll();
	BuildRuntime();
	ApplyDepth(CurrentDepth);
}

void UAnatomyViewComp::GetLayerSegment(int32 LayerIndex, float& OutStart, float& OutLength) const
{
	// Every layer but the innermost owns a stretch of the slider, sized by its FadeLength.
	float Total = 0.f;
	for (int32 Index = 0; Index < Layers.Num() - 1; ++Index)
	{
		Total += FMath::Max(Layers[Index].FadeLength, 0.01f);
	}
	OutStart = 0.f;
	OutLength = 0.f;
	if (Total <= 0.f)
	{
		return;
	}
	for (int32 Index = 0; Index < LayerIndex && Index < Layers.Num() - 1; ++Index)
	{
		OutStart += FMath::Max(Layers[Index].FadeLength, 0.01f) / Total;
	}
	if (LayerIndex < Layers.Num() - 1)
	{
		OutLength = FMath::Max(Layers[LayerIndex].FadeLength, 0.01f) / Total;
	}
	OutStart = FMath::Clamp(OutStart, 0.f, 1.f);
}

float UAnatomyViewComp::GetLayerDepth(int32 LayerIndex) const
{
	if (Layers.Num() <= 1)
	{
		return 0.f;
	}
	float Start = 0.f, Length = 0.f;
	GetLayerSegment(FMath::Clamp(LayerIndex, 0, Layers.Num() - 1), Start, Length);
	return Start;
}

float UAnatomyViewComp::GetLayerOpacity(int32 LayerIndex) const
{
	return Runtime.IsValidIndex(LayerIndex) ? Runtime[LayerIndex].Opacity : 0.f;
}

float UAnatomyViewComp::ComputeLayerOpacity(int32 LayerIndex, float Depth) const
{
	const int32 Num = Layers.Num();
	if (Num <= 1)
	{
		return 1.f - Depth;
	}

	// Layer k fades out across its own stretch of the slider; the innermost layer never fades out.
	float Start = 0.f, Length = 0.f;
	GetLayerSegment(LayerIndex, Start, Length);
	const float FadeOut = (LayerIndex == Num - 1 || Length <= 0.f)
		? 1.f
		: 1.f - FMath::Clamp((Depth - Start) / Length, 0.f, 1.f);

	// Inner layers appear while the outermost one starts to go (InnerLayerFadeIn = 0: as soon as the slider moves).
	float FadeIn = 1.f;
	if (LayerIndex > 0)
	{
		float FirstStart = 0.f, FirstLength = 0.f;
		GetLayerSegment(0, FirstStart, FirstLength);
		FadeIn = InnerLayerFadeIn <= 0.f
			? (Depth > KINDA_SMALL_NUMBER ? 1.f : 0.f)
			: FMath::Clamp(Depth / (InnerLayerFadeIn * FirstLength), 0.f, 1.f);
	}

	return FadeOut * FadeIn;
}

void UAnatomyViewComp::ApplyDepth(float Depth)
{
	for (int32 LayerIndex = 0; LayerIndex < Runtime.Num(); ++LayerIndex)
	{
		FAnatomyLayerRuntime& Layer = Runtime[LayerIndex];
		Layer.Opacity = ComputeLayerOpacity(LayerIndex, Depth);
		for (FAnatomyFadeTarget& Target : Layer.Targets)
		{
			ApplyOpacity(Target, Layer.Opacity, Layer.Ghost, Layer.GhostBelowOpacity);
		}
	}
	OnRevealDepthChanged.Broadcast(Depth);
}

void UAnatomyViewComp::SetMaterials(FAnatomyFadeTarget& Target, FAnatomyFadeTarget::EMaterials Materials, UMaterialInterface* Ghost)
{
	UMeshComponent* Mesh = Target.Mesh.Get();
	if (!Mesh || (Target.bInitialized && Target.Materials == Materials))
	{
		return;
	}

	for (int32 SlotIndex = 0; SlotIndex < Target.Slots.Num(); ++SlotIndex)
	{
		const FAnatomyFadeSlot& Slot = Target.Slots[SlotIndex];
		UMaterialInterface* Material = Slot.Original;
		if (Materials == FAnatomyFadeTarget::EMaterials::Ghost)
		{
			Material = Ghost;
		}
		else if (Materials == FAnatomyFadeTarget::EMaterials::Faders)
		{
			// Slots without a fade parameter (e.g. the eye occlusion shell) are hidden while fading.
			Material = Slot.Fader ? Slot.Fader.Get() : (LoadedHiddenMaterial ? LoadedHiddenMaterial.Get() : Slot.Original.Get());
		}
		Mesh->SetMaterial(SlotIndex, Material);
	}
	Target.Materials = Materials;
}

void UAnatomyViewComp::ApplyOpacity(FAnatomyFadeTarget& Target, float Opacity, UMaterialInterface* Ghost, float GhostBelowOpacity)
{
	UMeshComponent* Mesh = Target.Mesh.Get();
	if (!Mesh)
	{
		return;
	}
	if (Target.bInitialized && FMath::IsNearlyEqual(Opacity, Target.AppliedOpacity, 0.0005f))
	{
		return;
	}

	const bool bGroom = AnatomyView::IsGroom(Mesh);
	const bool bGhost = Ghost && GhostBelowOpacity > 0.f && Opacity < AnatomyView::Opaque && Opacity <= GhostBelowOpacity;

	// Hair strands don't dither well and have no shell look: they go away early.
	const bool bHide = bGroom
		? (bGhost || Opacity < HideGroomsBelowOpacity)
		: (!bGhost && Opacity <= AnatomyView::Gone);

	USkinnedMeshComponent* Skinned = Cast<USkinnedMeshComponent>(Mesh);
	if (bHide)
	{
		if (!Target.bHidden || !Target.bInitialized)
		{
			// Hidden meshes normally stop animating; keep the pose ticking so Leader Pose followers
			// (the muscle / skeleton layers follow Body) keep moving.
			if (Skinned)
			{
				Skinned->VisibilityBasedAnimTickOption = EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
			}
			Mesh->SetVisibility(false);
			Target.bHidden = true;
		}
	}
	else
	{
		if (Target.bHidden || !Target.bInitialized || !Mesh->IsVisible())
		{
			Mesh->SetVisibility(true);
			if (Skinned)
			{
				Skinned->VisibilityBasedAnimTickOption = Target.OriginalTickOption;
			}
			Target.bHidden = false;
		}

		if (bGhost)
		{
			SetMaterials(Target, FAnatomyFadeTarget::EMaterials::Ghost, Ghost);
		}
		else if (Opacity >= AnatomyView::Opaque)
		{
			SetMaterials(Target, FAnatomyFadeTarget::EMaterials::Original, nullptr);
		}
		else
		{
			SetMaterials(Target, FAnatomyFadeTarget::EMaterials::Faders, nullptr);
			for (const FAnatomyFadeSlot& Slot : Target.Slots)
			{
				if (Slot.Fader)
				{
					Slot.Fader->SetScalarParameterValue(Slot.Parameter, Slot.FullValue * Opacity);
				}
			}
		}
	}

	Target.AppliedOpacity = Opacity;
	Target.bInitialized = true;
}

void UAnatomyViewComp::SetRevealDepth(float Depth, bool bAnimate)
{
	TargetDepth = FMath::Clamp(Depth, 0.f, 1.f);

	if (!bBuilt)
	{
		InitialRevealDepth = TargetDepth; // BeginPlay hasn't run yet
		return;
	}

	if (bAnimate)
	{
		SetComponentTickEnabled(true);
		return;
	}

	SetComponentTickEnabled(false);
	if (!FMath::IsNearlyEqual(CurrentDepth, TargetDepth))
	{
		CurrentDepth = TargetDepth;
		ApplyDepth(CurrentDepth);
	}
}

void UAnatomyViewComp::ShowLayer(int32 LayerIndex)
{
	if (Layers.IsValidIndex(LayerIndex))
	{
		SetRevealDepth(GetLayerDepth(LayerIndex), /*bAnimate*/ true);
	}
}

void UAnatomyViewComp::ShowLayerByName(FName LayerName)
{
	ShowLayer(Layers.IndexOfByPredicate([LayerName](const FAnatomyLayerDef& Def) { return Def.LayerName == LayerName; }));
}

void UAnatomyViewComp::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CurrentDepth = FMath::FInterpConstantTo(CurrentDepth, TargetDepth, DeltaTime, TransitionSpeed);
	ApplyDepth(CurrentDepth);

	if (FMath::IsNearlyEqual(CurrentDepth, TargetDepth, KINDA_SMALL_NUMBER))
	{
		SetComponentTickEnabled(false);
	}
}
