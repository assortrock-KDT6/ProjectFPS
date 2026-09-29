// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Components/SceneCaptureComponent2D.h"
#include "MinimapCaptureComponent.generated.h"



class UTextureRenderTarget2D;
class AController;


/**
 * 
 * 
 * 
 */

UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))

class PROJECTFPS_API UMinimapCaptureComponent : public USceneCaptureComponent2D
{
	GENERATED_BODY()
public:
	UMinimapCaptureComponent();

protected:
	// 캐릭터 머리 위 얼마나 높이에서 내려다볼지
	UPROPERTY(EditAnywhere, Category = "Minimap")
	float _Height = 3000.f;

	//미니맵이 덮는 범위 작을 수록 확대된다.
	UPROPERTY(EditAnywhere, Category = "Minimap", meta = (ClampMin = "100.0"))
	float _ViewRange = 4000.f;

	// 렌더 타깃 해상도
	UPROPERTY(EditAnywhere, Category = "Minimap", meta = (ClampMin = "64" ))
	int32 _Resolution = 512;

	// 플레이어 시선 방향으로 지도를 돌릴지, 끌 경우 북쪽으로 고정시킴.
	UPROPERTY(EditAnywhere, Category = "Minimap")
	bool _bRotateWithPlayer = true;

	// 자기 자신 외의 폰을 숨길지
	UPROPERTY(EditAnywhere, Category = "Minimap|Hidden")
	bool _bHideItherPawns = true;

	// 이 태그가 붙은 액터는 미니맵에 안나옴.(천장, 지붕, 적군)
	UPROPERTY(EditAnywhere, Category = "Minimap|Hidden")
	FName _HideTag = TEXT("HideInMinimap");

	// 항상 숨김 클래스 Owner 복제를 기다리지 않고 스폰 즉시 걸림.
	// 기본 값으로 장착 무기가 들어가고 BP에서 추가 가능.
	UPROPERTY(EditAnywhere, Category = "Minimap|Hidden")
	TArray<TSubclassOf<AActor>> _HiddenClasses;

private:
	// 런타임에 만드는 미니맵 화면
	UPROPERTY()
	TObjectPtr<UTextureRenderTarget2D> _RenderTarget;

	//로컬 플레이어용으로 켜졌는지
	bool _bInitialized = false;

	// 액터 스폰 감시 핸들
	FDelegateHandle _ActorSpawnedHandle;

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// 빙의가 BeginPlay 뒤에 올 수 있어 컨트롤러 변경을 듣는다.
	UFUNCTION()
	void HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController);

	// 지금 로컬 플레이어인지 보고 켜거나 끈다.
	void ApplyLocalState();

	// 켤 때, 한번 렌더 타깃 생성 수김 목록 수집 후 스폰 감시
	void InitializeCapture();

	// 이 액터를 미니맵에서 빼야 하는가
	bool ShouldHide(const AActor* Actor) const;
	
	// 이미 월드에 있는 것들을 확인 후 숨김 목록을 만단다.
	void CollectHiddenActors();

	// 새로 생긴 액터를 즉시 숨김 목록에 넣는다.
	void HandleActorSpawned(AActor* SpawnedActor);

public:
	//위젯이 이걸 받아 화면에 띄운다.
	UTextureRenderTarget2D* GetRenderTarget() const { return _RenderTarget; }

	
};
