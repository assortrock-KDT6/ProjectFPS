// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "Interface/Interactable.h"
#include "InteractableDoor.generated.h"

class UStaticMeshComponent;
class USoundBase;

// 문짝 한 장이 열릴 때의 움직임. 경첩(Hinge) 컴포넌트 기준으로 적용된다.
//  - 여닫이 : _OpenRotation (예: Yaw 90)
//  - 미닫이 / 미서기 : _OpenOffset (예: Y 115)
USTRUCT(BlueprintType)
struct FDoorLeafMotion
{
	GENERATED_BODY()

	// 열렸을 때 이동량 (닫힌 위치 기준, Root 로컬)
	UPROPERTY(EditAnywhere, Category = "Door")
	FVector _OpenOffset = FVector::ZeroVector;

	// 열렸을 때 회전량 (경첩 기준)
	UPROPERTY(EditAnywhere, Category = "Door")
	FRotator _OpenRotation = FRotator::ZeroRotator;
};

// 상호작용(F)으로 열고 닫는 문. 문짝은 최대 두 장(왼쪽/오른쪽)이다.
// 오른쪽 문짝 메시를 비워두면 한 짝 문이 된다.
// 서버에서 Interact가 호출되면 열림 상태를 바꿔 복제하고, 문이 움직이는 모습은 각 화면에서 보간한다.
UCLASS()
class PROJECTFPS_API AInteractableDoor : public AActor, public IInteractable
{
	GENERATED_BODY()

protected:
	UPROPERTY(VisibleAnywhere, Category = "Door")
	TObjectPtr<USceneComponent> _Root;

	// 왼쪽 문짝의 경첩. 여닫이는 이 점을 축으로 돌고, 미닫이는 이 점이 이동한다.
	UPROPERTY(VisibleAnywhere, Category = "Door")
	TObjectPtr<USceneComponent> _LeftHinge;

	UPROPERTY(VisibleAnywhere, Category = "Door")
	TObjectPtr<UStaticMeshComponent> _LeftDoorMesh;

	// 오른쪽 문짝의 경첩. 메시를 비워두면 한 짝 문
	UPROPERTY(VisibleAnywhere, Category = "Door")
	TObjectPtr<USceneComponent> _RightHinge;

	UPROPERTY(VisibleAnywhere, Category = "Door")
	TObjectPtr<UStaticMeshComponent> _RightDoorMesh;

	UPROPERTY(EditAnywhere, Category = "Door")
	FDoorLeafMotion _LeftMotion;

	UPROPERTY(EditAnywhere, Category = "Door")
	FDoorLeafMotion _RightMotion;

	// 완전히 열리거나 닫히는 데 걸리는 시간(초)
	UPROPERTY(EditAnywhere, Category = "Door", meta = (ClampMin = "0.05", Units = "s"))
	float _MoveDuration = 0.4f;

	// 열린 뒤 자동으로 닫히기까지의 시간(초). 0이면 닫지 않는다.
	UPROPERTY(EditAnywhere, Category = "Door", meta = (ClampMin = "0.0", Units = "s"))
	float _AutoCloseDelay = 0.f;

	UPROPERTY(EditAnywhere, Category = "Door|Sound")
	TObjectPtr<USoundBase> _OpenSound;

	UPROPERTY(EditAnywhere, Category = "Door|Sound")
	TObjectPtr<USoundBase> _CloseSound;

private:
	// 열림과 양쪽 문짝의 방향을 한 값으로 복제한다.
	static constexpr uint8 OpenFlag = 1 << 0;
	static constexpr uint8 LeftReverseFlag = 1 << 1;
	static constexpr uint8 RightReverseFlag = 1 << 2;

	UPROPERTY(ReplicatedUsing = OnRep_Open)
	uint8 _OpenState = 0;

	// 0 = 닫힘, +1/-1 = 양쪽 방향으로 열림. 방향 변경도 연속적으로 보간한다.
	float _LeftOpenAlpha = 0.f;
	float _RightOpenAlpha = 0.f;

	// 에디터에 배치한 닫힌 상태의 경첩 위치
	FTransform _LeftClosedTransform;
	FTransform _RightClosedTransform;

	// 서버 전용: 자동 닫힘 타이머
	FTimerHandle _AutoCloseTimer;

public:
	AInteractableDoor();

	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;
	virtual void Tick(float DeltaSeconds) override;

	// IInteractable : 서버에서 호출된다.
	virtual void Interact_Implementation(AActor* Interactor) override;

protected:
	virtual void BeginPlay() override;

private:
	UFUNCTION()
	void OnRep_Open();

	// 서버: 열림 상태를 바꾸고 복제한다.
	void SetOpen(bool bOpen, AActor* Interactor = nullptr);

	bool IsOpen() const { return 0 != (_OpenState & OpenFlag); }

	// 닫힌 문짝의 회전 접선과 캐릭터 위치를 비교해 캐릭터 반대쪽을 선택한다.
	bool ShouldReverseLeaf(const UStaticMeshComponent* DoorMesh, const FTransform& ClosedTransform,
		const FDoorLeafMotion& Motion, const FVector& InteractorLocation) const;

	// 서버: 자동 닫힘 타이머가 호출한다.
	void CloseByTimer();

	// 열림 상태가 바뀌면 소리를 내고 움직임을 시작한다. (서버·클라이언트 공통)
	void HandleOpenChanged();

	void ApplyDoorTransform();

	// 문짝 한 장을 방향을 포함한 열림 정도(-1~1)만큼 움직인다.
	void ApplyLeafTransform(USceneComponent* Hinge, const FTransform& ClosedTransform, const FDoorLeafMotion& Motion, float Alpha);
};
