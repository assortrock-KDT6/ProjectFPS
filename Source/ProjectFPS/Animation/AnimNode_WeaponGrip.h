#pragma once

#include "CoreMinimal.h"
#include "BoneControllers/AnimNode_SkeletalControlBase.h"
#include "AnimNode_WeaponGrip.generated.h"

// 총의 그립 소켓에 손목을 맞춘다. 리타기팅된 표시용 메시에도 적용할 수 있다.
USTRUCT(BlueprintInternalUseOnly)
struct PROJECTFPS_API FAnimNode_WeaponGrip : public FAnimNode_SkeletalControlBase
{
	GENERATED_BODY()

	FAnimNode_WeaponGrip();

	UPROPERTY(EditAnywhere, Category = "Weapon Grip")
	FBoneReference _LeftHand;

	UPROPERTY(EditAnywhere, Category = "Weapon Grip")
	FBoneReference _RightHand;

	UPROPERTY(EditAnywhere, Category = "Weapon Grip")
	FName _LeftGripSocket = TEXT("LeftHandGrip");

	UPROPERTY(EditAnywhere, Category = "Weapon Grip")
	FName _RightGripSocket = TEXT("RightHandGrip");

	// 가까운 목표가 총구 뒤에 놓일 때 허용할 최대 후퇴량이다. 원본 팔 길이는 늘리지 않는다.
	UPROPERTY(EditAnywhere, Category = "Weapon Grip", meta = (ClampMin = "0", ClampMax = "35"))
	float _MaxNearRetraction = 24.f;

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
	void SolveHand(FComponentSpacePoseContext& Output, const FBoneReference& Hand,
		const FTransform& Target, TArray<FBoneTransform>& OutBoneTransforms) const;

	bool _Ready = false;
	bool _AttachedToEvaluatedMesh = false;
	bool _FirstPerson = false;
	float _TargetWeight = 0.f;
	float _GripWeight = 0.f;
	float _PrimaryHandRetraction = 0.f;
	FVector _PrimaryHandOffset = FVector::ZeroVector;
	// PIE에서 최종 후퇴량을 확인하기 위한 읽기 전용 진단값이다.
	UPROPERTY(VisibleAnywhere, Transient, Category = "Diagnostics")
	float _NearRetraction = 0.f;
	double _RetractionVelocity = 0.0;
	float _DeltaTime = 0.f;
	float _TargetADSWeight = 0.f;
	float _ADSWeight = 0.f;
	FBoneReference _AimPivot;
	FVector _AimPointWorld = FVector::ZeroVector;
	FVector _MuzzleInWeapon = FVector::ZeroVector;
	int32 _GripMeshIndex = INDEX_NONE;
	FTransform _WeaponInGrip = FTransform::Identity;
	FTransform _WeaponWorld = FTransform::Identity;
	FTransform _MeshWorld = FTransform::Identity;
	FTransform _LeftGrip = FTransform::Identity;
	FTransform _RightGrip = FTransform::Identity;
};
