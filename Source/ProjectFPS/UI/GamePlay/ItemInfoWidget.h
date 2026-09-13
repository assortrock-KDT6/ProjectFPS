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

protected:
	virtual void NativeConstruct() override;

public:
	// TID로 테이블 조회해서 표시.
	void SetInfoByTID(FName TID);

	// 숨기기
	void HideInfo();


};
