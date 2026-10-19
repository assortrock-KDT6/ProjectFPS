#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "DamageFeedbackWidget.generated.h"

class UImage;

/** 피해 종류와 관계없이 피해자 화면의 테두리만 표시한다. */
UCLASS()
class PROJECTFPS_API UDamageFeedbackWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	void ShowIncomingHit(APawn* Victim, float Damage);
	void ClearFeedback();

protected:
	virtual void NativeOnInitialized() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	// BP 디자이너에서 전체 화면 크기로 배치하고 브러시의 텍스처를 지정한다.
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<UImage> _VignetteImage;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Feedback", meta = (ClampMin = "0.1"))
	float _VignetteDuration = 0.65f;

	UPROPERTY(EditDefaultsOnly, Category = "Damage Feedback", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float _VignetteMaxOpacity = 0.22f;

private:
	float CalculateOpacity() const;

	float _VignetteAge = 0.f;
	float _VignetteStrength = 0.f;
	TWeakObjectPtr<APawn> _Victim;
};
