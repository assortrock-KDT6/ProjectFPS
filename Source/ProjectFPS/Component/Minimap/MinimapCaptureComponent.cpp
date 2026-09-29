// Fill out your copyright notice in the Description page of Project Settings.


#include "Component/Minimap/MinimapCaptureComponent.h"
#include "Weapons/WeaponActor.h"
#include "Engine/TextureRenderTarget2D.h"	// 카메라 화면을 텍스처로 저장하는 RenderTarget을 사용
#include "Kismet/KismetRenderingLibrary.h"	// RenderTarget을 초기화 관리하는 렌더링 기능을 사용.
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"					//	worlde에서 Actor을 검색/순회


UMinimapCaptureComponent::UMinimapCaptureComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	
	// 원근이 있으면 지도처럼 안 보인다.
	ProjectionType = ECameraProjectionMode::Orthographic;
	OrthoWidth = _ViewRange;
	
	// 게임 화면과 같은 색으로
	CaptureSource = ESceneCaptureSource::SCS_FinalColorLDR;

	// 매 프레임 갱신 (여기서 실시간 미니맵의 비용)
	bCaptureEveryFrame = true;
	bCaptureOnMovement = false;

	// 부모가 돌아도 카메라는 우리가 직접 방향으 정함.
	SetUsingAbsoluteRotation(true);

	// 손에 든 무기가 미니맵에서 뜨는것을 막음.
	_HiddenClasses.Add(AWeaponActor::StaticClass());

}

void UMinimapCaptureComponent::BeginPlay()
{
	Super::BeginPlay();

	APawn* OwnerPawn = Cast<APawn>(GetOwner()); // 폰인지 확인.

	// 내 화면용 -> 로컬 플레이어가 아니면 둘 필요가 없음.
	if (nullptr == OwnerPawn)
	{
		bCaptureEveryFrame = false;
		SetComponentTickEnabled(false);
		return;
	}

	// 연결이 나중에 올 수 있으므려 변경을 듣는다.
	OwnerPawn->ReceiveControllerChangedDelegate.AddDynamic(this, &UMinimapCaptureComponent::HandleControllerChanged);
	
	ApplyLocalState();
}

void UMinimapCaptureComponent::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	// 미니맵 컴포넌트의 플레이가 끝날 때, 등록했던 이벤트 알림을 해제하는 코드.

	UWorld* world = GetWorld();
	if (nullptr != world && _ActorSpawnedHandle.IsValid())
	{
		world->RemoveOnActorSpawnedHandler(_ActorSpawnedHandle);
		_ActorSpawnedHandle.Reset();
	}

	APawn* OwnerPawn = Cast<APawn>(GetOwner());
	if (nullptr != OwnerPawn)
	{
		OwnerPawn->ReceiveControllerChangedDelegate.RemoveDynamic(this, &UMinimapCaptureComponent::HandleControllerChanged);
	}

	Super::EndPlay(EndPlayReason);


}

void UMinimapCaptureComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	
	AActor* OwnerActor = GetOwner();
	if (false == IsValid(OwnerActor))
		return;
	
	// 캐릭터 머리 위로
	FVector Location = OwnerActor->GetActorLocation();
	Location.Z += _Height;
	SetWorldLocation(Location);

	// 바로 아래를 보되, 필요하면 시선 방향으로 돌린다.
	float Yaw = 0.f;
	if (_bRotateWithPlayer)
	{
		APawn* OwnerPawn = Cast<APawn>(OwnerActor);
		if (nullptr != OwnerPawn)
		{
			APlayerController* Pc = Cast<APlayerController>(OwnerPawn->GetController());
			if (nullptr != Pc)
				Yaw = Pc->GetControlRotation().Yaw;
		}
	}
	SetWorldRotation(FRotator(-90.f, Yaw, 0.f));

}

void UMinimapCaptureComponent::HandleControllerChanged(APawn* Pawn, AController* OldController, AController* NewController)
{
	ApplyLocalState();
}

void UMinimapCaptureComponent::ApplyLocalState()
{
	APawn* OwnerPawn = Cast<APawn>(GetOwner());

	// IsLocallyControlled()는 서버의 AI도 true다.
	// 미니맵은 화면을 가진 플레이어 것이므로 PlayerController까지 확인한다.
	bool bLocalPlayer = false;
	if (nullptr != OwnerPawn)
	{
		APlayerController* Pc = Cast<APlayerController>(OwnerPawn->GetController());
		if (nullptr != Pc && Pc->IsLocalController())
			bLocalPlayer = true;
	}

	if (false == bLocalPlayer)
	{
		bCaptureEveryFrame = false;
		SetComponentTickEnabled(false);
		return;
	}
	SetComponentTickEnabled(true);
	bCaptureEveryFrame = true;
	
	if (false == _bInitialized)
		InitializeCapture();
}

void UMinimapCaptureComponent::InitializeCapture()
{
	UWorld* World = GetWorld();
	if (nullptr == World)
		return;

	// 매니맵 화면을 런타임에 만든다. (에셋x )
	_RenderTarget = UKismetRenderingLibrary::CreateRenderTarget2D(this, _Resolution, _Resolution, RTF_RGBA8);
	TextureTarget = _RenderTarget;
	
	OrthoWidth = _ViewRange;

	// 이미 있는 것들.
	CollectHiddenActors();
	
	// 앞으로 생기는 것들 -> 스폰 즉시 등록하기에 노출되지 않음.
	_ActorSpawnedHandle = World->AddOnActorSpawnedHandler(
		FOnActorSpawned::FDelegate::CreateUObject(this, &UMinimapCaptureComponent::HandleActorSpawned));

	_bInitialized = true;
}

bool UMinimapCaptureComponent::ShouldHide(const AActor* Actor) const
{
	if (false == IsValid(Actor))
		return false;

	// 자기자신은 보인다.
	if (Actor == GetOwner())
		return false;

	// 태그로 지정한 것.
	if(false == _HideTag.IsNone() && Actor->ActorHasTag(_HideTag))
		return true;

	// 클래스로 지정한 것(장차 무기 등)
	for (const TSubclassOf<AActor>& HiddenClass : _HiddenClasses)
	{
		if (nullptr != HiddenClass && Actor->IsA(HiddenClass))
			return true;
	}

	// 다른 폰
	if (_bHideItherPawns && Actor->IsA<APawn>())
		return true;

	return false;
}

void UMinimapCaptureComponent::CollectHiddenActors()
{
	UWorld* World = GetWorld();
	if (nullptr == World)
		return;

	HiddenActors.Reset();
	
	for (TActorIterator<AActor> It(World); It; ++It)
	{
		AActor* Actor = *It;
		if (ShouldHide(Actor))
			HiddenActors.Add(Actor);
	}
	MarkRenderStateDirty();
}

void UMinimapCaptureComponent::HandleActorSpawned(AActor* SpawnedActor)
{
	// 파괴된 액터가 목록에 쌓이지 않게 정리
	for (int32 i = HiddenActors.Num() - 1; i >= 0; --i)
	{
		if (false == IsValid(HiddenActors[i]))
		{
			HiddenActors.RemoveAt(i);
		}
	}
	if (ShouldHide(SpawnedActor))
	{
		HiddenActors.AddUnique(SpawnedActor);
	}
	MarkRenderStateDirty();
}
