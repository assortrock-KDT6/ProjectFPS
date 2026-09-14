// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "ItemInfoWidget.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTFPS_API UItemInfoWidget : public UUserWidget
{
	GENERATED_BODY()
	
	// meta = (BindWidget) -> 위젯BP에서 이름이 같은 위젯을 찾아 연결.
	
	// 아이템 아이콘
	UPROPERTY(meta = (BindWidget))			
	TObjectPtr<class UImage> _IconImage;

	// 아이템 이름
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> _NameText;

	// 아이템 설명
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> _DescText;

	// 툴팁 능력치
	UPROPERTY(meta = (BindWidget))
	TObjectPtr<class UTextBlock> _AbilityText;


	// 커서에서 띄우는 오프셋.
	UPROPERTY(EditAnywhere, Category = "Info")
	FVector2D _CursorOffset = FVector2D(16.f, 16.f); 
private:
	// 무기 테이블 -> 능력치 테이블 순으로 조회해서 채움.
	void SetWeaponAbility(FName TID);
	void HideAbility();


protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

public:
	// TID로 테이블 조회해서 표시.
	void SetInfoByTID(FName TID);

	// 숨기기
	void HideInfo();


};
