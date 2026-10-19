#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "HitDirectionWidget.generated.h"

class UImage;

/** 총알이 들어온 방향만 표시한다. 테두리 위젯과 독립적으로 생성하고 정리한다. */
UCLASS()
class PROJECTFPS_API UHitDirectionWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowIncomingHit(APawn* Victim, const FVector& IncomingWorldDirection, float Damage, float SourceDistance = -1.f);
	void ClearFeedback();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// BP에서 네 이미지를 화면 중앙에 같은 크기로 겹쳐 배치한다.
	// 각 브러시는 위쪽을 정면으로 하는 텍스처를 사용하며 텍스처와 크기는 BP에서 지정한다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> _DirectionImage0;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> _DirectionImage1;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> _DirectionImage2;

	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> _DirectionImage3;

	UPROPERTY(EditDefaultsOnly, Category = "Hit Direction", meta = (ClampMin = "0.1"))
	float _DirectionDuration = 0.85f;

	// 공격자와 피해자 사이의 거리가 이 값 이상이면 최소 불투명도로 표시한다. 단위는 cm이다.
	UPROPERTY(EditDefaultsOnly, Category = "Hit Direction|Distance", meta = (ClampMin = "0.0", Units = "cm"))
	float _DistanceThreshold = 5000.f;

	// 원거리 피격 표시의 최대 불투명도. 이후에는 기존 페이드에 따라 0까지 사라진다.
	UPROPERTY(EditDefaultsOnly, Category = "Hit Direction|Distance", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float _MinDirectionOpacity = 0.15f;

private:
	struct FHitPulse
	{
		FVector _Direction = FVector::ZeroVector;
		float _Age = 0.f;
		float _Strength = 0.f;
	};

	bool CalculateAngle(const FVector& Direction, float CameraYawDegrees, float& OutAngle) const;
	float CalculateOpacity(float Age) const;

	static constexpr int32 _MaxDirections = 4;
	FHitPulse _Hits[_MaxDirections];
	TWeakObjectPtr<APawn> _Victim;

	UPROPERTY(Transient)
	TArray<TObjectPtr<UImage>> _DirectionImages;
};
