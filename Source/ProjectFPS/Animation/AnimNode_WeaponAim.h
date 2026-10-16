#pragma once

#include "CoreMinimal.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_WeaponAim.generated.h"

// Control Rig에 조준 입력을 전달하거나, 자세 보정 후 총구 방향을 수렴시킨다.
USTRUCT(BlueprintInternalUseOnly)
struct PROJECTFPS_API FAnimNode_WeaponAim : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

	FAnimNode_WeaponAim();

	// 준비 노드 → 자세 Control Rig → 수렴 노드 → 지지 손·시선 Control Rig 순서로 사용한다.
	UPROPERTY(EditAnywhere, Category = "Weapon Aim")
	bool _PrepareControlRig = false;

	// 무기 부착 본이 상체 계층 안에 있는지 확인하는 기준 본이다.
	UPROPERTY(EditAnywhere, Category = "Weapon Aim")
	FBoneReference _SpineBone;

	UPROPERTY(EditAnywhere, Category = "Weapon Aim")
	FName _MuzzleSocket = TEXT("Muzzle");

	// 프로젝트 총기 메시의 총열은 로컬 +Y 방향을 향한다.
	UPROPERTY(EditAnywhere, Category = "Weapon Aim")
	FVector _BarrelAxis = FVector::RightVector;

	virtual bool HasPreUpdate() const override;
	virtual void PreUpdate(const UAnimInstance* Instance) override;
	virtual void GatherDebugData(FNodeDebugData& DebugData) override;

protected:
	virtual void InitializeBoneReferences(const FBoneContainer& RequiredBones) override;
	virtual bool IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones) override;
	virtual void UpdateInternal(const FAnimationUpdateContext& Context) override;
	virtual void EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output,
		TArray<FBoneTransform>& OutBoneTransforms) override;

private:
	FBoneReference _LeftHand;
	bool _IsADS = false;

	// 게임 스레드에서 복사한 값만 병렬 포즈 평가에 사용한다.
	bool _AlignWeapon = false;
	bool _HasAimPoint = false;
	float _DeltaTime = 0.f;
	FVector _SmoothedAimPointWorld = FVector::ZeroVector;
	FQuat _ViewRotationWorld = FQuat::Identity;
	FVector _AimOriginWorld = FVector::ZeroVector;
	double _InverseAimDepth = 0.0;
	double _AimDepthVelocity = 0.0;

	FVector _AimPointWorld = FVector::ZeroVector;

	FVector _MuzzleInWeapon = FVector::ZeroVector;

	FTransform _WeaponInGrip = FTransform::Identity;
	FTransform _MeshWorld = FTransform::Identity;

	int32 _GripMeshIndex = INDEX_NONE;

	// 진단을 켰을 때만 게임 스레드의 시각·역할을 복사한다.
	int32 _DebugRole = 0;
	double _DebugTime = 0.0;
};
