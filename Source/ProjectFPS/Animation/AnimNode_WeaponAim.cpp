#include "Animation/AnimNode_WeaponAim.h"

#include "Animation/AnimInstanceProxy.h"
#include "Animation/WeaponAimSolver.h"
#include "Animation/WeaponPoseCurves.h"
#include "AbilitySystemComponent.h"
#include "Character/CharacterPlayer.h"
#include "Component/Movement/FPSCharacterMovementComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "GameTag/FPSGameplayTag.h"
#include "Projectiles/GrenadeActor.h"
#include "Weapons/WeaponActor.h"

static TAutoConsoleVariable<int32> CVarWeaponAimDiagnostics(
	TEXT("fps.WeaponAim.Diagnostics"), 0, TEXT("총구 포즈의 입력과 평가 결과 기록"));

FAnimNode_WeaponAim::FAnimNode_WeaponAim()
{
	_SpineBone.BoneName = TEXT("spine_03");
	_LeftHand.BoneName = TEXT("hand_l");
}

bool FAnimNode_WeaponAim::HasPreUpdate() const
{
	return true;
}

void FAnimNode_WeaponAim::PreUpdate(const UAnimInstance* Instance)
{
	_AlignWeapon = false;
	_GripMeshIndex = INDEX_NONE;

	const ACharacterPlayer* Owner = Cast<ACharacterPlayer>(Instance->GetOwningActor());
	const USkeletalMeshComponent* Mesh = Instance->GetSkelMeshComponent();
	if (false == IsValid(Owner) || false == IsValid(Mesh) || Mesh != Owner->GetMesh()
		|| true == Owner->IsDeathStarted() || true == IsValid(Owner->GetEquippedGrenade()))
	{
		return;
	}

	const AWeaponActor* Weapon = Owner->GetEquippedWeapon();
	const USceneComponent* WeaponMesh = true == IsValid(Weapon) ? Weapon->GetWeaponMeshComponent() : nullptr;
	if (false == IsValid(WeaponMesh) || WeaponMesh->GetAttachParent() != Mesh
		|| false == WeaponMesh->DoesSocketExist(_MuzzleSocket))
	{
		return;
	}

	const EWeaponType WeaponType = Weapon->GetWeaponData()._WeaponType;
	if (WeaponType != EWeaponType::Pistol && WeaponType != EWeaponType::Rifle
		&& WeaponType != EWeaponType::Shotgun && WeaponType != EWeaponType::Sniper)
	{
		return;
	}

	// 재장전·근접 공격·파쿠르 중에는 원래 손 동작을 보존한다.
	const UAnimInstance* BodyInstance = Mesh->GetAnimInstance();
	const UFPSCharacterMovementComponent* Movement = Cast<UFPSCharacterMovementComponent>(Owner->GetCharacterMovement());
	if ((true == IsValid(BodyInstance) && true == BodyInstance->IsAnyMontagePlaying())
		|| (true == IsValid(Movement) && true == Movement->IsTraversing()))
	{
		return;
	}

	// 동명의 본이 있더라도 실제 부착 소켓의 부모 본과 오프셋을 사용한다.
	const FName GripName = WeaponMesh->GetAttachSocketName();
	const USkeletalMeshSocket* GripSocket = Mesh->GetSocketByName(GripName);
	_GripMeshIndex = Mesh->GetBoneIndex(nullptr != GripSocket ? GripSocket->BoneName : GripName);
	_WeaponInGrip = WeaponMesh->GetRelativeTransform()
		* (nullptr != GripSocket ? GripSocket->GetSocketLocalTransform() : FTransform::Identity);
	_MuzzleInWeapon = WeaponMesh->GetSocketTransform(_MuzzleSocket, RTS_Component).GetLocation();
	_AimPointWorld = Owner->GetWeaponPoseAimPoint(Weapon->GetWeaponRange());
	_AimOriginWorld = Owner->GetWeaponAimOrigin();
	_ViewRotationWorld = Owner->GetWeaponAimRotation().Quaternion();
	const UAbilitySystemComponent* Ability = Owner->GetAbilitySystemComponent();
	_IsADS = true == IsValid(Ability) && true == Ability->HasMatchingGameplayTag(FPSGameplayTags::Status_ADS);
	// 서버 이동 처리 중에는 자식 메시의 월드 변환 갱신이 지연된다.
	// 현재 캡슐과 메시 상대 변환으로 평가 후 실제 사용할 좌표계를 구성한다.
	_MeshWorld = true == Owner->HasAuthority()
		? Mesh->GetRelativeTransform() * Owner->GetActorTransform() : Mesh->GetComponentTransform();
	_DebugRole = CVarWeaponAimDiagnostics.GetValueOnGameThread() != 0 ? static_cast<int32>(Owner->GetLocalRole()) : 0;
	_DebugTime = Owner->GetWorld()->GetTimeSeconds();
	_AlignWeapon = _GripMeshIndex != INDEX_NONE;
}

void FAnimNode_WeaponAim::InitializeBoneReferences(const FBoneContainer& RequiredBones)
{
	_SpineBone.Initialize(RequiredBones);
	_LeftHand.Initialize(RequiredBones);
}

bool FAnimNode_WeaponAim::IsValidToEvaluate(const USkeleton* Skeleton, const FBoneContainer& RequiredBones)
{
	if (false == _AlignWeapon || true == _BarrelAxis.IsNearlyZero()
		|| false == _SpineBone.IsValidToEvaluate(RequiredBones))
	{
		return false;
	}

	const FCompactPoseBoneIndex Grip = RequiredBones.MakeCompactPoseIndex(FMeshPoseBoneIndex(_GripMeshIndex));
	const FCompactPoseBoneIndex Spine = _SpineBone.GetCompactPoseIndex(RequiredBones);
	return Grip != INDEX_NONE && true == RequiredBones.BoneIsChildOf(Grip, Spine);
}

void FAnimNode_WeaponAim::UpdateInternal(const FAnimationUpdateContext& Context)
{
	FAnimNode_SkeletalControlBase::UpdateInternal(Context);
	_DeltaTime = Context.GetDeltaTime();
	if (false == _AlignWeapon)
	{
		_HasAimPoint = false;
	}
}

void FAnimNode_WeaponAim::EvaluateSkeletalControl_AnyThread(FComponentSpacePoseContext& Output,
	TArray<FBoneTransform>& OutBoneTransforms)
{
	const FBoneContainer& Bones = Output.Pose.GetPose().GetBoneContainer();
	const FCompactPoseBoneIndex Grip = Bones.MakeCompactPoseIndex(FMeshPoseBoneIndex(_GripMeshIndex));
	const FTransform WeaponCS = _WeaponInGrip * Output.Pose.GetComponentSpaceTransform(Grip);
	const FVector Muzzle = WeaponCS.TransformPosition(_MuzzleInWeapon);
	const FVector Barrel = WeaponCS.TransformVectorNoScale(_BarrelAxis).GetSafeNormal();
	const FTransform& MeshWorld = _MeshWorld;
	if (true == _PrepareControlRig)
	{
		const bool Reset = false == _HasAimPoint || _DeltaTime > 0.15f;
		// 총구 보정 회전은 지연시키지 않는다. 현재 시선 위의 표시용 목표 깊이만 완화한다.
		// 캐릭터 경계에서 속도가 연속적으로 변하므로 첫 접촉 프레임의 급격한 꺾임을 줄인다.
		_SmoothedAimPointWorld = WeaponAim::SmoothTargetDepth(_AimOriginWorld, _ViewRotationWorld.GetForwardVector(),
			_AimPointWorld, _DeltaTime, Reset, _InverseAimDepth, _AimDepthVelocity);
		_HasAimPoint = true;

		const FVector Target = MeshWorld.InverseTransformPosition(_SmoothedAimPointWorld);
		const FQuat Initial = WeaponAim::Solve(Output.Pose.GetComponentSpaceTransform(Grip).GetLocation(), Muzzle, Barrel, Target);
		Output.Curve.Set(WeaponPose::Enabled, 1.f);
		Output.Curve.Set(WeaponPose::ADS, true == _IsADS ? 1.f : 0.f);
		Output.Curve.Set(WeaponPose::Reset, true == Reset ? 1.f : 0.f);
		WeaponPose::Write(Output.Curve, WeaponPose::Target, Target);
		WeaponPose::Write(Output.Curve, WeaponPose::Initial, Initial);
		WeaponPose::Write(Output.Curve, WeaponPose::MeshRotation, MeshWorld.GetRotation());
		return;
	}

	// 준비 단계가 평가되지 않은 포즈에는 커브의 기본값으로 조준을 적용하지 않는다.
	if (Output.Curve.Get(WeaponPose::Enabled) < 0.5f)
	{
		return;
	}
	const FVector Target = WeaponPose::Read(Output.Curve, WeaponPose::Target);
	const FTransform BeforeWeapon = _WeaponInGrip * Output.Pose.GetComponentSpaceTransform(Grip);
	const FVector HandPivot = Output.Pose.GetComponentSpaceTransform(Grip).GetLocation();
	const FQuat Correction = WeaponAim::Solve(HandPivot, BeforeWeapon.TransformPosition(_MuzzleInWeapon),
		BeforeWeapon.TransformVectorNoScale(_BarrelAxis).GetSafeNormal(), Target);
	FTransform HandTarget = Output.Pose.GetComponentSpaceTransform(Grip);
	HandTarget.SetRotation((Correction * HandTarget.GetRotation()).GetNormalized());
	FTransform LeftTarget;
	const bool HasLeft = true == _LeftHand.IsValidToEvaluate(Bones);
	if (true == HasLeft)
	{
		LeftTarget = Output.Pose.GetComponentSpaceTransform(_LeftHand.GetCompactPoseIndex(Bones));
		LeftTarget.SetLocation(HandPivot + Correction.RotateVector(LeftTarget.GetLocation() - HandPivot));
		LeftTarget.SetRotation((Correction * LeftTarget.GetRotation()).GetNormalized());
	}
	// 오른손의 총구 수렴은 C++에 남기고 지지 팔의 관절 계산은 후속 Control Rig에 맡긴다.
	OutBoneTransforms.Add(FBoneTransform(Grip, HandTarget));
	if (true == HasLeft)
	{
		WeaponPose::Write(Output.Curve, WeaponPose::LeftPosition, LeftTarget.GetLocation());
		WeaponPose::Write(Output.Curve, WeaponPose::LeftRotation, LeftTarget.GetRotation());
	}
	if (_DebugRole != 0)
	{
		const FVector FinalBarrel = MeshWorld.TransformVectorNoScale((_WeaponInGrip * HandTarget).TransformVectorNoScale(_BarrelAxis));
		UE_LOG(LogTemp, Display, TEXT("WeaponAimTiming role=%d time=%.6f dt=%.6f view=%.6f mesh=%.6f raw=%s smooth=%s barrel=%s"),
			_DebugRole, _DebugTime, _DeltaTime, _ViewRotationWorld.Rotator().Yaw, MeshWorld.Rotator().Yaw,
			*_AimPointWorld.ToString(), *MeshWorld.TransformPosition(Target).ToString(), *FinalBarrel.ToString());
	}
}

void FAnimNode_WeaponAim::GatherDebugData(FNodeDebugData& DebugData)
{
	FString DebugLine = DebugData.GetNodeName(this);
	AddDebugNodeData(DebugLine);
	DebugLine += FString::Printf(TEXT(" Spine: %s, Ready: %d"), *_SpineBone.BoneName.ToString(), _AlignWeapon);
	DebugData.AddDebugItem(DebugLine);
	ComponentPose.GatherDebugData(DebugData);
}
