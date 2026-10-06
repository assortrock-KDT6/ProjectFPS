#pragma once

#include "CoreMinimal.h"
#include "VitalSegments.generated.h"

/** All visual values are authored in widget Blueprint defaults. End points use normalized cell coordinates. */
USTRUCT(BlueprintType)
struct FVitalSegmentSettings
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vital")
    float UnitsPerCell = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vital")
    FVector2D CellSize = FVector2D::ZeroVector;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vital")
    float CellGap = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vital")
    float ShearAngle = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vital")
    float FillPadding = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vital")
    float PanelExtraWidth = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vital")
    float PanelHeight = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vital")
    TArray<FVector2D> EndOuterPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vital")
    TArray<FVector2D> EndInnerPoints;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Vital")
    FVector2D FrameSampleUV = FVector2D::ZeroVector;

    int32 Count(float Maximum) const
    {
        return FMath::IsFinite(Maximum) && Maximum > 0.f && FMath::IsFinite(UnitsPerCell) && UnitsPerCell > 0.f
            ? FMath::CeilToInt(Maximum / UnitsPerCell) : 0;
    }

    float Fill(float Current, float Maximum, int32 Index) const
    {
        if (!FMath::IsFinite(Current) || !FMath::IsFinite(Maximum) || Maximum <= 0.f || Index < 0
            || !FMath::IsFinite(UnitsPerCell) || UnitsPerCell <= 0.f) return 0.f;
        return FMath::Clamp((FMath::Clamp(Current, 0.f, Maximum) - Index * UnitsPerCell) / UnitsPerCell, 0.f, 1.f);
    }
};
