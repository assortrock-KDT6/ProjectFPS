// 프로젝트 설정의 설명 페이지에 저작권 정보를 입력하세요.

#pragma once

#include "EngineMinimal.h"
#include "GameFramework/Character.h"
#include "AbilitySystemInterface.h"
#include "Component/Ability/FPSAbilitySystemComponent.h"
#include "CharacterBase.generated.h"


UCLASS()
class PROJECTFPS_API ACharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	ACharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

	// 빙의 전에 기존 블루프린트의 능력 컴포넌트 참조를 확인한다.
	virtual void PostInitializeComponents() override;

protected:
	// 캐릭터의 고유 아이디
	UPROPERTY(BlueprintReadWrite)
	FName	_Id = FName(TEXT(""));

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "FPS | AbilitySystem")
	TObjectPtr<UFPSAbilitySystemComponent> _AbilitySystemComponent;

protected:
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "FPS | AbilitySystem")
	EGameplayEffectReplicationMode _AbilitySystemComponentReplicationMode = EGameplayEffectReplicationMode::Mixed;

protected:
	virtual void BeginPlay() override;

public:	
	virtual void Tick(float DeltaTime) override;

	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

	virtual void PossessedBy(AController* Newcontroller) override;

	// 플레이어가 서버에 접속하거나 만들어짐.
	virtual void OnRep_PlayerState() override;

	virtual void OnMovementModeChanged(EMovementMode PrevMovementMode, uint8 PreviousCustomMode = 0) override;

	virtual void OnStartCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

	virtual void OnEndCrouch(float HalfHeightAdjust, float ScaledHalfHeightAdjust) override;

public:
	// 캐릭터의 고유 아이디를 가져온다.
	const FName& GetId() const;

	// 캐릭터의 고유 아이디를 설정한다.
	void SetId(const FName& Id);

	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const ;

	// 행동을 시작·종료·취소할 때 서버에서 호출한다.
	UFUNCTION(BlueprintCallable, BlueprintAuthorityOnly, Category = "FPS|Animation")
	void SetAnimationStateTag(FGameplayTag Tag, bool bActive);

private:
	void RefreshMovementTags();

};
