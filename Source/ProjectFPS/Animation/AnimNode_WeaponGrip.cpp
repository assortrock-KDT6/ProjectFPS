#include "Animation/AnimNode_WeaponGrip.h"

#include "Animation/AnimInstanceProxy.h"
#include "Animation/WeaponAimSolver.h"
#include "Animation/WeaponPoseCurves.h"
#include "AbilitySystemComponent.h"
#include "Character/CharacterPlayer.h"
#include "Component/Movement/FPSCharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "HAL/IConsoleManager.h"
#include "GameTag/FPSGameplayTag.h"
#include "Projectiles/GrenadeActor.h"
#include "TwoBoneIK.h"
#include "Weapons/WeaponActor.h"

static TAutoConsoleVariable<int32> CVarWeaponGripDiagnostics(
	TEXT("fps.WeaponGrip.Diagnostics"), 0, TEXT("근거리 팔 후퇴량 기록"));

FAnimNode_WeaponGrip::FAnimNode_WeaponGrip()
{
	_LeftHand.BoneName = TEXT("hand_l");
	_RightHand.BoneName = TEXT("hand_r");
	_AimPivot.BoneName = TEXT("spine_03");
}

bool FAnimNode_WeaponGrip::HasPreUpdate() const
{
	return true;
}

void FAnimNode_WeaponGrip::PreUpdate(const UAnimInstance* Instance)
{
	_Ready = false;
	_TargetWeight = 0.f;
	const ACharacterPlayer* Owner = Cast<ACharacterPlayer>(Instance->GetOwningActor());
	const USkeletalMeshComponent* Mesh = Instance->GetSkelMeshComponent();
	if (false == IsValid(Owner) || false == IsValid(Mesh) || true == Owner->IsDeathStarted())
	{
		return;
	}

	const AWeaponActor* Weapon = Owner->GetEquippedWeapon();
	const USceneComponent* WeaponMesh = true == IsValid(Weapon) ? Weapon->GetWeaponMeshComponent() : nullptr;
	if (false == IsValid(WeaponMesh) || false == WeaponMesh->DoesSocketExist(_LeftGripSocket)
		|| false == WeaponMesh->DoesSocketExist(_RightGripSocket))
	{
		return;
	}

	_AttachedToEvaluatedMesh = WeaponMesh->GetAttachParent() == Mesh;
	// 원본 메시의 서버 이동 스코프 안에서는 월드 변환이 아직 이전 프레임 값이다.
	_MeshWorld = true == Owner->HasAuthority() && Mesh == Owner->GetMesh()
		? Mesh->GetRelativeTransform() * Owner->GetActorTransform() : Mesh->GetComponentTransform();
	if (true == _AttachedToEvaluatedMesh)
	{
		const FName SocketName = WeaponMesh->GetAttachSocketName();
		const USkeletalMeshSocket* Socket = Mesh->GetSocketByName(SocketName);
		_GripMeshIndex = Mesh->GetBoneIndex(nullptr != Socket ? Socket->BoneName : SocketName);
		_WeaponInGrip = WeaponMesh->GetRelativeTransform()
			* (nullptr != Socket ? Socket->GetSocketLocalTransform() : FTransform::Identity);
		_AimPointWorld = Owner->GetWeaponPoseAimPoint(Weapon->GetWeaponRange());
		// 초근접에서 총구 수렴을 생략할 때 손만 따로 후퇴하지 않도록 같은 표시용 깊이를 사용한다.
		const FVector AimOrigin = Owner->GetWeaponAimOrigin();
		const FVector AimDirection = Owner->GetWeaponAimRotation().Vector();
		_AimPointWorld = AimOrigin + AimDirection * WeaponAim::PoseTargetDepth(
			FVector::DotProduct(_AimPointWorld - AimOrigin, AimDirection));
		_MuzzleInWeapon = WeaponMesh->GetSocketTransform(TEXT("Muzzle"), RTS_Component).GetLocation();
	}
	else
	{
		// 표시용 Body는 원본 메시의 병렬 포즈 평가가 끝난 다음 Tick하도록 설정되어 있다.
		_WeaponWorld = WeaponMesh->GetComponentTransform();
	}

	_LeftGrip = WeaponMesh->GetSocketTransform(_LeftGripSocket, RTS_Component);
	_RightGrip = WeaponMesh->GetSocketTransform(_RightGripSocket, RTS_Component);
	_PrimaryHandRetraction = FMath::Clamp(Weapon->GetWeaponData()._ThirdPersonHandRetraction, 0.f, 15.f);
	const UAbilitySystemComponent* Ability = Owner->GetAbilitySystemComponent();
	_TargetADSWeight = true == IsValid(Ability) && true == Ability->HasMatchingGameplayTag(FPSGameplayTags::Status_ADS) ? 1.f : 0.f;
	const UAnimInstance* Body = Owner->GetMesh()->GetAnimInstance();
	const UFPSCharacterMovementComponent* Movement = Cast<UFPSCharacterMovementComponent>(Owner->GetCharacterMovement());
	const bool Busy = true == IsValid(Owner->GetEquippedGrenade())
		|| (true == IsValid(Body) && true == Body->IsAnyMontagePlaying())
		|| (true == IsValid(Movement) && true == Movement->IsTraversing());
	_TargetWeight = true == Busy ? 0.f : 1.f;
	_Ready = true;
}

void FAnimNode_WeaponGrip::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	_LeftHand.Initialize(RequiredBones);
	_RightHand.Initialize(RequiredBones);
	_AimPivot.Initialize(RequiredBones);
}

bool FAnimNode_WeaponGrip::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	if (false == _Ready || false == _LeftHand.IsValidToEvaluate(RequiredBones)
		|| false == _RightHand.IsValidToEvaluate(RequiredBones))
	{
		return false;
	}

	for (const FBoneReference* Hand : { &_LeftHand, &_RightHand })
	{
		const FCompactPoseBoneIndex Lower = RequiredBones.GetParentBoneIndex(Hand->GetCompactPoseIndex(RequiredBones));
		if (Lower == INDEX_NONE || RequiredBones.GetParentBoneIndex(Lower) == INDEX_NONE)
		{
			return false;
		}
	}

	return false == _AttachedToEvaluatedMesh
		|| RequiredBones.MakeCompactPoseIndex(FMeshPoseBoneIndex(_GripMeshIndex)) != INDEX_NONE;
}

void FAnimNode_WeaponGrip::UpdateInternal(const FAnimationUpdateContext& Context)
{
	FAnimNode_SkeletalControlBase::UpdateInternal(Context);
	_DeltaTime = Context.GetDeltaTime();
	_ADSWeight = FMath::Lerp(_ADSWeight, _TargetADSWeight, 1.f - FMath::Exp(-10.f * _DeltaTime));
	// 재장전·근접 공격으로 전환할 때 손이 그립에서 자연스럽게 풀리도록 한다.
	_GripWeight = FMath::FInterpTo(_GripWeight, _TargetWeight, Context.GetDeltaTime(), 20.f);
	if (false == _Ready || _TargetWeight == 0.f)
	{
		_NearRetraction = 0.f;
		_RetractionVelocity = 0.0;
	}
}

void FAnimNode_WeaponGrip::SolveHand(FComponentSpacePoseContext& Output, const FBoneReference& Hand,
	const FTransform& Target, TArray<FBoneTransform>& OutBoneTransforms) const
{
	const FBoneContainer& Bones = Output.Pose.GetPose().GetBoneContainer();
	const FCompactPoseBoneIndex End = Hand.GetCompactPoseIndex(Bones);
	const FCompactPoseBoneIndex Lower = Bones.GetParentBoneIndex(End);
	const FCompactPoseBoneIndex Upper = Bones.GetParentBoneIndex(Lower);
	FTransform UpperCS = Output.Pose.GetComponentSpaceTransform(Upper);
	FTransform LowerCS = Output.Pose.GetComponentSpaceTransform(Lower);
	FTransform EndCS = Output.Pose.GetComponentSpaceTransform(End);

	// 원래 팔꿈치가 굽은 평면과 팔 길이를 유지하며 손목 위치·방향을 맞춘다.
	FVector JointTarget = LowerCS.GetLocation();
	AnimationCore::SolveTwoBoneIK(UpperCS, LowerCS, EndCS, JointTarget, Target.GetLocation(), false, 1.0, 1.0);
	// 메타휴먼 손목은 Quinn과 본의 축이 다르다. 소켓의 Quinn 회전을 복사하면
	// 손바닥과 손가락이 비틀리므로 표시용 메시의 리타기팅된 손목 방향을 보존한다.
	if (true == _AttachedToEvaluatedMesh)
	{
		EndCS.SetRotation(Target.GetRotation().GetNormalized());
	}
	else
	{
		EndCS.SetRotation(Output.Pose.GetComponentSpaceTransform(End).GetRotation());
	}
	FTransform Blended;
	Blended.Blend(Output.Pose.GetComponentSpaceTransform(Upper), UpperCS, _GripWeight);
	OutBoneTransforms.Add(FBoneTransform(Upper, Blended));
	Blended.Blend(Output.Pose.GetComponentSpaceTransform(Lower), LowerCS, _GripWeight);
	OutBoneTransforms.Add(FBoneTransform(Lower, Blended));
	Blended.Blend(Output.Pose.GetComponentSpaceTransform(End), EndCS, _GripWeight);
	OutBoneTransforms.Add(FBoneTransform(End, Blended));
}

void FAnimNode_WeaponGrip::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output,
	TArray<FBoneTransform>& OutBoneTransforms)
{
	const FBoneContainer& Bones = Output.Pose.GetPose().GetBoneContainer();
	FTransform WeaponCS = true == _AttachedToEvaluatedMesh
		? _WeaponInGrip * Output.Pose.GetComponentSpaceTransform(Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(_GripMeshIndex)))
		: _WeaponWorld.GetRelativeTransform(_MeshWorld);

	if (true == _AttachedToEvaluatedMesh
		&& Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(_GripMeshIndex)) == _RightHand.GetCompactPoseIndex(Bones))
	{
		const FCompactPoseBoneIndex Hand = _RightHand.GetCompactPoseIndex(Bones);
		const FCompactPoseBoneIndex Shoulder = Bones.GetParentBoneIndex(Bones.GetParentBoneIndex(Hand));
		FTransform Target = Output.Pose.GetComponentSpaceTransform(Hand);
		const FVector TowardShoulder = (Output.Pose.GetComponentSpaceTransform(Shoulder).GetLocation()
			- Target.GetLocation()).GetSafeNormal();
		Target.AddToTranslation(TowardShoulder * _PrimaryHandRetraction);
		if (true == _AimPivot.IsValidToEvaluate(Bones))
		{
			const FTransform CandidateWeapon = _WeaponInGrip * Target;
			const FVector Barrel = CandidateWeapon.GetUnitAxis(EAxis::Y);
			const FVector AimPoint = _MeshWorld.InverseTransformPosition(_AimPointWorld);
			// 오른손과 총을 뒤·아래로 함께 이동한다. 뒤로만 당기면 손이 어깨에 겹쳐 팔이 접힌다.
			const FVector Down = _MeshWorld.InverseTransformVectorNoScale(-FVector::UpVector);
			const FCompactPoseBoneIndex LeftLower = Bones.GetParentBoneIndex(_LeftHand.GetCompactPoseIndex(Bones));
			const FCompactPoseBoneIndex LeftUpper = Bones.GetParentBoneIndex(LeftLower);
			const FVector Outward = (Output.Pose.GetComponentSpaceTransform(Shoulder).GetLocation()
				- Output.Pose.GetComponentSpaceTransform(LeftUpper).GetLocation()).GetSafeNormal();
			// ADS는 견착 높이를 유지하고 몸 바깥쪽 공간을 사용한다. 손을 몸통 안으로 밀어 넣지 않는다.
			const FVector Retreat = (-Barrel * FMath::Lerp(0.6, 0.7, _ADSWeight)
				+ Down * FMath::Lerp(0.8, 0.3, _ADSWeight) + Outward * (0.6 * _ADSWeight)).GetSafeNormal();
			const float DesiredRetraction = WeaponAim::HandRetraction(Target.GetLocation(),
				CandidateWeapon.TransformPosition(_MuzzleInWeapon), AimPoint, Retreat, FMath::Clamp(_MaxNearRetraction, 0.f, 35.f));
			// 목표 경계에서 후퇴 속도도 연속적으로 바뀌어 손이 갑자기 당겨지지 않게 한다.
			_NearRetraction = static_cast<float>(WeaponAim::SmoothTargetValue(
				_NearRetraction, DesiredRetraction, _RetractionVelocity, _DeltaTime));
			if (CVarWeaponGripDiagnostics.GetValueOnAnyThread() != 0)
			{
				UE_LOG(LogTemp, Display, TEXT("WeaponGripRetraction desired=%.3f actual=%.3f maximum=%.3f"),
					DesiredRetraction, _NearRetraction, _MaxNearRetraction);
			}
			Target.AddToTranslation(Retreat * _NearRetraction);
			// 팔꿈치 자세는 다음 Control Rig에서 같은 프레임의 손 위치를 유지하며 조정한다.
			Output.Curve.Set(WeaponPose::Retraction, _NearRetraction * _GripWeight);
		}
		SolveHand(Output, _RightHand, Target, OutBoneTransforms);
		// 같은 평가에서 보정된 오른손으로 총과 왼손 목표를 계산한다. 총구 조준은 이 노드 다음에 적용한다.
		WeaponCS = _WeaponInGrip * OutBoneTransforms.Last().Transform;
	}

	SolveHand(Output, _LeftHand, _LeftGrip * WeaponCS, OutBoneTransforms);
	// 표시용 오른손은 리타기팅 후 실제 무기 그립에 다시 맞춘다.
	if (false == _AttachedToEvaluatedMesh)
	{
		SolveHand(Output, _RightHand, _RightGrip * WeaponCS, OutBoneTransforms);
	}
	OutBoneTransforms.Sort(FCompareBoneTransformIndex());
}

void FAnimNode_WeaponGrip::GatherDebugData(FNodeDebugData& DebugData)
{
	FString Line = DebugData.GetNodeName(this);
	AddDebugNodeData(Line);
	DebugData.AddDebugItem(Line + FString::Printf(TEXT(" Grip: %.2f"), _GripWeight));
	ComponentPose.GatherDebugData(DebugData);
}
