// 프로젝트 설정의 설명 페이지에 저작권 정보를 입력하세요.

#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DefaultInput.generated.h"

/**
 * 
 */
UCLASS()
class PROJECTFPS_API UDefaultInput : public UObject
{
	GENERATED_BODY()
	friend class ACharacterPlayer;
public:
	UDefaultInput();
protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UInputMappingContext> _DefaultInputMappingContext;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UInputAction> _Move;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UInputAction> _Jump;
	
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UInputAction> _MouseLook;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UInputAction> _AimZoom;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	TObjectPtr<class UInputAction> _Parkour;

	UPROPERTY(EditAnywhere)
	TObjectPtr<class UInputAction> _Inventory;

	UPROPERTY(EditAnywhere)
	TObjectPtr<class UInputAction> _Map;

	UPROPERTY(EditAnywhere)
	TObjectPtr<class UInputAction> _Interact;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<class UInputAction> _Fire;

	// 무기 장착 여부와 관계없이 근접 공격을 요청하는 전용 입력이다.
	UPROPERTY(EditAnywhere)
	TObjectPtr<class UInputAction> _Melee;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<class UInputAction> _FireToggle;

	UPROPERTY(EditAnywhere)
	TObjectPtr<class UInputAction> _Cook;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<class UInputAction> _DropItem;
	
	UPROPERTY(EditAnywhere)
	TObjectPtr<class UInputAction> _WeaponSlot3;

	// 무기 슬롯 교체 (Main, Sub)
	UPROPERTY(EditAnywhere)
	TObjectPtr<class UInputAction> _EquipMainWeapon;

	// 무기 슬롯 교체 (Main, Sub)
	UPROPERTY(EditAnywhere)
	TObjectPtr<class UInputAction> _EquipSubWeapon;

	UPROPERTY(EditAnywhere)
	TObjectPtr<class UInputAction> _Reload;
};
