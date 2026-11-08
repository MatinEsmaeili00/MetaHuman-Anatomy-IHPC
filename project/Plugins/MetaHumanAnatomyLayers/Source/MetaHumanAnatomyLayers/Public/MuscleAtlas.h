// Muscle atlas: names, info text and picking samples for the muscles of an anatomy mesh.
// The anatomy mesh stores each vertex's muscle ID in its vertex colour (red = id / 255); the samples are points on
// each muscle, stored relative to a bone, so they can be placed on the animated skeleton for hover / click picking.

#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "MuscleAtlas.generated.h"

USTRUCT(BlueprintType)
struct METAHUMANANATOMYLAYERS_API FMuscleSample
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Muscle")
	FName Bone;

	/** Position in the bone's space (reference pose). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Muscle")
	FVector Offset = FVector::ZeroVector;
};

USTRUCT(BlueprintType)
struct METAHUMANANATOMYLAYERS_API FMuscleInfo
{
	GENERATED_BODY()

	/** Matches the muscle ID in the mesh's vertex colours (1..255; 0 = no muscle). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Muscle")
	int32 Id = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Muscle")
	FName Key;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Muscle")
	FText Name;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Muscle")
	FText Side;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Muscle")
	FText Region;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Muscle", meta=(MultiLine=true))
	FText Function;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Muscle", meta=(MultiLine=true))
	FText Running;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Muscle", meta=(MultiLine=true))
	FText Injury;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Muscle")
	TArray<FMuscleSample> Samples;
};

UCLASS(BlueprintType)
class METAHUMANANATOMYLAYERS_API UMuscleAtlas : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Muscle")
	TArray<FMuscleInfo> Muscles;

	const FMuscleInfo* FindById(int32 Id) const
	{
		return Muscles.FindByPredicate([Id](const FMuscleInfo& Info) { return Info.Id == Id; });
	}
};
