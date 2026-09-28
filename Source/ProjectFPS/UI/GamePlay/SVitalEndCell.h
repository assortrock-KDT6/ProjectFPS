#pragma once

#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElementTypes.h"
#include "Rendering/SlateRenderer.h"
#include "Widgets/SLeafWidget.h"
#include "UI/GamePlay/VitalSegments.h"

// The final cell keeps the diagonal separator, but ends in a short chamfer
// and a sloped edge inset from the panel. Brushes remain the authored textures.
class SVitalEndCell final : public SLeafWidget
{
public:
	SLATE_BEGIN_ARGS(SVitalEndCell) {}
		SLATE_ARGUMENT(FVitalSegmentSettings, Settings)
		SLATE_ARGUMENT(FProgressBarStyle, BarStyle)
		SLATE_ARGUMENT(FSlateBrush, FrameBrush)
		SLATE_ARGUMENT(FLinearColor, FillTint)
	SLATE_END_ARGS()

	void Construct(const FArguments& Args)
	{
		Settings = Args._Settings;
		Style = Args._BarStyle;
		Frame = Args._FrameBrush;
		FillTint = Args._FillTint;
	}

	void SetPercent(float Value)
	{
		Percent = FMath::Clamp(Value, 0.f, 1.f);
		Invalidate(EInvalidateWidgetReason::Paint);
	}

	virtual FVector2D ComputeDesiredSize(float) const override { return Settings.CellSize; }

	virtual int32 OnPaint(const FPaintArgs&, const FGeometry& Geometry, const FSlateRect&,
		FSlateWindowElementList& Elements, int32 Layer, const FWidgetStyle& WidgetStyle,
		bool) const override
	{
        const FVector2D Size = Settings.CellSize;
        if (Size.X <= 0 || Size.Y <= 0) return Layer;
        const float Slope = FMath::Tan(FMath::DegreesToRadians(Settings.ShearAngle));
        TArray<FVector2f> Outer, Inner;
        for (const FVector2D& P : Settings.EndOuterPoints) Outer.Add(FVector2f(P * Size));
        for (const FVector2D& P : Settings.EndInnerPoints) Inner.Add(FVector2f(P * Size));
        if (Outer.Num() < 3 || Inner.Num() < 3) return Layer;
        float FillStart = TNumericLimits<float>::Max(), FillEnd = TNumericLimits<float>::Lowest();
        for (const FVector2f& P : Inner)
        {
            const float Projected = P.X - (P.Y - Size.Y * .5f) * Slope;
            FillStart = FMath::Min(FillStart, Projected);
            FillEnd = FMath::Max(FillEnd, Projected);
        }
		const FLinearColor Tint = WidgetStyle.GetColorAndOpacityTint();
		// Sample the opaque top stroke of the existing frame texture.
		DrawPolygon(Outer, Frame, true, Tint, Geometry, Elements, Layer);
		DrawPolygon(Inner, Style.BackgroundImage, false, Tint, Geometry, Elements, Layer + 1);
		if (Percent > 0.f)
		{
			// Clip the textured polygon against the moving fill edge.
			// Using the slanted local axis preserves partial-fill behavior.
			TArray<FVector2f> Filled;
			const auto Distance = [this, Size, Slope, FillStart, FillEnd](const FVector2f& P)
			{
				return P.X - (P.Y - Size.Y * .5f) * Slope - FMath::Lerp(FillStart, FillEnd, Percent);
			};
			for (int32 I = 0; I < Inner.Num(); ++I)
			{
				const FVector2f A = Inner[I];
				const FVector2f B = Inner[(I + 1) % Inner.Num()];
				const float DA = Distance(A), DB = Distance(B);
				if (DA <= 0) Filled.Add(A);
				if ((DA <= 0) != (DB <= 0)) Filled.Add(A + (B - A) * (DA / (DA - DB)));
			}
			DrawPolygon(Filled, Style.FillImage, false, Tint * FillTint, Geometry, Elements, Layer + 2);
		}
		return Layer + 2;
	}

private:
	void DrawPolygon(const TArray<FVector2f>& Points, const FSlateBrush& Brush,
		bool FrameSample, const FLinearColor& Tint, const FGeometry& Geometry,
		FSlateWindowElementList& Elements, int32 Layer) const
	{
		if (Points.Num() < 3) return;
		TArray<FSlateVertex> Vertices;
		TArray<SlateIndex> Indices;
		const FColor Color = (Tint * Brush.TintColor.GetSpecifiedColor()).ToFColor(true);
        FVector2D UVMin(TNumericLimits<double>::Max(), TNumericLimits<double>::Max());
        FVector2D UVMax(TNumericLimits<double>::Lowest(), TNumericLimits<double>::Lowest());
        for (const FVector2D& P : Settings.EndOuterPoints)
        {
            UVMin.X = FMath::Min(UVMin.X, P.X * Settings.CellSize.X);
            UVMin.Y = FMath::Min(UVMin.Y, P.Y * Settings.CellSize.Y);
            UVMax.X = FMath::Max(UVMax.X, P.X * Settings.CellSize.X);
            UVMax.Y = FMath::Max(UVMax.Y, P.Y * Settings.CellSize.Y);
        }
        if (UVMax.X <= UVMin.X || UVMax.Y <= UVMin.Y) return;
		for (const FVector2f& P : Points)
		{
			const FVector2f UV = FrameSample ? FVector2f(Settings.FrameSampleUV)
				: FVector2f((P.X - UVMin.X) / (UVMax.X - UVMin.X), (P.Y - UVMin.Y) / (UVMax.Y - UVMin.Y));
			Vertices.Add(FSlateVertex::Make<ESlateVertexRounding::Disabled>(Geometry.GetAccumulatedRenderTransform(), P, UV, Color));
		}
		for (int32 I = 1; I + 1 < Points.Num(); ++I)
		{
			Indices.Add(0); Indices.Add(I); Indices.Add(I + 1);
		}
		FSlateDrawElement::MakeCustomVerts(Elements, Layer,
			FSlateApplication::Get().GetRenderer()->GetResourceHandle(Brush), Vertices, Indices, nullptr, 0, 0);
	}

	FVitalSegmentSettings Settings;
	FProgressBarStyle Style;
	FSlateBrush Frame;
	FLinearColor FillTint = FLinearColor::White;
	float Percent = 0.f;
};
